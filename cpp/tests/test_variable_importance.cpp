/*
 * Unit tests for VariableImportance (Phase 6).
 */

#include "maxent/variable_importance.hpp"
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

// Helper: build grids matching the model
static void make_grids(Grid<float>& g0, Grid<float>& g1) {
    int idx = 0;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c) {
            g0.setValue(r, c, static_cast<float>(idx / 8.0));
            g1.setValue(r, c, 0.5f);
            ++idx;
        }
}

// ---- Test 1: permutation importance returns correct size ----
void test_perm_importance_size() {
    auto model = make_model();
    GridDimension dim(3, 3, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "env0"), g1(dim, "env1");
    make_grids(g0, g1);

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::string> names = {"env0", "env1"};
    std::vector<int> pres_r = {2, 2, 2}, pres_c = {0, 1, 2};
    std::vector<int> abs_r = {0, 0, 0},  abs_c = {0, 1, 2};

    auto results = VariableImportance::permutation_importance(
        model, grids, names, pres_r, pres_c, abs_r, abs_c);

    assert(results.size() == 2);
    assert(results[0].name == "env0");
    assert(results[1].name == "env1");
    std::cout << "  Test 1 (perm importance size): OK" << std::endl;
}

// ---- Test 2: importances sum to ~100% (or 0 if all zero) ----
void test_perm_importance_sums() {
    auto model = make_model();
    GridDimension dim(3, 3, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "env0"), g1(dim, "env1");
    make_grids(g0, g1);

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::string> names = {"env0", "env1"};
    std::vector<int> pres_r = {2, 2, 2}, pres_c = {0, 1, 2};
    std::vector<int> abs_r = {0, 0, 0},  abs_c = {0, 1, 2};

    auto results = VariableImportance::permutation_importance(
        model, grids, names, pres_r, pres_c, abs_r, abs_c);

    double total = 0.0;
    for (const auto& r : results) total += r.permutation_importance;

    // Should sum to 100% or be 0 (if no AUC drop)
    assert(near(total, 100.0) || near(total, 0.0));
    std::cout << "  Test 2 (perm importance sums to 100%): OK" << std::endl;
}

// ---- Test 3: env0 should be more important than env1 (constant) ----
void test_perm_importance_ranking() {
    auto model = make_model();
    GridDimension dim(3, 3, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "env0"), g1(dim, "env1");
    make_grids(g0, g1);

    std::vector<const Grid<float>*> grids = {&g0, &g1};
    std::vector<std::string> names = {"env0", "env1"};
    std::vector<int> pres_r = {2, 2, 2}, pres_c = {0, 1, 2};
    std::vector<int> abs_r = {0, 0, 0},  abs_c = {0, 1, 2};

    auto results = VariableImportance::permutation_importance(
        model, grids, names, pres_r, pres_c, abs_r, abs_c);

    // env0 has a gradient and presence is at high values, so permuting
    // it should cause a bigger AUC drop than permuting constant env1
    assert(results[0].permutation_importance >= results[1].permutation_importance);
    std::cout << "  Test 3 (env0 more important than constant env1): OK" << std::endl;
}

// ---- Test 4: percent contribution returns correct size ----
void test_percent_contribution_size() {
    auto model = make_model();
    std::vector<std::string> names = {"env0", "env1"};

    auto results = VariableImportance::percent_contribution(model, names);
    assert(results.size() == 2);
    assert(results[0].name == "env0");
    assert(results[1].name == "env1");
    std::cout << "  Test 4 (percent contribution size): OK" << std::endl;
}

// ---- Test 5: percent contributions sum to 100% ----
void test_percent_contribution_sums() {
    auto model = make_model();
    std::vector<std::string> names = {"env0", "env1"};

    auto results = VariableImportance::percent_contribution(model, names);
    double total = 0.0;
    for (const auto& r : results) total += r.contribution;

    assert(near(total, 100.0) || near(total, 0.0));
    std::cout << "  Test 5 (percent contribution sums to 100%): OK" << std::endl;
}

// ---- Test 6: invalid inputs throw ----
void test_invalid_inputs() {
    auto model = make_model();
    GridDimension dim(3, 3, 0.0, 0.0, 1.0);
    Grid<float> g0(dim, "env0");

    std::vector<const Grid<float>*> grids = {&g0};
    std::vector<std::string> names = {"env0", "env1"};  // mismatch
    std::vector<int> pr = {0}, pc = {0}, ar = {1}, ac = {1};

    bool threw = false;
    try {
        VariableImportance::permutation_importance(
            model, grids, names, pr, pc, ar, ac);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
    std::cout << "  Test 6 (invalid inputs throw): OK" << std::endl;
}

int main() {
    std::cout << "Testing VariableImportance..." << std::endl;
    test_perm_importance_size();
    test_perm_importance_sums();
    test_perm_importance_ranking();
    test_percent_contribution_size();
    test_percent_contribution_sums();
    test_invalid_inputs();
    std::cout << "All VariableImportance tests passed!" << std::endl;
    return 0;
}
