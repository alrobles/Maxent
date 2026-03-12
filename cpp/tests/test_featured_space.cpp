#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <memory>
#include <numeric>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include "maxent/feature.hpp"
#include "maxent/featured_space.hpp"

// Helper: compare doubles within tolerance
static bool near(double a, double b, double tol = 1e-9) {
    return std::fabs(a - b) < tol;
}

// Build a small synthetic dataset
// 100 background points, 10 samples, 2 linear features
static std::vector<std::shared_ptr<maxent::Feature>> make_features(
        const std::vector<double>& env1,
        const std::vector<double>& env2)
{
    using namespace maxent;
    auto v1 = std::make_shared<std::vector<double>>(env1);
    auto v2 = std::make_shared<std::vector<double>>(env2);

    double min1 = *std::min_element(env1.begin(), env1.end());
    double max1 = *std::max_element(env1.begin(), env1.end());
    double min2 = *std::min_element(env2.begin(), env2.end());
    double max2 = *std::max_element(env2.begin(), env2.end());

    std::vector<std::shared_ptr<Feature>> features;
    features.push_back(std::make_shared<LinearFeature>(v1, "env1_linear", min1, max1));
    features.push_back(std::make_shared<LinearFeature>(v2, "env2_linear", min2, max2));
    return features;
}

