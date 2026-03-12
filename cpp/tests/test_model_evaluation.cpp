/*
 * Unit tests for ModelEvaluation (Phase 5).
 */

#include "maxent/model_evaluation.hpp"

#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

using namespace maxent;

static bool near(double a, double b, double tol = 1e-6) {
    return std::fabs(a - b) < tol;
}

// ---- Test 1: mean, variance, stddev -----------------------------------
void test_basic_stats() {
    std::vector<double> x = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    double m = ModelEvaluation::mean(x);
    assert(near(m, 5.0));

    double v = ModelEvaluation::variance(x);
    assert(near(v, 4.0));

    double s = ModelEvaluation::stddev(x);
    assert(near(s, 2.0));

    // empty
    assert(ModelEvaluation::mean({}) == 0.0);
    assert(ModelEvaluation::variance({}) == 0.0);
    assert(ModelEvaluation::stddev({}) == 0.0);

    std::cout << "  Test 1 (basic stats): OK\n";
}

// ---- Test 2: Pearson correlation --------------------------------------
void test_correlation() {
    // Perfect positive correlation
    std::vector<double> x = {1, 2, 3, 4, 5};
    std::vector<double> y = {2, 4, 6, 8, 10};
    double r = ModelEvaluation::correlation(x, y);
    assert(near(r, 1.0, 1e-9));

    // Perfect negative
    std::vector<double> yn = {10, 8, 6, 4, 2};
    r = ModelEvaluation::correlation(x, yn);
    assert(near(r, -1.0, 1e-9));

    // Zero correlation (constant)
    std::vector<double> yc = {5, 5, 5, 5, 5};
    r = ModelEvaluation::correlation(x, yc);
    assert(near(r, 0.0, 1e-9));

    std::cout << "  Test 2 (Pearson correlation): OK\n";
}

// ---- Test 3: correlation length mismatch throws -----------------------
void test_correlation_mismatch() {
    std::vector<double> x = {1, 2, 3};
    std::vector<double> y = {1, 2};
    bool threw = false;
    try { ModelEvaluation::correlation(x, y); }
    catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
    std::cout << "  Test 3 (correlation mismatch): OK\n";
}

// ---- Test 4: AUC – perfect separation ---------------------------------
void test_auc_perfect() {
    std::vector<double> presence = {0.8, 0.9, 1.0};
    std::vector<double> absence  = {0.1, 0.2, 0.3};
    double max_k, max_kt;
    double auc = ModelEvaluation::auc(presence, absence, &max_k, &max_kt);
    assert(near(auc, 1.0));
    std::cout << "  Test 4 (AUC perfect separation): OK\n";
}

// ---- Test 5: AUC – no discrimination ----------------------------------
void test_auc_random() {
    // Same values => AUC = 0.5
    std::vector<double> presence = {0.5, 0.5, 0.5};
    std::vector<double> absence  = {0.5, 0.5, 0.5};
    double auc = ModelEvaluation::auc(presence, absence);
    assert(near(auc, 0.5));
    std::cout << "  Test 5 (AUC no discrimination): OK\n";
}

// ---- Test 6: AUC – empty inputs return 0.5 ----------------------------
void test_auc_empty() {
    double auc = ModelEvaluation::auc({}, {0.1, 0.2});
    assert(near(auc, 0.5));
    auc = ModelEvaluation::auc({0.9}, {});
    assert(near(auc, 0.5));
    std::cout << "  Test 6 (AUC empty inputs): OK\n";
}

// ---- Test 7: AUC – partial overlap ------------------------------------
void test_auc_partial() {
    std::vector<double> presence = {0.6, 0.7, 0.8};
    std::vector<double> absence  = {0.3, 0.5, 0.65};
    double mk, mkt;
    double auc = ModelEvaluation::auc(presence, absence, &mk, &mkt);
    // With this data: AUC should be > 0.5 and < 1.0
    assert(auc > 0.5 && auc < 1.0);
    // Max kappa should be non-negative
    assert(mk >= 0.0);
    std::cout << "  Test 7 (AUC partial overlap): OK\n";
}

