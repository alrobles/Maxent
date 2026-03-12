/*
Copyright (c) 2025 Maxent Contributors

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef MAXENT_FEATURED_SPACE_HPP
#define MAXENT_FEATURED_SPACE_HPP

#include "feature.hpp"

#include <vector>
#include <memory>
#include <string>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace maxent {

// ============================================================================
// Helper structs
// ============================================================================

/**
 * @brief Summary statistics over a set of sample values.
 * Ported from the inner class FeaturedSpace.SampleInfo in Java Maxent.
 */
struct SampleInfo {
    double avg;        ///< Mean value
    double std;        ///< Standard deviation
    double min;        ///< Minimum value
    double max;        ///< Maximum value
    int    sample_cnt; ///< Number of samples

    SampleInfo() : avg(0), std(0), min(0), max(0), sample_cnt(0) {}
    SampleInfo(double avg, double std, double min, double max, int cnt)
        : avg(avg), std(std), min(min), max(max), sample_cnt(cnt) {}
};

/**
 * @brief Confidence interval [low, high] for a feature value.
 * Ported from the inner class FeaturedSpace.Interval in Java Maxent.
 */
struct Interval {
    double low;   ///< Lower bound
    double high;  ///< Upper bound

    Interval() : low(0), high(0) {}
    Interval(double low, double high) : low(low), high(high) {}

    /** Construct interval from SampleInfo and beta multiplier. */
    Interval(const SampleInfo& f, double beta) {
        if (f.sample_cnt == 0) {
            low  = f.min;
            high = f.max;
        } else {
            double spread = beta / std::sqrt(static_cast<double>(f.sample_cnt)) * f.std;
            low  = f.avg - spread;
            high = f.avg + spread;
        }
    }

    /** Construct interval as the ratio of two intervals (for bias correction). */
    Interval(const Interval& a, const Interval& b) {
        if (b.low < 0.0) {
            low  =  std::numeric_limits<double>::infinity();
            high = -std::numeric_limits<double>::infinity();
        } else {
            low  = a.low  / (b.high > 0.0 ? b.high : 1.0);
            high = a.high / (b.low  > 0.0 ? b.low  : 1.0);
        }
    }

    /// Mid-point of the interval
    double mid() const { return 0.5 * (low + high); }
    /// Half-width of the interval
    double dev() const { return 0.5 * (high - low); }
};

/**
 * @brief Result returned by FeaturedSpace::train()
 */
struct TrainResult {
    double loss;          ///< Final regularized loss
    double entropy;       ///< Shannon entropy of the distribution
    int    iterations;    ///< Number of iterations completed
    bool   converged;     ///< Whether convergence threshold was reached
    std::vector<double> lambdas; ///< Final lambda values for all features
};

// ============================================================================
// FeaturedSpace class
// ============================================================================

/**
 * @brief Core MaxEnt featured space: manages the Gibbs distribution over
 *        background points and drives model training.
 *
 * Terminology (following the original Java code):
 *   density[i]         = exp(linearPredictor[i] - linearPredictorNormalizer)
 *   normalized prob    = density[i] / densityNormalizer
 *   linearPredictor[i] = sum_j lambda_j * feature_j.eval(i)
 *
 * Ported from density/FeaturedSpace.java and density/Sequential.java.
 */
class FeaturedSpace {
public:
    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Construct a FeaturedSpace.
     * @param num_points     Number of background points.
     * @param sample_indices 0-based indices of occurrence samples in background.
     * @param features       Feature objects (shared ownership).
     */
    FeaturedSpace(int                                    num_points,
                  std::vector<int>                       sample_indices,
                  std::vector<std::shared_ptr<Feature>>  features)
        : num_points_(num_points)
        , num_samples_(static_cast<int>(sample_indices.size()))
        , sample_indices_(std::move(sample_indices))
        , features_(std::move(features))
        , density_(num_points, 0.0)
        , linear_predictor_(num_points, 0.0)
    {
        for (int idx : sample_indices_) {
            if (idx < 0 || idx >= num_points_)
                throw std::out_of_range("FeaturedSpace: sample index out of range");
        }
        set_linear_predictor();
        set_density();
    }

    // -----------------------------------------------------------------------
    // Core distribution methods
    // -----------------------------------------------------------------------

    /** Recompute linearPredictor[i] = sum_j lambda_j * feature_j.eval(i). */
    void set_linear_predictor() {
        std::fill(linear_predictor_.begin(), linear_predictor_.end(), 0.0);
        for (const auto& f : features_) {
            double lam = f->lambda();
            if (lam == 0.0) continue;
            for (int i = 0; i < num_points_; ++i)
                linear_predictor_[i] += lam * f->eval(i);
        }
        set_linear_predictor_normalizer();
    }

