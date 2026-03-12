/*
 * Unit tests for Projection (Phase 5).
 */

#include "maxent/projection.hpp"
#include "maxent/model_evaluation.hpp"
#include "maxent/feature.hpp"

#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <memory>

using namespace maxent;

static bool near(double a, double b, double tol = 1e-4) {
    return std::fabs(a - b) < tol;
}

// Helper: create a small FeaturedSpace with 2 linear features trained
// on synthetic data so we can test projection.
static FeaturedSpace make_model() {
    // 9 background points (3x3 grid), 2 features
    int n_bg = 9;

    // Background values for features (from grid cells)
    std::vector<double> env0(n_bg), env1(n_bg);
    // Feature 0: gradient 0..1
    for (int i = 0; i < n_bg; ++i) env0[i] = i / 8.0;
    // Feature 1: constant 0.5
    for (int i = 0; i < n_bg; ++i) env1[i] = 0.5;

    // Presence at cells 6, 7, 8 (high values of feature 0)
    std::vector<int> presence_idx = {6, 7, 8};

    // Create feature objects with shared data
    auto v0 = std::make_shared<std::vector<double>>(env0);
    auto v1 = std::make_shared<std::vector<double>>(env1);

    double min0 = 0.0, max0 = 1.0;
    double min1 = 0.5, max1 = 0.5;

    std::vector<std::shared_ptr<Feature>> features;
    features.push_back(std::make_shared<LinearFeature>(v0, "env0_linear", min0, max0));
    features.push_back(std::make_shared<LinearFeature>(v1, "env1_linear", min1, max1));

    FeaturedSpace fs(n_bg, presence_idx, features);
    fs.train(100, 0.001, 1.0);
    return fs;
}

// ---- Test 1: project_raw produces valid output -------------------------
void test_project_raw() {
    auto model = make_model();

    GridDimension dim(3, 3, 0.0, 0.0, 1.0);

    // Create two env grids
    Grid<float> g0(dim, "env0_linear");
    Grid<float> g1(dim, "env1_linear");
    int idx = 0;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            g0.setValue(r, c, static_cast<float>(idx / 8.0));
            g1.setValue(r, c, 0.5f);
            ++idx;
        }
    }

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::string> names = {"env0_linear", "env1_linear"};

    Grid<float> raw = Projection::project_raw(model, grids, names);
    assert(raw.getDimension().nrows == 3);
    assert(raw.getDimension().ncols == 3);

    // All values should be positive (raw Gibbs scores)
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            assert(raw.getValue(r, c) > 0.0f);

    std::cout << "  Test 1 (project_raw): OK\n";
}

// ---- Test 2: project_cloglog in [0,1] ----------------------------------
void test_project_cloglog() {
    auto model = make_model();

    GridDimension dim(3, 3, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "env0_linear");
    Grid<float> g1(dim, "env1_linear");
    int idx = 0;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            g0.setValue(r, c, static_cast<float>(idx / 8.0));
            g1.setValue(r, c, 0.5f);
            ++idx;
        }
    }

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::string> names = {"env0_linear", "env1_linear"};

    Grid<float> out = Projection::project_cloglog(model, grids, names);
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c) {
            float v = out.getValue(r, c);
            assert(v >= 0.0f && v <= 1.0f);
        }

    std::cout << "  Test 2 (project_cloglog in [0,1]): OK\n";
}

// ---- Test 3: project_logistic in [0,1] ---------------------------------
void test_project_logistic() {
    auto model = make_model();

    GridDimension dim(3, 3, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "env0_linear");
    Grid<float> g1(dim, "env1_linear");
    int idx = 0;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            g0.setValue(r, c, static_cast<float>(idx / 8.0));
            g1.setValue(r, c, 0.5f);
            ++idx;
        }
    }

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::string> names = {"env0_linear", "env1_linear"};

    Grid<float> out = Projection::project_logistic(model, grids, names);
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c) {
            float v = out.getValue(r, c);
            assert(v >= 0.0f && v <= 1.0f);
        }

    std::cout << "  Test 3 (project_logistic in [0,1]): OK\n";
}

