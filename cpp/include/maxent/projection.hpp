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

#ifndef MAXENT_PROJECTION_HPP
#define MAXENT_PROJECTION_HPP

#include "featured_space.hpp"
#include "grid.hpp"
#include "grid_dimension.hpp"

#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace maxent {

// ============================================================================
// Projection – apply a trained FeaturedSpace model to environmental grids
// Ported from density/Project.java
// ============================================================================

/**
 * @brief Project a trained Maxent model onto environmental grids.
 *
 * Given a set of environmental variable grids and a trained FeaturedSpace
 * model, computes raw Gibbs scores (or optionally cloglog-transformed scores)
 * for every cell in the study area.
 *
 * Inspired by density/Project.java in the Java Maxent implementation.
 */
class Projection {
public:

    /**
     * @brief Project model onto grids, producing a raw-score output grid.
     *
     * For each cell, evaluates all features at the environmental variable
     * values for that cell, then computes:
     *   raw_score = exp(sum_j lambda_j * feature_j(env) - lpNormalizer)
     *
     * Cells where any input grid has NODATA are marked NODATA in the output.
     *
     * @param model          Trained FeaturedSpace with lambdas set.
     * @param env_grids      Environmental variable grids (one per feature input).
     * @param feature_names  Names of environment variables, in order matching
     *                       env_grids. Must match the feature names used during
     *                       training.
     * @return A Grid<float> with raw Gibbs scores.
     */
    static Grid<float> project_raw(
            const FeaturedSpace& model,
            const std::vector<const Grid<float>*>& env_grids,
            const std::vector<std::string>& feature_names) {

        validate_inputs(model, env_grids, feature_names);

        const auto& dim = env_grids[0]->getDimension();
        int nrows = dim.nrows;
        int ncols = dim.ncols;

        Grid<float> output(dim, "raw_prediction");

        for (int r = 0; r < nrows; ++r) {
            for (int c = 0; c < ncols; ++c) {
                // Check for NODATA in any input grid
                bool has_nodata = false;
                for (const auto* g : env_grids) {
                    if (!g->hasData(r, c)) {
                        has_nodata = true;
                        break;
                    }
                }
                if (has_nodata) {
                    output.setValue(r, c, output.getNodataValue());
                    continue;
                }

                // Build feature vector for this cell
                std::vector<double> env_values(env_grids.size());
                for (size_t k = 0; k < env_grids.size(); ++k)
                    env_values[k] = static_cast<double>(env_grids[k]->getValue(r, c));

                // Predict using the model (single point)
                std::vector<std::vector<double>> fm = { env_values };
                std::vector<double> scores = model.predict(fm);
                output.setValue(r, c, static_cast<float>(scores[0]));
            }
        }

        return output;
    }

    /**
     * @brief Project model onto grids, producing a cloglog-transformed output.
     *
     * cloglog(x) = 1 - exp(-x)
     *
     * This is the recommended output format for Maxent v3.4+.
     *
     * @param model          Trained FeaturedSpace with lambdas set.
     * @param env_grids      Environmental variable grids.
     * @param feature_names  Names of environment variables.
     * @return A Grid<float> with cloglog-transformed scores in [0, 1].
     */
    static Grid<float> project_cloglog(
            const FeaturedSpace& model,
            const std::vector<const Grid<float>*>& env_grids,
            const std::vector<std::string>& feature_names) {

        Grid<float> raw = project_raw(model, env_grids, feature_names);
        const auto& dim = raw.getDimension();
        int nrows = dim.nrows;
        int ncols = dim.ncols;

        Grid<float> output(dim, "cloglog_prediction");

        for (int r = 0; r < nrows; ++r) {
            for (int c = 0; c < ncols; ++c) {
                if (!raw.hasData(r, c)) {
                    output.setValue(r, c, output.getNodataValue());
                    continue;
                }
                double raw_val = static_cast<double>(raw.getValue(r, c));
                double cloglog = 1.0 - std::exp(-raw_val);
                output.setValue(r, c, static_cast<float>(cloglog));
            }
        }

        return output;
    }