    /** Set linearPredictorNormalizer = max(linearPredictor). */
    void set_linear_predictor_normalizer() {
        if (num_points_ == 0) { linear_predictor_normalizer_ = 0.0; return; }
        linear_predictor_normalizer_ = *std::max_element(
            linear_predictor_.begin(), linear_predictor_.end());
    }

    /**
     * @brief Recompute density array and all feature model expectations.
     *
     * density[i] = exp(linearPredictor[i] - linearPredictorNormalizer)
     * densityNormalizer = sum(density)
     * feature_j.expectation = sum_i density[i]*feature_j(i) / densityNormalizer
     */
    void set_density() {
        density_normalizer_ = 0.0;
        for (int i = 0; i < num_points_; ++i) {
            double d = std::exp(linear_predictor_[i] - linear_predictor_normalizer_);
            density_[i] = d;
            density_normalizer_ += d;
        }
        if (density_normalizer_ > 0.0) {
            for (auto& f : features_) {
                double sum = 0.0;
                for (int i = 0; i < num_points_; ++i)
                    sum += density_[i] * f->eval(i);
                f->set_expectation(sum / density_normalizer_);
            }
        }
        entropy_ = -1.0;
    }

    /**
     * @brief Incrementally increase feature j's lambda by alpha.
     * Updates linearPredictor, linearPredictorNormalizer, and density.
     */
    void increase_lambda(int feature_index, double alpha) {
        if (alpha == 0.0) return;
        auto& f = features_[feature_index];
        f->increase_lambda(alpha);
        for (int i = 0; i < num_points_; ++i) {
            linear_predictor_[i] += alpha * f->eval(i);
            if (linear_predictor_[i] > linear_predictor_normalizer_)
                linear_predictor_normalizer_ = linear_predictor_[i];
        }
        set_density();
    }

    /**
     * @brief Batch-update all lambdas simultaneously.
     * @param alphas Vector of increments, one per feature.
     */
    void increase_lambda(const std::vector<double>& alphas) {
        if (alphas.size() != features_.size())
            throw std::invalid_argument("increase_lambda: alphas size mismatch");
        for (std::size_t j = 0; j < features_.size(); ++j) {
            if (alphas[j] == 0.0) continue;
            features_[j]->increase_lambda(alphas[j]);
            for (int i = 0; i < num_points_; ++i)
                linear_predictor_[i] += alphas[j] * features_[j]->eval(i);
        }
        set_linear_predictor_normalizer();
        set_density();
    }

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    double get_density(int i) const { return density_[i]; }
    double get_density_normalizer() const { return density_normalizer_; }
    double get_linear_predictor_normalizer() const { return linear_predictor_normalizer_; }
    int num_points() const { return num_points_; }
    int num_samples() const { return num_samples_; }
    int num_features() const { return static_cast<int>(features_.size()); }
    const std::vector<std::shared_ptr<Feature>>& features() const { return features_; }

    // -----------------------------------------------------------------------
    // Loss / entropy / weights
    // -----------------------------------------------------------------------

    /**
     * @brief Negative log-likelihood loss.
     * loss = getN1() + log(densityNormalizer)
     * getN1() = -sum_j lambda_j * sampleExpectation_j + linearPredictorNormalizer
     */
    double get_loss() const {
        double n1 = linear_predictor_normalizer_;
        for (const auto& f : features_)
            n1 -= f->lambda() * f->sample_expectation();
        return n1 + std::log(density_normalizer_);
    }

    /** Sum of L1 regularization terms. */
    double get_l1_reg() const {
        double result = 0.0;
        for (const auto& f : features_)
            result += std::abs(f->lambda()) * f->sample_deviation();
        return result;
    }

    /** Shannon entropy H = -sum_i p_i * log(p_i), cached. */
    double get_entropy() const {
        if (entropy_ >= 0.0) return entropy_;
        entropy_ = 0.0;
        for (int i = 0; i < num_points_; ++i) {
            double p = density_[i] / density_normalizer_;
            if (p > 0.0) entropy_ += -p * std::log(p);
        }
        return entropy_;
    }

    /** Return normalized distribution weights (sum to 1). */
    std::vector<double> get_weights() const {
        std::vector<double> w(num_points_);
        for (int i = 0; i < num_points_; ++i)
            w[i] = density_[i] / density_normalizer_;
        return w;
    }

    // -----------------------------------------------------------------------
    // Sample expectations
    // -----------------------------------------------------------------------

