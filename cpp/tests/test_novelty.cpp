/*
 * Unit tests for Novelty / MESS (Phase 6).
 */

#include "maxent/novelty.hpp"
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

// ---- Test 1: all cells within training range → MESS positive ----
void test_within_range_positive() {
    GridDimension dim(2, 2, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "temp");
    g0.setValue(0, 0, 10.0f);
    g0.setValue(0, 1, 15.0f);
    g0.setValue(1, 0, 12.0f);
    g0.setValue(1, 1, 18.0f);

    std::vector<const Grid<float>*> grids = {&g0};
    std::vector<std::vector<double>> refs = {{5.0, 10.0, 15.0, 20.0, 25.0}};
    std::vector<std::string> names = {"temp"};

    auto result = Novelty::mess(grids, refs, names);

    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 2; ++c)
            assert(result.mess_grid.getValue(r, c) >= 0.0f);

    std::cout << "  Test 1 (within range → positive MESS): OK" << std::endl;
}

// ---- Test 2: cell below training min → MESS negative ----
void test_below_min_negative() {
    GridDimension dim(1, 1, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "temp");
    g0.setValue(0, 0, -10.0f);  // well below training min of 0

    std::vector<const Grid<float>*> grids = {&g0};
    std::vector<std::vector<double>> refs = {{0.0, 5.0, 10.0, 15.0, 20.0}};
    std::vector<std::string> names = {"temp"};

    auto result = Novelty::mess(grids, refs, names);

    assert(result.mess_grid.getValue(0, 0) < 0.0f);

    std::cout << "  Test 2 (below min → negative MESS): OK" << std::endl;
}

// ---- Test 3: cell above training max → MESS negative ----
void test_above_max_negative() {
    GridDimension dim(1, 1, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "temp");
    g0.setValue(0, 0, 30.0f);  // above training max of 20

    std::vector<const Grid<float>*> grids = {&g0};
    std::vector<std::vector<double>> refs = {{0.0, 5.0, 10.0, 15.0, 20.0}};
    std::vector<std::string> names = {"temp"};

    auto result = Novelty::mess(grids, refs, names);

    assert(result.mess_grid.getValue(0, 0) < 0.0f);

    std::cout << "  Test 3 (above max → negative MESS): OK" << std::endl;
}

// ---- Test 4: MoD grid identifies the most dissimilar variable ----
void test_mod_identification() {
    GridDimension dim(1, 1, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "temp"), g1(dim, "precip");
    g0.setValue(0, 0, 10.0f);   // within range
    g1.setValue(0, 0, -50.0f);  // far below range

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::vector<double>> refs = {
        {5.0, 10.0, 15.0, 20.0},
        {0.0, 50.0, 100.0, 150.0}
    };
    std::vector<std::string> names = {"temp", "precip"};

    auto result = Novelty::mess(grids, refs, names);

    // MoD should be 2 (precip, 1-based) because precip is novel
    assert(near(result.mod_grid.getValue(0, 0), 2.0));

    std::cout << "  Test 4 (MoD identifies precip): OK" << std::endl;
}

// ---- Test 5: NODATA propagation ----
void test_nodata_propagation() {
    GridDimension dim(1, 2, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "temp");
    g0.setValue(0, 0, g0.getNodataValue());  // NODATA
    g0.setValue(0, 1, 10.0f);

    std::vector<const Grid<float>*> grids = {&g0};
    std::vector<std::vector<double>> refs = {{0.0, 10.0, 20.0}};
    std::vector<std::string> names = {"temp"};

    auto result = Novelty::mess(grids, refs, names);

    assert(!result.mess_grid.hasData(0, 0));
    assert(result.mess_grid.hasData(0, 1));

    std::cout << "  Test 5 (NODATA propagation): OK" << std::endl;
}

// ---- Test 6: mess_range simplified version ----
void test_mess_range() {
    GridDimension dim(1, 3, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "temp");
    g0.setValue(0, 0, -5.0f);   // below min → negative
    g0.setValue(0, 1, 10.0f);   // within range → positive
    g0.setValue(0, 2, 25.0f);   // above max → negative

    std::vector<const Grid<float>*> grids = {&g0};
    std::vector<double> mins = {0.0};
    std::vector<double> maxs = {20.0};

    auto result = Novelty::mess_range(grids, mins, maxs);

    assert(result.mess_grid.getValue(0, 0) < 0.0f);   // novel
    assert(result.mess_grid.getValue(0, 1) >= 0.0f);   // within range
    assert(result.mess_grid.getValue(0, 2) < 0.0f);    // novel

    std::cout << "  Test 6 (mess_range): OK" << std::endl;
}

// ---- Test 7: empty inputs throw ----
void test_empty_inputs_throw() {
    std::vector<const Grid<float>*> grids;
    std::vector<std::vector<double>> refs;
    std::vector<std::string> names;

    bool threw = false;
    try {
        Novelty::mess(grids, refs, names);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
    std::cout << "  Test 7 (empty inputs throw): OK" << std::endl;
}

// ---- Test 8: size mismatch throws ----
void test_size_mismatch_throws() {
    GridDimension dim(1, 1, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "temp");

    std::vector<const Grid<float>*> grids = {&g0};
    std::vector<std::vector<double>> refs = {{1.0}, {2.0}};  // mismatch
    std::vector<std::string> names = {"temp"};

    bool threw = false;
    try {
        Novelty::mess(grids, refs, names);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
    std::cout << "  Test 8 (size mismatch throws): OK" << std::endl;
}

int main() {
    std::cout << "Testing Novelty (MESS)..." << std::endl;
    test_within_range_positive();
    test_below_min_negative();
    test_above_max_negative();
    test_mod_identification();
    test_nodata_propagation();
    test_mess_range();
    test_empty_inputs_throw();
    test_size_mismatch_throws();
    std::cout << "All Novelty tests passed!" << std::endl;
    return 0;
}