// ---- Test 8: log-loss -------------------------------------------------
void test_logloss() {
    // Perfect prediction: presence=1.0, absence=0.0 => logloss ~ 0
    std::vector<double> p = {1.0, 1.0};
    std::vector<double> a = {0.0, 0.0};
    double ll = ModelEvaluation::logloss(p, a);
    assert(near(ll, 0.0, 1e-9));

    // Worst prediction: presence=0.0, absence=1.0 => logloss = 20 (capped)
    std::vector<double> p2 = {0.0};
    std::vector<double> a2 = {1.0};
    double ll2 = ModelEvaluation::logloss(p2, a2);
    assert(ll2 > 0.0);
    assert(near(ll2, 20.0));  // capped at exp(-20)

    std::cout << "  Test 8 (log-loss): OK\n";
}

// ---- Test 9: squared error --------------------------------------------
void test_square_error() {
    // Perfect prediction
    std::vector<double> p = {1.0, 1.0};
    std::vector<double> a = {0.0, 0.0};
    double se = ModelEvaluation::square_error(p, a);
    assert(near(se, 0.0));

    // Worst prediction
    std::vector<double> p2 = {0.0, 0.0};
    std::vector<double> a2 = {1.0, 1.0};
    double se2 = ModelEvaluation::square_error(p2, a2);
    assert(near(se2, 1.0));

    // 50/50
    std::vector<double> p3 = {0.5};
    std::vector<double> a3 = {0.5};
    double se3 = ModelEvaluation::square_error(p3, a3);
    assert(near(se3, 0.25));

    std::cout << "  Test 9 (squared error): OK\n";
}

// ---- Test 10: misclassification ---------------------------------------
void test_misclassification() {
    // All correct
    std::vector<double> p = {0.8, 0.9};
    std::vector<double> a = {0.1, 0.2};
    double mc = ModelEvaluation::misclassification(p, a);
    assert(near(mc, 0.0));

    // All wrong
    std::vector<double> p2 = {0.1, 0.2};
    std::vector<double> a2 = {0.8, 0.9};
    double mc2 = ModelEvaluation::misclassification(p2, a2);
    assert(near(mc2, 1.0));

    std::cout << "  Test 10 (misclassification): OK\n";
}

// ---- Test 11: kappa ---------------------------------------------------
void test_kappa() {
    // np = false negatives, nn = true negatives
    // tp = total positives, tn = total negatives
    // Perfect classification: np=0 (no false neg), nn=tn (all neg correct)
    double k = ModelEvaluation::kappa(0, 5, 5, 5);
    assert(near(k, 1.0));

    // All classified as positive (no negatives classified correctly)
    // np=0, nn=0 => observed = 0 + 5 - 0 = 5, expected = 0*5/10 + 10*5/10 = 5
    double k2 = ModelEvaluation::kappa(0, 0, 5, 5);
    assert(near(k2, 0.0));

    std::cout << "  Test 11 (kappa): OK\n";
}

// ---- Test 12: full evaluate -------------------------------------------
void test_evaluate() {
    std::vector<double> presence = {0.9, 0.85, 0.95};
    std::vector<double> absence  = {0.1, 0.15, 0.2};

    EvalResult res = ModelEvaluation::evaluate(presence, absence);

    assert(near(res.auc, 1.0));
    assert(res.correlation > 0.0);
    assert(res.square_error < 0.1);
    assert(res.logloss > 0.0);
    assert(res.misclassification == 0.0);
    assert(near(res.prevalence, 0.5));
    assert(res.max_kappa >= 0.0);

    std::cout << "  Test 12 (full evaluate): OK\n";
}

int main() {
    std::cout << "Testing ModelEvaluation...\n";
    test_basic_stats();
    test_correlation();
    test_correlation_mismatch();
    test_auc_perfect();
    test_auc_random();
    test_auc_empty();
    test_auc_partial();
    test_logloss();
    test_square_error();
    test_misclassification();
    test_kappa();
    test_evaluate();
    std::cout << "All ModelEvaluation tests passed!\n";
    return 0;
}