    /**
     * @brief Compute sample_expectation and sample_deviation for each feature.
     *
     * sample_expectation = mean of feature over occurrence samples
     * sample_deviation   = beta * std / sqrt(n), clipped to min_deviation
     *
     * Ported from FeaturedSpace.setSampleExpectations() in Java.
     */
    void set_sample_expectations(double beta_multiplier = 1.0,
                                 double min_deviation   = 0.001) {
        for (auto& f : features_) {
            SampleInfo fi = get_sample_info(*f);
            Interval fi_interval(fi, beta_multiplier);

            f->set_sample_expectation(fi_interval.mid());

            double dev = fi_interval.dev();
            if (dev < min_deviation) dev = min_deviation;
            f->set_sample_deviation(dev);
        }
    }

    // -----------------------------------------------------------------------
    // Training
    // -----------------------------------------------------------------------

    /**
     * @brief Run sequential coordinate-ascent MaxEnt optimization.
     *
     * Ported from density/Sequential.java (run(), doSequentialUpdate(),
     * goodAlpha(), reduceAlpha()).
     *
     * @param max_iter             Maximum iterations (default 500).
     * @param convergence_threshold  Stop when 20-iter loss drop < threshold.
     * @param beta_multiplier      Regularization multiplier (default 1.0).
     * @param min_deviation        Sample deviation floor (default 0.001).
     * @return TrainResult with loss, entropy, iterations, converged, lambdas.
     */
    TrainResult train(int    max_iter             = 500,
                      double convergence_threshold = 1e-5,
                      double beta_multiplier       = 1.0,
                      double min_deviation         = 0.001) {
        set_sample_expectations(beta_multiplier, min_deviation);
        entropy_ = -1.0;

        double prev_loss = std::numeric_limits<double>::infinity();
        bool   converged = false;
        int    iter      = 0;

        static constexpr int kConvergenceFreq = 20;

        for (iter = 0; iter < max_iter; ++iter) {
            for (int j = 0; j < static_cast<int>(features_.size()); ++j) {
                double alpha = good_alpha(*features_[j]);
                alpha = reduce_alpha(alpha, iter);
                if (alpha == 0.0) continue;
                increase_lambda(j, alpha);
            }

            if (iter % kConvergenceFreq == 0) {
                double loss = get_loss();
                if (prev_loss - loss < convergence_threshold) {
                    converged = true;
                    ++iter;
                    break;
                }
                prev_loss = loss;
            }
        }

        TrainResult result;
        result.loss       = get_loss();
        result.entropy    = get_entropy();
        result.iterations = iter;
        result.converged  = converged;
        result.lambdas.reserve(features_.size());
        for (const auto& f : features_)
            result.lambdas.push_back(f->lambda());
        return result;
    }

    // -----------------------------------------------------------------------
    // Prediction
    // -----------------------------------------------------------------------

    /**
     * @brief Predict raw Gibbs scores for new feature data.
     * @param feature_matrix  [n_pts x n_features] matrix of pre-evaluated values.
     * @return Exp(sum_j lambda_j * val_j - lpNormalizer) for each new point.
     */
    std::vector<double> predict(
            const std::vector<std::vector<double>>& feature_matrix) const {
        if (feature_matrix.empty()) return {};
        int n_new = static_cast<int>(feature_matrix.size());
        std::vector<double> scores(n_new, 0.0);
        for (int i = 0; i < n_new; ++i) {
            if (static_cast<int>(feature_matrix[i].size()) != num_features())
                throw std::invalid_argument(
                    "predict: feature_matrix row width does not match num_features");
            double lp = 0.0;
            for (int j = 0; j < num_features(); ++j)
                lp += features_[j]->lambda() * feature_matrix[i][j];
            scores[i] = std::exp(lp - linear_predictor_normalizer_);
        }
        return scores;
    }

    // -----------------------------------------------------------------------
    // Lambda file I/O
    // -----------------------------------------------------------------------

    /**
     * @brief Write trained lambdas to a CSV file.
     * Format: featureName, lambda, min, max  (then normalizer metadata lines).
     */
    void write_lambdas(const std::string& filename) const {
        std::ofstream out(filename);
        if (!out)
            throw std::runtime_error("write_lambdas: cannot open: " + filename);

        out << std::setprecision(17);
        for (const auto& f : features_) {
            out << f->name() << ", " << f->lambda() << ", "
                << f->min_val() << ", " << f->max_val() << "\n";
        }
        out << "linearPredictorNormalizer, " << linear_predictor_normalizer_ << "\n";
        out << "densityNormalizer, "          << density_normalizer_          << "\n";
        out << "numBackgroundPoints, "        << num_points_                  << "\n";
        out << "entropy, "                    << get_entropy() << "\n";
    }

