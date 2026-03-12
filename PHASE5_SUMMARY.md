# Phase 5: Model Evaluation – AUC, Metrics, Spatial Projection

## Summary

Phase 5 implements **model evaluation** and **spatial projection** for Maxent
models. This phase ports the evaluation metrics from `density/tools/Stats.java`
and `density/tools/Eval.java`, and the spatial projection from
`density/Project.java` to modern C++17 with corresponding R bindings.

## Files Created/Modified

### New Files

| File | Lines | Description |
|------|-------|-------------|
| `cpp/include/maxent/model_evaluation.hpp` | ~300 | `ModelEvaluation` class: AUC, kappa, correlation, logloss, etc. |
| `cpp/include/maxent/projection.hpp` | ~280 | `Projection` class: raw, cloglog, logistic output; sample extraction |
| `cpp/src/model_evaluation.cpp` | 2 | Stub include |
| `cpp/src/projection.cpp` | 2 | Stub include |
| `cpp/tests/test_model_evaluation.cpp` | ~180 | 12 C++ unit tests for ModelEvaluation |
| `cpp/tests/test_projection.cpp` | ~270 | 8 C++ unit tests for Projection |
| `R-package/src/rcpp_model_evaluation.cpp` | ~240 | Rcpp bindings for evaluation and projection |
| `R-package/R/model_evaluation.R` | ~180 | 10 user-friendly R wrapper functions |
| `R-package/tests/testthat/test-model-evaluation.R` | ~80 | 9 R testthat tests |
| `PHASE5_SUMMARY.md` | — | This file |

### Modified Files

| File | Changes |
|------|---------|
| `cpp/CMakeLists.txt` | Added 2 new source files to `MAXENT_SOURCES` |
| `cpp/tests/CMakeLists.txt` | Added 2 new test executables and `add_test()` calls |
| `R-package/DESCRIPTION` | Bumped version from 0.4.0 → 0.5.0 |
| `R-package/NAMESPACE` | Added 20 new exported functions |
| `MIGRATION.md` | Phase 5 marked as ✅ Completed; directory structure updated |
| `QUICKSTART.md` | Updated status, added Phase 5 evaluation examples |

## Key Design Decisions

### Header-only Implementation

Following the existing project pattern, all new headers (`model_evaluation.hpp`,
`projection.hpp`) are fully self-contained with inline implementations.
The `cpp/src/*.cpp` stubs exist only for CMake consistency.

### AUC Algorithm

The AUC implementation ports `Stats.auc()` from Java Maxent, which computes
the Wilcoxon–Mann–Whitney AUC statistic using an efficient O(n log n)
sort-and-sweep algorithm:

1. Sort both presence and absence prediction arrays
2. For each presence value, count how many absence values are strictly less
3. Handle ties between presence and absence values
4. Normalize by total comparisons: `auc = sum / (2 * n_pres * n_abs)`

As a side product, the algorithm also computes the maximum Cohen's Kappa
and its corresponding threshold.

### Evaluation Metrics

All metrics from `density/tools/Eval.java` are ported:

| Metric | Formula | Description |
|--------|---------|-------------|
| **AUC** | Wilcoxon rank-sum | Discrimination ability |
| **Max Kappa** | Cohen's Kappa at best threshold | Classification agreement |
| **Correlation** | Pearson r(pred, truth) | Linear association |
| **Log-loss** | -mean(log(pred)) | Cross-entropy |
| **Squared Error** | mean((truth - pred)²) | Calibration quality |
| **Misclassification** | Error rate at threshold 0.5 | Simple accuracy |
| **Prevalence** | n_presence / n_total | Data balance |

### Projection Outputs

Three projection output formats are supported, following Maxent conventions:

| Format | Formula | Range | Use Case |
|--------|---------|-------|----------|
| **Raw** | exp(Σ λⱼfⱼ - norm) | (0, ∞) | Internal model scores |
| **Cloglog** | 1 - exp(-raw) | [0, 1] | Recommended since v3.4 |
| **Logistic** | raw / (1 + raw) | [0, 1] | Traditional Maxent output |