int main() {
    using namespace maxent;

    std::cout << "Testing FeaturedSpace..." << std::endl;

    // ----------------------------------------------------------------
    // Synthetic data: 100 background points, 10 samples
    // ----------------------------------------------------------------
    const int N = 100;
    const int S = 10;

    std::vector<double> env1(N), env2(N);
    for (int i = 0; i < N; ++i) {
        env1[i] = static_cast<double>(i) / (N - 1);  // 0 to 1
        env2[i] = 1.0 - env1[i];                      // 1 to 0
    }

    // Samples at the high end of env1 (indices 90..99)
    std::vector<int> sample_idx(S);
    for (int k = 0; k < S; ++k) sample_idx[k] = 90 + k;

    // ----------------------------------------------------------------
    // Test 1: Construction and initial uniform distribution
    // ----------------------------------------------------------------
    {
        auto features = make_features(env1, env2);
        FeaturedSpace fs(N, sample_idx, features);

        // All lambdas are 0 → linear_predictor is all 0 → density uniform
        auto weights = fs.get_weights();
        assert(static_cast<int>(weights.size()) == N);

        double wsum = 0.0;
        for (double w : weights) wsum += w;
        assert(near(wsum, 1.0, 1e-12));

        // Each weight should be 1/N
        for (double w : weights) {
            assert(near(w, 1.0 / N, 1e-9));
        }

        // Entropy should be log(N)
        double H = fs.get_entropy();
        assert(H >= 0.0);
        assert(near(H, std::log(N), 1e-9));

        std::cout << "  Test 1 (initial uniform distribution): OK" << std::endl;
    }

    // ----------------------------------------------------------------
    // Test 2: set_linear_predictor and set_density consistency
    // ----------------------------------------------------------------
    {
        auto features = make_features(env1, env2);
        // Manually set a non-zero lambda
        features[0]->set_lambda(1.0);

        FeaturedSpace fs(N, sample_idx, features);
        // After construction, linear predictor should be sum lambda_j * feature_j

        // Check density sums to density_normalizer
        double dsum = 0.0;
        for (int i = 0; i < N; ++i) dsum += fs.get_density(i);
        assert(near(dsum, fs.get_density_normalizer(), 1e-9));

        // Weights should sum to 1
        auto w = fs.get_weights();
        double wsum = 0.0;
        for (double wi : w) wsum += wi;
        assert(near(wsum, 1.0, 1e-9));

        std::cout << "  Test 2 (linear predictor / density consistency): OK" << std::endl;
    }

    // ----------------------------------------------------------------
    // Test 3: increase_lambda updates density correctly
    // ----------------------------------------------------------------
    {
        auto features = make_features(env1, env2);
        FeaturedSpace fs(N, sample_idx, features);

        double loss_before = fs.get_loss();

        // Increase lambda of feature 0
        fs.set_sample_expectations(1.0);
        fs.increase_lambda(0, 0.5);

        // Density should still sum to density_normalizer
        double dsum = 0.0;
        for (int i = 0; i < N; ++i) dsum += fs.get_density(i);
        assert(near(dsum, fs.get_density_normalizer(), 1e-6));

        // Weights should sum to 1
        auto w = fs.get_weights();
        double wsum = 0.0;
        for (double wi : w) wsum += wi;
        assert(near(wsum, 1.0, 1e-9));

        std::cout << "  Test 3 (increase_lambda updates density): OK" << std::endl;
    }

    // ----------------------------------------------------------------
    // Test 4: get_weights sums to ~1.0
    // ----------------------------------------------------------------
    {
        auto features = make_features(env1, env2);
        features[0]->set_lambda(2.0);
        features[1]->set_lambda(-0.5);
        FeaturedSpace fs(N, sample_idx, features);

        auto w = fs.get_weights();
        double wsum = std::accumulate(w.begin(), w.end(), 0.0);
        assert(near(wsum, 1.0, 1e-9));

        std::cout << "  Test 4 (get_weights sums to 1): OK" << std::endl;
    }

    // ----------------------------------------------------------------
    // Test 5: get_entropy is non-negative and bounded by log(N)
    // ----------------------------------------------------------------
    {
        auto features = make_features(env1, env2);
        features[0]->set_lambda(2.0);
        FeaturedSpace fs(N, sample_idx, features);

        double H = fs.get_entropy();
        assert(H >= 0.0);
        assert(H <= std::log(N) + 1e-9);

        std::cout << "  Test 5 (entropy non-negative and bounded): OK" << std::endl;
    }

    // ----------------------------------------------------------------
    // Test 6: Training decreases loss
    // ----------------------------------------------------------------
    {
        auto features = make_features(env1, env2);
        FeaturedSpace fs(N, sample_idx, features);

        // Record initial loss (with all lambdas = 0, sample expectations set)
        fs.set_sample_expectations(1.0);
        double initial_loss = fs.get_loss();

        // Reset and train
        auto features2 = make_features(env1, env2);
        FeaturedSpace fs2(N, sample_idx, features2);
        auto result = fs2.train(200, 1e-5, 1.0);

        assert(result.loss <= initial_loss + 1e-9);
        assert(result.iterations > 0);
        assert(result.lambdas.size() == 2);

        // Weights should still sum to 1
        auto w = fs2.get_weights();
        double wsum = std::accumulate(w.begin(), w.end(), 0.0);
        assert(near(wsum, 1.0, 1e-9));

        // Entropy should be non-negative
        assert(result.entropy >= 0.0);

        std::cout << "  Test 6 (training decreases loss): OK" << std::endl;
        std::cout << "    Initial loss: " << initial_loss
                  << " Final loss: " << result.loss
                  << " Iterations: " << result.iterations << std::endl;
    }

    // ----------------------------------------------------------------
    // Test 7: get_loss returns finite value
    // ----------------------------------------------------------------
    {
        auto features = make_features(env1, env2);
        FeaturedSpace fs(N, sample_idx, features);
        fs.set_sample_expectations(1.0);

        double loss = fs.get_loss();
        assert(std::isfinite(loss));

        std::cout << "  Test 7 (loss is finite): OK" << std::endl;
    }

    // ----------------------------------------------------------------
    // Test 8: write_lambdas / read_lambdas round-trip
    // ----------------------------------------------------------------
    {
        auto features = make_features(env1, env2);
        FeaturedSpace fs(N, sample_idx, features);
        auto result = fs.train(100, 1e-5, 1.0);

        // Write to a temp file
        std::string tmpfile = "/tmp/test_maxent_lambdas_" + std::to_string(getpid()) + ".csv";
        fs.write_lambdas(tmpfile);

        // Read into a fresh FeaturedSpace
        auto features2 = make_features(env1, env2);
        FeaturedSpace fs2(N, sample_idx, features2);
        fs2.read_lambdas(tmpfile);

        // Lambdas should match
        for (int j = 0; j < 2; ++j) {
            assert(near(features[j]->lambda(), features2[j]->lambda(), 1e-12));
        }

        // Density normalizer should be close
        assert(near(fs.get_density_normalizer(), fs2.get_density_normalizer(), 1e-6));

        std::cout << "  Test 8 (write/read lambdas round-trip): OK" << std::endl;
    }

    // ----------------------------------------------------------------
    // Test 9: batch increase_lambda
    // ----------------------------------------------------------------
    {
        auto features = make_features(env1, env2);
        FeaturedSpace fs(N, sample_idx, features);
        fs.set_sample_expectations(1.0);

        std::vector<double> alphas = {0.1, -0.1};
        fs.increase_lambda(alphas);

        auto w = fs.get_weights();
        double wsum = std::accumulate(w.begin(), w.end(), 0.0);
        assert(near(wsum, 1.0, 1e-9));

        std::cout << "  Test 9 (batch increase_lambda): OK" << std::endl;
    }

    // ----------------------------------------------------------------
    // Test 10: predict() output range
    // ----------------------------------------------------------------
    {
        auto features = make_features(env1, env2);
        FeaturedSpace fs(N, sample_idx, features);
        fs.train(100, 1e-5, 1.0);

        // Predict on a single new point with feature values [0.5, 0.5]
        std::vector<std::vector<double>> newdata = {{0.5, 0.5}};
        auto preds = fs.predict(newdata);

        assert(preds.size() == 1);
        assert(std::isfinite(preds[0]));
        assert(preds[0] >= 0.0);

        std::cout << "  Test 10 (predict output range): OK" << std::endl;
    }

    std::cout << "All FeaturedSpace tests passed!" << std::endl;
    return 0;
}