    /**
     * @brief Read lambdas from a file and apply them to features by name.
     * Also restores linearPredictorNormalizer, densityNormalizer, and entropy.
     */
    void read_lambdas(const std::string& filename) {
        std::ifstream in(filename);
        if (!in)
            throw std::runtime_error("read_lambdas: cannot open: " + filename);

        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::istringstream ss(line);
            std::string name;
            std::getline(ss, name, ',');
            while (!name.empty() && (name.front() == ' ' || name.front() == '\t'))
                name.erase(name.begin());
            while (!name.empty() && (name.back()  == ' ' || name.back()  == '\t'))
                name.pop_back();

            std::string val_str;
            std::getline(ss, val_str, ',');
            double val = std::stod(val_str);

            if      (name == "linearPredictorNormalizer") linear_predictor_normalizer_ = val;
            else if (name == "densityNormalizer")         density_normalizer_ = val;
            else if (name == "numBackgroundPoints")       { /* informational */ }
            else if (name == "entropy")                   entropy_ = val;
            else {
                for (auto& f : features_) {
                    if (f->name() == name) { f->set_lambda(val); break; }
                }
            }
        }
        set_linear_predictor();
        set_density();
    }

private:
    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    /** Compute SampleInfo for a feature over the occurrence sample indices. */
    SampleInfo get_sample_info(const Feature& feat) const {
        double vmin =  std::numeric_limits<double>::infinity();
        double vmax = -std::numeric_limits<double>::infinity();
        for (int i = 0; i < num_points_; ++i) {
            double v = feat.eval(i);
            if (v < vmin) vmin = v;
            if (v > vmax) vmax = v;
        }

        double avg = 0.0, std_val = 0.0;
        int cnt = static_cast<int>(sample_indices_.size());
        for (int idx : sample_indices_) {
            double v = feat.eval(idx);
            avg     += v;
            std_val += v * v;
        }

        if (cnt == 0) {
            avg     = (vmin + vmax) / 2.0;
            std_val = 0.5 * (vmax - vmin);
        } else if (cnt == 1) {
            avg     /= cnt;
            std_val  = 0.5 * (vmax - vmin);
        } else {
            avg /= cnt;
            double var = std_val / cnt - avg * avg;
            if (var < 0.0) var = 0.0;
            std_val = std::sqrt(var * cnt / (cnt - 1));
            double half_range = 0.5 * (vmax - vmin);
            if (std_val > half_range) std_val = half_range;
        }

        return SampleInfo(avg, std_val, vmin, vmax, cnt);
    }

    /**
     * @brief Compute the "good alpha" coordinate-ascent step for feature feat.
     * Ported from Sequential.goodAlpha() in the Java code.
     *
     * Uses sample expectation (N1), model expectation (W1), and regularization
     * penalty (beta1 = sample_deviation) to find the optimal lambda increment.
     */
    double good_alpha(const Feature& feat) const {
        double N1 = feat.sample_expectation();
        double W1 = feat.expectation();
        double W0 = 1.0 - W1;
        double N0 = 1.0 - N1;

        if (W0 < kEps || W1 < kEps) return 0.0;

        double lambda = feat.lambda();
        double beta1  = feat.sample_deviation();

        // Try positive direction
        if (N1 - beta1 > kEps) {
            double cand = std::log((N1 - beta1) * W0 / ((N0 + beta1) * W1));
            if (std::isfinite(cand) && cand + lambda > 0.0) return cand;
        }

        // Try negative direction
        if (N0 - beta1 > kEps) {
            double cand = std::log((N1 + beta1) * W0 / ((N0 - beta1) * W1));
            if (std::isfinite(cand) && cand + lambda < 0.0) return cand;
        }

        // Default: zero out lambda
        return -lambda;
    }

    /**
     * @brief Scale down alpha in early iterations for numerical stability.
     * Ported from Sequential.reduceAlpha() in the Java code.
     */
    static double reduce_alpha(double alpha, int iteration) {
        if (iteration < 10) return alpha / 50.0;
        if (iteration < 20) return alpha / 10.0;
        if (iteration < 50) return alpha /  3.0;
        return alpha;
    }

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------
    int num_points_;   ///< Number of background points
    int num_samples_;  ///< Number of occurrence samples

    std::vector<int>                      sample_indices_; ///< 0-based sample locations
    std::vector<std::shared_ptr<Feature>> features_;       ///< Feature objects

    std::vector<double> density_;          ///< Unnormalized Gibbs density per point
    std::vector<double> linear_predictor_; ///< Lambda-weighted feature sum per point

    double linear_predictor_normalizer_ = 0.0; ///< max(linearPredictor)
    double density_normalizer_          = 0.0; ///< sum(density) – Z
    mutable double entropy_                     = -1.0; ///< Cached entropy (-1 = invalid)

    static constexpr double kEps = 1e-6;
};

} // namespace maxent

#endif // MAXENT_FEATURED_SPACE_HPP
