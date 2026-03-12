/*
 * Unit tests for Clamping (Phase 6).
 */

#include "maxent/clamping.hpp"
#include "maxent/grid.hpp"
#include "maxent/grid_dimension.hpp"

#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

using namespace maxent;

static bool near(double a, double b, double tol = 1e-4) {
    return std::fabs(a - b) < tol;
}

// ---- Test 1: values within range are unchanged ----
void test_no_clamping_needed() {
    GridDimension dim(2, 2, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "temp");
    g0.setValue(0, 0, 10.0f);
    g0.setValue(0, 1, 15.0f);
    g0.setValue(1, 0, 20.0f);
    g0.setValue(1, 1, 25.0f);

    std::vector<const Grid<float>*> grids = {&g0};
    std::vector<double> mins = {5.0};
    std::vector<double> maxs = {30.0};

    auto result = Clamping::clamp(grids, mins, maxs);

    assert(result.clamped_grids.size() == 1);
    assert(near(result.clamped_grids[0].getValue(0, 0), 10.0));
    assert(near(result.clamped_grids[0].getValue(1, 1), 25.0));

    // No clamping occurred: clamp_grid should be 0 everywhere
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 2; ++c)
            assert(near(result.clamp_grid.getValue(r, c), 0.0));

    std::cout << "  Test 1 (no clamping needed): OK" << std::endl;
}

// ---- Test 2: values below min are clamped ----
void test_clamp_below_min() {
    GridDimension dim(1, 2, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "temp");
    g0.setValue(0, 0, -5.0f);    // below min
    g0.setValue(0, 1, 10.0f);    // within range

    std::vector<const Grid<float>*> grids = {&g0};
    std::vector<double> mins = {0.0};
    std::vector<double> maxs = {20.0};

    auto result = Clamping::clamp(grids, mins, maxs);

    assert(near(result.clamped_grids[0].getValue(0, 0), 0.0));
    assert(near(result.clamped_grids[0].getValue(0, 1), 10.0));
    assert(near(result.clamp_grid.getValue(0, 0), 5.0));  // clamped by 5
    assert(near(result.clamp_grid.getValue(0, 1), 0.0));  // no clamping

    std::cout << "  Test 2 (clamp below min): OK" << std::endl;
}

// ---- Test 3: values above max are clamped ----
void test_clamp_above_max() {
    GridDimension dim(1, 2, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "temp");
    g0.setValue(0, 0, 25.0f);    // above max
    g0.setValue(0, 1, 15.0f);    // within range

    std::vector<const Grid<float>*> grids = {&g0};
    std::vector<double> mins = {0.0};
    std::vector<double> maxs = {20.0};

    auto result = Clamping::clamp(grids, mins, maxs);

    assert(near(result.clamped_grids[0].getValue(0, 0), 20.0));
    assert(near(result.clamped_grids[0].getValue(0, 1), 15.0));
    assert(near(result.clamp_grid.getValue(0, 0), 5.0));  // clamped by 5
    assert(near(result.clamp_grid.getValue(0, 1), 0.0));

    std::cout << "  Test 3 (clamp above max): OK" << std::endl;
}

// ---- Test 4: multi-variable clamping accumulates ----
void test_multi_variable_clamping() {
    GridDimension dim(1, 1, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "temp"), g1(dim, "precip");
    g0.setValue(0, 0, -3.0f);   // 3 below min of 0
    g1.setValue(0, 0, 200.0f);  // 100 above max of 100

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<double> mins = {0.0, 0.0};
    std::vector<double> maxs = {30.0, 100.0};

    auto result = Clamping::clamp(grids, mins, maxs);

    assert(near(result.clamped_grids[0].getValue(0, 0), 0.0));
    assert(near(result.clamped_grids[1].getValue(0, 0), 100.0));
    // Total clamping = 3 + 100 = 103
    assert(near(result.clamp_grid.getValue(0, 0), 103.0));

    std::cout << "  Test 4 (multi-variable clamping): OK" << std::endl;
}

// ---- Test 5: NODATA is preserved ----
void test_nodata_preserved() {
    GridDimension dim(1, 2, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "temp");
    g0.setValue(0, 0, g0.getNodataValue());  // NODATA
    g0.setValue(0, 1, 10.0f);

    std::vector<const Grid<float>*> grids = {&g0};
    std::vector<double> mins = {0.0};
    std::vector<double> maxs = {20.0};

    auto result = Clamping::clamp(grids, mins, maxs);

    assert(!result.clamp_grid.hasData(0, 0));  // NODATA propagated
    assert(result.clamp_grid.hasData(0, 1));

    std::cout << "  Test 5 (NODATA preserved): OK" << std::endl;
}

// ---- Test 6: compute_ranges finds correct min/max ----
void test_compute_ranges() {
    GridDimension dim(2, 2, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "temp"), g1(dim, "precip");
    g0.setValue(0, 0, 5.0f);  g0.setValue(0, 1, 15.0f);
    g0.setValue(1, 0, 10.0f); g0.setValue(1, 1, 25.0f);
    g1.setValue(0, 0, 100.0f); g1.setValue(0, 1, 200.0f);
    g1.setValue(1, 0, 50.0f);  g1.setValue(1, 1, 300.0f);

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<double> mins, maxs;
    Clamping::compute_ranges(grids, mins, maxs);

    assert(near(mins[0], 5.0));
    assert(near(maxs[0], 25.0));
    assert(near(mins[1], 50.0));
    assert(near(maxs[1], 300.0));

    std::cout << "  Test 6 (compute_ranges): OK" << std::endl;
}

// ---- Test 7: empty grids throw ----
void test_empty_grids_throw() {
    std::vector<const Grid<float>*> grids;
    std::vector<double> mins, maxs;

    bool threw = false;
    try {
        Clamping::clamp(grids, mins, maxs);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
    std::cout << "  Test 7 (empty grids throw): OK" << std::endl;
}

// ---- Test 8: size mismatch throws ----
void test_size_mismatch_throws() {
    GridDimension dim(2, 2, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "temp");
    std::vector<const Grid<float>*> grids = {&g0};
    std::vector<double> mins = {0.0, 1.0};  // too many
    std::vector<double> maxs = {10.0};

    bool threw = false;
    try {
        Clamping::clamp(grids, mins, maxs);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
    std::cout << "  Test 8 (size mismatch throws): OK" << std::endl;
}

int main() {
    std::cout << "Testing Clamping..." << std::endl;
    test_no_clamping_needed();
    test_clamp_below_min();
    test_clamp_above_max();
    test_multi_variable_clamping();
    test_nodata_preserved();
    test_compute_ranges();
    test_empty_grids_throw();
    test_size_mismatch_throws();
    std::cout << "All Clamping tests passed!" << std::endl;
    return 0;
}