    /**
     * @brief Project model onto grids, producing a logistic output.
     *
     * logistic(x) = x / (1 + x)
     *
     * @param model          Trained FeaturedSpace with lambdas set.
     * @param env_grids      Environmental variable grids.
     * @param feature_names  Names of environment variables.
     * @return A Grid<float> with logistic scores in [0, 1].
     */
    static Grid<float> project_logistic(
            const FeaturedSpace& model,
            const std::vector<const Grid<float>*>& env_grids,
            const std::vector<std::string>& feature_names) {

        Grid<float> raw = project_raw(model, env_grids, feature_names);
        const auto& dim = raw.getDimension();
        int nrows = dim.nrows;
        int ncols = dim.ncols;

        Grid<float> output(dim, "logistic_prediction");

        for (int r = 0; r < nrows; ++r) {
            for (int c = 0; c < ncols; ++c) {
                if (!raw.hasData(r, c)) {
                    output.setValue(r, c, output.getNodataValue());
                    continue;
                }
                double raw_val = static_cast<double>(raw.getValue(r, c));
                double logistic = raw_val / (1.0 + raw_val);
                output.setValue(r, c, static_cast<float>(logistic));
            }
        }

        return output;
    }

    /**
     * @brief Extract prediction scores at specific sample locations.
     *
     * Useful for evaluating the model: given test occurrence points and
     * environmental grids, extract the model's prediction score at each
     * point location.
     *
     * @param model          Trained FeaturedSpace with lambdas set.
     * @param env_grids      Environmental variable grids.
     * @param feature_names  Names of environment variables.
     * @param rows           Row indices of sample locations.
     * @param cols           Column indices of sample locations.
     * @return Vector of raw prediction scores at each sample location.
     *         NaN for locations where any grid has NODATA.
     */
    static std::vector<double> extract_predictions(
            const FeaturedSpace& model,
            const std::vector<const Grid<float>*>& env_grids,
            const std::vector<std::string>& feature_names,
            const std::vector<int>& rows,
            const std::vector<int>& cols) {

        if (rows.size() != cols.size())
            throw std::invalid_argument(
                "extract_predictions: rows and cols must have the same length");

        validate_inputs(model, env_grids, feature_names);

        int n = static_cast<int>(rows.size());
        std::vector<double> results(n, std::numeric_limits<double>::quiet_NaN());

        for (int i = 0; i < n; ++i) {
            int r = rows[i], c = cols[i];

            // Check NODATA
            bool has_nodata = false;
            for (const auto* g : env_grids) {
                if (!g->hasData(r, c)) {
                    has_nodata = true;
                    break;
                }
            }
            if (has_nodata) continue;

            std::vector<double> env_values(env_grids.size());
            for (size_t k = 0; k < env_grids.size(); ++k)
                env_values[k] = static_cast<double>(env_grids[k]->getValue(r, c));

            std::vector<std::vector<double>> fm = { env_values };
            std::vector<double> scores = model.predict(fm);
            results[i] = scores[0];
        }

        return results;
    }

private:

    /**
     * @brief Validate projection inputs.
     */
    static void validate_inputs(
            const FeaturedSpace& model,
            const std::vector<const Grid<float>*>& env_grids,
            const std::vector<std::string>& feature_names) {
        if (env_grids.empty())
            throw std::invalid_argument("project: env_grids must not be empty");
        if (env_grids.size() != feature_names.size())
            throw std::invalid_argument(
                "project: env_grids and feature_names must have the same length");
        if (static_cast<int>(env_grids.size()) != model.num_features())
            throw std::invalid_argument(
                "project: number of env_grids must equal model num_features");

        // Verify all grids share the same dimensions
        const auto& ref_dim = env_grids[0]->getDimension();
        for (size_t i = 1; i < env_grids.size(); ++i) {
            const auto& d = env_grids[i]->getDimension();
            if (d.nrows != ref_dim.nrows || d.ncols != ref_dim.ncols)
                throw std::invalid_argument(
                    "project: all env_grids must have the same dimensions");
        }
    }
};

} // namespace maxent

#endif // MAXENT_PROJECTION_HPP
