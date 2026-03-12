#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <memory>
#include "maxent/feature.hpp"

// Helper: compare doubles within tolerance
static bool near(double a, double b, double tol = 1e-9) {
    return std::fabs(a - b) < tol;
}

int main() {
    using namespace maxent;

    std::cout << "Testing Feature classes..." << std::endl;

    // ----------------------------------------------------------------
    // LinearFeature
    // ----------------------------------------------------------------
    {
        auto vals = std::make_shared<std::vector<double>>(
            std::vector<double>{0.0, 5.0, 10.0, 3.0});
        LinearFeature lf(vals, "temp_linear", 0.0, 10.0);

        assert(lf.type() == "linear");
        assert(lf.name() == "temp_linear");
        assert(lf.size() == 4);

        assert(near(lf.eval(0), 0.0));
        assert(near(lf.eval(1), 0.5));
        assert(near(lf.eval(2), 1.0));
        assert(near(lf.eval(3), 0.3));

        // min == max edge case → always 0
        LinearFeature lf_flat(vals, "flat", 5.0, 5.0);
        assert(near(lf_flat.eval(0), 0.0));
        assert(near(lf_flat.eval(1), 0.0));

        std::cout << "  LinearFeature: OK" << std::endl;
    }

    // ----------------------------------------------------------------
    // QuadraticFeature
    // ----------------------------------------------------------------
    {
        auto vals = std::make_shared<std::vector<double>>(
            std::vector<double>{0.0, 5.0, 10.0});
        QuadraticFeature qf(vals, "temp_quadratic", 0.0, 10.0);

        assert(qf.type() == "quadratic");
        assert(near(qf.eval(0), 0.0));        // (0/10)^2 = 0
        assert(near(qf.eval(1), 0.25));       // (5/10)^2 = 0.25
        assert(near(qf.eval(2), 1.0));        // (10/10)^2 = 1

        // min == max edge case
        QuadraticFeature qf_flat(vals, "flat", 5.0, 5.0);
        assert(near(qf_flat.eval(0), 0.0));

        std::cout << "  QuadraticFeature: OK" << std::endl;
    }

    // ----------------------------------------------------------------
    // ProductFeature
    // ----------------------------------------------------------------
    {
        auto v1 = std::make_shared<std::vector<double>>(
            std::vector<double>{0.0, 5.0, 10.0});
        auto v2 = std::make_shared<std::vector<double>>(
            std::vector<double>{0.0, 2.0, 4.0});
        ProductFeature pf(v1, v2, "temp_x_prec", 0.0, 10.0, 0.0, 4.0);

        assert(pf.type() == "product");
        // i=0: (0/10)*(0/4) = 0
        assert(near(pf.eval(0), 0.0));
        // i=1: (5/10)*(2/4) = 0.5 * 0.5 = 0.25
        assert(near(pf.eval(1), 0.25));
        // i=2: (10/10)*(4/4) = 1.0
        assert(near(pf.eval(2), 1.0));

        std::cout << "  ProductFeature: OK" << std::endl;
    }

    // ----------------------------------------------------------------
    // ThresholdFeature
    // ----------------------------------------------------------------
    {
        auto vals = std::make_shared<std::vector<double>>(
            std::vector<double>{1.0, 5.0, 10.0, 5.0});
        ThresholdFeature tf(vals, "temp_threshold", 5.0);

        assert(tf.type() == "threshold");
        assert(near(tf.eval(0), 0.0));  // 1.0 > 5.0 → false
        assert(near(tf.eval(1), 0.0));  // 5.0 > 5.0 → false (strict >)
        assert(near(tf.eval(2), 1.0));  // 10.0 > 5.0 → true
        assert(near(tf.eval(3), 0.0));  // 5.0 > 5.0 → false (strict >)
        assert(near(tf.threshold(), 5.0));

        std::cout << "  ThresholdFeature: OK" << std::endl;
    }

    // ----------------------------------------------------------------
    // HingeFeature (forward)
    // ----------------------------------------------------------------
    {
        auto vals = std::make_shared<std::vector<double>>(
            std::vector<double>{0.0, 5.0, 10.0, 3.0});
        HingeFeature hf(vals, "temp_hinge", 2.0, 8.0, false);

        assert(hf.type() == "hinge");
        assert(!hf.is_reverse());
        assert(near(hf.min_knot(), 2.0));
        assert(near(hf.max_knot(), 8.0));

        // 0.0 <= 2.0 → 0
        assert(near(hf.eval(0), 0.0));
        // 5.0 > 2.0 → (5-2)/(8-2) = 3/6 = 0.5
        assert(near(hf.eval(1), 0.5));
        // 10.0 > 2.0 → (10-2)/(8-2) = 8/6 ≈ 1.333
        assert(near(hf.eval(2), 8.0 / 6.0));
        // 3.0 > 2.0 → (3-2)/(8-2) = 1/6 ≈ 0.1667
        assert(near(hf.eval(3), 1.0 / 6.0));

        std::cout << "  HingeFeature (forward): OK" << std::endl;
    }

    // ----------------------------------------------------------------
    // HingeFeature (reverse)
    // ----------------------------------------------------------------
    {
        auto vals = std::make_shared<std::vector<double>>(
            std::vector<double>{0.0, 5.0, 10.0, 8.0});
        HingeFeature rhf(vals, "temp_rev_hinge", 2.0, 8.0, true);

        assert(rhf.type() == "reverse_hinge");
        assert(rhf.is_reverse());

        // 0.0 < 8.0 → (8-0)/(8-2) = 8/6 ≈ 1.333
        assert(near(rhf.eval(0), 8.0 / 6.0));
        // 5.0 < 8.0 → (8-5)/(8-2) = 3/6 = 0.5
        assert(near(rhf.eval(1), 0.5));
        // 10.0 >= 8.0 → 0
        assert(near(rhf.eval(2), 0.0));
        // 8.0 < 8.0 → false → 0
        assert(near(rhf.eval(3), 0.0));

        std::cout << "  HingeFeature (reverse): OK" << std::endl;
    }

    // ----------------------------------------------------------------
    // HingeFeature: invalid knots
    // ----------------------------------------------------------------
    {
        auto vals = std::make_shared<std::vector<double>>(
            std::vector<double>{1.0, 2.0});
        bool caught = false;
        try {
            HingeFeature bad(vals, "bad", 5.0, 3.0);
        } catch (const std::invalid_argument&) {
            caught = true;
        }
        assert(caught);
        std::cout << "  HingeFeature invalid knots: OK" << std::endl;
    }

    // ----------------------------------------------------------------
    // FeatureGenerator
    // ----------------------------------------------------------------
    {
        std::vector<std::pair<std::string, std::vector<double>>> data = {
            {"temp",  {0.0, 5.0, 10.0, 3.0, 7.0}},
            {"prec",  {100.0, 200.0, 150.0, 50.0, 300.0}}
        };

        FeatureGenerator::Config cfg;
        cfg.linear    = true;
        cfg.quadratic = true;
        cfg.product   = true;
        cfg.threshold = true;
        cfg.hinge     = true;
        cfg.n_thresholds = 3;
        cfg.n_hinges     = 2;

        auto features = FeatureGenerator::generate(data, cfg);

        // Count expected features per variable:
        //   linear: 1, quadratic: 1, threshold: 3, hinge(fwd+rev): 2*2 = 4  → 9 per var
        // Product pairs: 1
        // Total: 9*2 + 1 = 19
        std::size_t expected = 9 * 2 + 1;
        assert(features.size() == expected);

        // Check types present
        bool has_linear = false, has_quad = false, has_product = false;
        bool has_threshold = false, has_hinge = false, has_rev_hinge = false;
        for (const auto& f : features) {
            if (f->type() == "linear")        has_linear    = true;
            if (f->type() == "quadratic")     has_quad      = true;
            if (f->type() == "product")       has_product   = true;
            if (f->type() == "threshold")     has_threshold = true;
            if (f->type() == "hinge")         has_hinge     = true;
            if (f->type() == "reverse_hinge") has_rev_hinge = true;
        }
        assert(has_linear);
        assert(has_quad);
        assert(has_product);
        assert(has_threshold);
        assert(has_hinge);
        assert(has_rev_hinge);

        std::cout << "  FeatureGenerator: OK (" << features.size()
                  << " features)" << std::endl;
    }

    // FeatureGenerator: single variable, no product
    {
        std::vector<std::pair<std::string, std::vector<double>>> data = {
            {"elev", {100.0, 200.0, 300.0}}
        };
        FeatureGenerator::Config cfg;
        cfg.linear    = true;
        cfg.quadratic = false;
        cfg.product   = false;
        cfg.threshold = false;
        cfg.hinge     = false;

        auto features = FeatureGenerator::generate(data, cfg);
        assert(features.size() == 1);
        assert(features[0]->type() == "linear");
        std::cout << "  FeatureGenerator (linear only): OK" << std::endl;
    }

    // FeatureGenerator: constant variable (min == max) → no threshold/hinge
    {
        std::vector<std::pair<std::string, std::vector<double>>> data = {
            {"const", {5.0, 5.0, 5.0}}
        };
        FeatureGenerator::Config cfg;
        cfg.linear    = true;
        cfg.quadratic = true;
        cfg.product   = false;
        cfg.threshold = true;
        cfg.hinge     = true;
        cfg.n_thresholds = 5;
        cfg.n_hinges     = 5;

        auto features = FeatureGenerator::generate(data, cfg);
        // Only linear + quadratic (threshold/hinge skipped when min==max)
        assert(features.size() == 2);
        std::cout << "  FeatureGenerator (constant variable): OK" << std::endl;
    }

    std::cout << "All Feature tests passed!" << std::endl;
    return 0;
}