// ---- Test 4: NODATA handling -------------------------------------------
void test_nodata_handling() {
    auto model = make_model();

    GridDimension dim(3, 3, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "env0_linear");
    Grid<float> g1(dim, "env1_linear");
    int idx = 0;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            g0.setValue(r, c, static_cast<float>(idx / 8.0));
            g1.setValue(r, c, 0.5f);
            ++idx;
        }
    }
    // Set one cell to NODATA
    g0.setValue(1, 1, g0.getNodataValue());

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::string> names = {"env0_linear", "env1_linear"};

    Grid<float> out = Projection::project_raw(model, grids, names);

    // Cell (1,1) should be NODATA
    assert(!out.hasData(1, 1));

    // Other cells should have data
    assert(out.hasData(0, 0));
    assert(out.hasData(2, 2));

    std::cout << "  Test 4 (NODATA handling): OK\n";
}

// ---- Test 5: extract_predictions at sample locations -------------------
void test_extract_predictions() {
    auto model = make_model();

    GridDimension dim(3, 3, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "env0_linear");
    Grid<float> g1(dim, "env1_linear");
    int idx = 0;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            g0.setValue(r, c, static_cast<float>(idx / 8.0));
            g1.setValue(r, c, 0.5f);
            ++idx;
        }
    }

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::string> names = {"env0_linear", "env1_linear"};

    std::vector<int> rows = {0, 1, 2};
    std::vector<int> cols = {0, 1, 2};

    auto preds = Projection::extract_predictions(model, grids, names, rows, cols);
    assert(preds.size() == 3);

    // All predictions should be positive (raw Gibbs scores)
    for (double p : preds)
        assert(p > 0.0);

    std::cout << "  Test 5 (extract_predictions): OK\n";
}

// ---- Test 6: dimension mismatch throws --------------------------------
void test_dimension_mismatch() {
    auto model = make_model();

    GridDimension dim1(3, 3, 0.0, 0.0, 1.0);
    GridDimension dim2(4, 4, 0.0, 0.0, 1.0);  // Different!
    Grid<float> g0(dim1, "env0_linear");
    Grid<float> g1(dim2, "env1_linear");

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::string> names = {"env0_linear", "env1_linear"};

    bool threw = false;
    try { Projection::project_raw(model, grids, names); }
    catch (const std::invalid_argument&) { threw = true; }
    assert(threw);

    std::cout << "  Test 6 (dimension mismatch throws): OK\n";
}

// ---- Test 7: empty grids throws ----------------------------------------
void test_empty_grids_throws() {
    auto model = make_model();

    std::vector<const Grid<float>*> grids;
    std::vector<std::string> names;

    bool threw = false;
    try { Projection::project_raw(model, grids, names); }
    catch (const std::invalid_argument&) { threw = true; }
    assert(threw);

    std::cout << "  Test 7 (empty grids throws): OK\n";
}

// ---- Test 8: higher values where species present -----------------------
void test_prediction_gradient() {
    auto model = make_model();

    GridDimension dim(3, 3, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "env0_linear");
    Grid<float> g1(dim, "env1_linear");
    int idx = 0;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            g0.setValue(r, c, static_cast<float>(idx / 8.0));
            g1.setValue(r, c, 0.5f);
            ++idx;
        }
    }

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::string> names = {"env0_linear", "env1_linear"};

    Grid<float> raw = Projection::project_raw(model, grids, names);

    // Cell (0,0) has low env0 value, cell (2,2) has high env0 value
    // Presence was at high env0 cells, so raw(2,2) > raw(0,0)
    assert(raw.getValue(2, 2) > raw.getValue(0, 0));

    std::cout << "  Test 8 (prediction gradient): OK\n";
}

int main() {
    std::cout << "Testing Projection...\n";
    test_project_raw();
    test_project_cloglog();
    test_project_logistic();
    test_nodata_handling();
    test_extract_predictions();
    test_dimension_mismatch();
    test_empty_grids_throws();
    test_prediction_gradient();
    std::cout << "All Projection tests passed!\n";
    return 0;
}