## C++ API

```cpp
// --- Model Evaluation ---
using namespace maxent;

std::vector<double> presence = {0.9, 0.85, 0.95};
std::vector<double> absence  = {0.1, 0.15, 0.2};

// Single metrics
double auc = ModelEvaluation::auc(presence, absence);
double cor = ModelEvaluation::correlation(pred, truth);
double ll  = ModelEvaluation::logloss(presence, absence);
double se  = ModelEvaluation::square_error(presence, absence);
double mc  = ModelEvaluation::misclassification(presence, absence);

// Full evaluation
EvalResult res = ModelEvaluation::evaluate(presence, absence);
// res.auc, res.max_kappa, res.correlation, res.logloss, ...

// --- Projection ---
Grid<float> raw_grid = Projection::project_raw(model, env_grids, names);
Grid<float> cll_grid = Projection::project_cloglog(model, env_grids, names);
Grid<float> log_grid = Projection::project_logistic(model, env_grids, names);

// Extract predictions at sample locations
auto scores = Projection::extract_predictions(
    model, env_grids, names, rows, cols);
```

## R API

```r
# --- Model Evaluation ---
auc_res <- maxent_auc(presence, absence)
auc_res$auc           # 1.0
auc_res$max_kappa     # Best kappa

r   <- maxent_correlation(pred, truth)
ll  <- maxent_logloss(presence, absence)
se  <- maxent_square_error(presence, absence)
mc  <- maxent_misclassification(presence, absence)

# Full evaluation
res <- maxent_evaluate(presence, absence)
res$auc
res$correlation
res$logloss
res$square_error
res$misclassification
res$prevalence

# --- Projection ---
raw_grid <- maxent_project_raw(model, list(g1, g2), c("env1", "env2"))
cll_grid <- maxent_project_cloglog(model, list(g1, g2), c("env1", "env2"))
log_grid <- maxent_project_logistic(model, list(g1, g2), c("env1", "env2"))
mat <- maxent_grid_to_matrix(cll_grid)

# Extract predictions at sample locations
preds <- maxent_extract_predictions(model, list(g1, g2),
                                    c("env1", "env2"), rows, cols)
```

## Test Results

### C++ Tests (10 suites, 100% pass)

| Test | Count | Description |
|------|-------|-------------|
| GridTest | existing | Grid dimension and raster operations |
| SampleTest | existing | Sample point management |
| FeatureTest | existing | All 6 feature types + generator |
| FeaturedSpaceTest | existing | Training, prediction, lambda I/O |
| LayerTest | existing | Construction, type parsing, name extraction |
| CsvReaderTest | existing | Comma/semicolon, quotes, European mode |
| CsvWriterTest | existing | Write + read round-trip, quoting |
| GridIOTest | existing | ASC read/write, round-trip, errors |
| **ModelEvaluationTest** | **12 new** | AUC, kappa, correlation, logloss, MSE, misclassification |
| **ProjectionTest** | **8 new** | Raw/cloglog/logistic projection, NODATA, gradient, errors |

### R Tests

9 testthat tests in `test-model-evaluation.R` covering:
- AUC perfect separation (= 1.0)
- AUC no discrimination (= 0.5)
- AUC partial overlap (0.5 < AUC < 1.0)
- Pearson correlation (positive and negative)
- Log-loss (perfect = 0)
- Squared error (perfect = 0)
- Misclassification (0% and 100% error)
- Full evaluate with all metrics

## References

- Original Java source: `density/tools/Stats.java`, `density/tools/Eval.java`,
  `density/Project.java`
- Phillips, S.J., Anderson, R.P., Schapire, R.E. (2006). Maximum entropy modeling
  of species geographic distributions. *Ecological Modelling*, 190(3-4), 231-259.
- DeLong, E.R., DeLong, D.M., Clarke-Pearson, D.L. (1988). Comparing the areas
  under two or more correlated receiver operating characteristic curves.
  *Biometrics*, 44(3), 837-845.
