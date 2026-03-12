/*
 * Unit tests for ResponseCurve (Phase 6).
 */

#include "maxent/response_curve.hpp"
#include "maxent/feature.hpp"
#include "maxent/grid.hpp"
#include "maxent/grid_dimension.hpp"

#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <memory>

using namespace maxent;

static bool near(double a, double b, double tol = 1e-4) {
    return std::fabs(a - b) < tol;
}

// Helper: create a small trained model on a 3x3 grid with 2 features.
static FeaturedSpace make_model() {
    int n_bg = 9;
    std::vector<double> env0(n_bg), env1(n_bg);
    for (int i = 0; i < n_bg; ++i) env0[i] = i / 8.0;
    for (int i = 0; i < n_bg; ++i) env1[i] = 0.5;

    std::vector<int> presence_idx = {6, 7, 8};

    auto v0 = std::make_shared<std::vector<double>>(env0);
    auto v1 = std::make_shared<std::vector<double>>(env1);

    std::vector<std::shared_ptr<Feature>> features;
    features.push_back(std::make_shared<LinearFeature>(v0, "env0", 0.0, 1.0));
    features.push_back(std::make_shared<LinearFeature>(v1, "env1", 0.5, 0.5));

    FeaturedSpace fs(n_bg, presence_idx, features);
    fs.train(100, 0.001, 1.0);
    return fs;
}

// Helper: build grids
static void make_grids(Grid<float>& g0, Grid<float>& g1) {
    int idx = 0;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c) {
            g0.setValue(r, c, static_cast<float>(idx / 8.0));
            g1.setValue(r, c, 0.5f);
            ++idx;
        }
}

// ---- Test 1: marginal curve has correct size ----
void test_marginal_size() {
    auto model = make_model();
    GridDimension dim(3, 3, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "env0"), g1(dim, "env1");
    make_grids(g0, g1);

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::string> names = {"env0", "env1"};

    auto curve = ResponseCurve::marginal(model, grids, names, 0, 50);
    assert(static_cast<int>(curve.size()) == 50);
    std::cout << "  Test 1 (marginal curve size): OK" << std::endl;
}

// ---- Test 2: marginal curve spans variable range ----
void test_marginal_range() {
    auto model = make_model();
    GridDimension dim(3, 3, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "env0"), g1(dim, "env1");
    make_grids(g0, g1);

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::string> names = {"env0", "env1"};

    auto curve = ResponseCurve::marginal(model, grids, names, 0, 50);

    // First value should be at min (0.0), last at max (1.0)
    assert(near(curve.front().value, 0.0));
    assert(near(curve.back().value, 1.0));
    std::cout << "  Test 2 (marginal spans range): OK" << std::endl;
}

// ---- Test 3: predictions are in [0,1] (cloglog output) ----
void test_marginal_prediction_range() {
    auto model = make_model();
    GridDimension dim(3, 3, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "env0"), g1(dim, "env1");
    make_grids(g0, g1);

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::string> names = {"env0", "env1"};

    auto curve = ResponseCurve::marginal(model, grids, names, 0, 50);

    for (const auto& pt : curve) {
        assert(pt.prediction >= 0.0);
        assert(pt.prediction <= 1.0);
    }
    std::cout << "  Test 3 (predictions in [0,1]): OK" << std::endl;
}

// ---- Test 4: response should increase with env0 (presence at high values) ----
void test_marginal_increasing() {
    auto model = make_model();
    GridDimension dim(3, 3, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "env0"), g1(dim, "env1");
    make_grids(g0, g1);

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::string> names = {"env0", "env1"};

    auto curve = ResponseCurve::marginal(model, grids, names, 0, 10);

    // Prediction at max should be > prediction at min
    assert(curve.back().prediction > curve.front().prediction);
    std::cout << "  Test 4 (response increases with env0): OK" << std::endl;
}

// ---- Test 5: marginal_fixed works with explicit fixed values ----
void test_marginal_fixed() {
    auto model = make_model();
    std::vector<double> fixed = {0.5, 0.5};
    std::vector<std::string> names = {"env0", "env1"};

    auto curve = ResponseCurve::marginal_fixed(
        model, fixed, names, 0, 0.0, 1.0, 20);

    assert(static_cast<int>(curve.size()) == 20);
    assert(near(curve.front().value, 0.0));
    assert(near(curve.back().value, 1.0));
    for (const auto& pt : curve)
        assert(pt.prediction >= 0.0 && pt.prediction <= 1.0);
    std::cout << "  Test 5 (marginal_fixed): OK" << std::endl;
}

// ---- Test 6: var_index out of range throws ----
void test_invalid_var_index() {
    auto model = make_model();
    GridDimension dim(3, 3, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "env0"), g1(dim, "env1");
    make_grids(g0, g1);

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::string> names = {"env0", "env1"};

    bool threw = false;
    try {
        ResponseCurve::marginal(model, grids, names, 5, 10);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
    std::cout << "  Test 6 (invalid var_index throws): OK" << std::endl;
}

// ---- Test 7: n_steps < 2 throws ----
void test_invalid_nsteps() {
    auto model = make_model();
    GridDimension dim(3, 3, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "env0"), g1(dim, "env1");
    make_grids(g0, g1);

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::string> names = {"env0", "env1"};

    bool threw = false;
    try {
        ResponseCurve::marginal(model, grids, names, 0, 1);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
    std::cout << "  Test 7 (n_steps < 2 throws): OK" << std::endl;
}

int main() {
    std::cout << "Testing ResponseCurve..." << std::endl;
    test_marginal_size();
    test_marginal_range();
    test_marginal_prediction_range();
    test_marginal_increasing();
    test_marginal_fixed();
    test_invalid_var_index();
    test_invalid_nsteps();
    std::cout << "All ResponseCurve tests passed!" << std::endl;
    return 0;
}
