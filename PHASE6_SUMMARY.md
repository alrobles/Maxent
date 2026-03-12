# Phase 6: Model Interpretation & Diagnostics

## Summary

Phase 6 implements **model interpretation and diagnostic tools** for Maxent
models. This phase ports variable importance analysis from
`density/tools/PermutationImportance.java`, response curve data generation
from `density/ResponsePlot.java`, clamping logic from `density/Project.java`,
and novelty detection (MESS) from `density/tools/Novel.java` to modern
C++17 with corresponding R bindings.

## Files Created/Modified

### New Files

| File | Lines | Description |
|------|-------|-------------|
| `cpp/include/maxent/variable_importance.hpp` | ~220 | `VariableImportance`: permutation importance & percent contribution |
| `cpp/include/maxent/response_curve.hpp` | ~210 | `ResponseCurve`: marginal & fixed response curve generation |
| `cpp/include/maxent/clamping.hpp` | ~190 | `Clamping`: clamp grids to training range, clamping indicator grid |
| `cpp/include/maxent/novelty.hpp` | ~280 | `Novelty`: MESS analysis, MoD grid, simplified range-based MESS |
| `cpp/src/variable_importance.cpp` | 2 | Stub include |
| `cpp/src/response_curve.cpp` | 2 | Stub include |
| `cpp/src/clamping.cpp` | 2 | Stub include |
| `cpp/src/novelty.cpp` | 2 | Stub include |
| `cpp/tests/test_variable_importance.cpp` | ~160 | 6 C++ unit tests for VariableImportance |
| `cpp/tests/test_response_curve.cpp` | ~170 | 7 C++ unit tests for ResponseCurve |
| `cpp/tests/test_clamping.cpp` | ~170 | 8 C++ unit tests for Clamping |
| `cpp/tests/test_novelty.cpp` | ~160 | 8 C++ unit tests for Novelty |
| `R-package/src/rcpp_model_diagnostics.cpp` | ~300 | Rcpp bindings for all Phase 6 components |
| `R-package/R/model_diagnostics.R` | ~200 | 8 user-friendly R wrapper functions |
| `R-package/tests/testthat/test-model-diagnostics.R` | ~140 | 6 R testthat tests |
| `PHASE6_SUMMARY.md` | — | This file |

### Modified Files

| File | Changes |
|------|---------|
| `cpp/CMakeLists.txt` | Added 4 new source files to `MAXENT_SOURCES` |
| `cpp/tests/CMakeLists.txt` | Added 4 new test executables and `add_test()` calls |
| `R-package/DESCRIPTION` | Bumped version from 0.5.0 → 0.6.0 |
| `R-package/NAMESPACE` | Added 16 new exported functions |
| `MIGRATION.md` | Phase 6 marked as ✅ Completed; directory structure updated |
| `README.md` | Updated phase table with Phase 6 status |
| `QUICKSTART.md` | Added Phase 6 examples |

## Key Design Decisions

### Header-only Implementation

Following the existing project pattern, all new headers are fully
self-contained with inline implementations. The `cpp/src/*.cpp` stubs
exist only for CMake consistency.

### Permutation Importance Algorithm

Ported from `PermutationImportance.java`:

1. Compute baseline AUC from unpermuted model predictions
2. For each variable k:
   a. Randomly permute variable k's values across sample locations
   b. Re-predict with the permuted data
   c. Compute permuted AUC
   d. Importance = max(0, baseline_AUC − permuted_AUC)
3. Normalise all importances to sum to 100%

Uses `std::mt19937` with configurable seed for reproducibility.

### Percent Contribution

Sums the absolute lambda values (`|λ_j|`) for all features derived from
each base environmental variable, then normalises to 100%. Features are
matched to variables by checking if the feature name starts with the
variable name (e.g., feature `"temp_linear"` maps to variable `"temp"`).

### Response Curves

Two modes are provided:

| Mode | Description |
|------|-------------|
| **Marginal** | Vary target variable min→max, hold others at grid mean |
| **Fixed** | Vary target variable, hold others at user-supplied values |

Predictions are cloglog-transformed: `cloglog(x) = 1 − exp(−x)` to
produce values in [0, 1].

### Clamping

Ported from the clamping logic in `Project.java`:

- For each cell and variable: clamp value to [training_min, training_max]
- The **clamping grid** records the total absolute clamping magnitude
  per cell (sum across all variables)
- A `compute_ranges()` helper extracts min/max from grids

### MESS (Multivariate Environmental Similarity Surface)

Ported from `Novel.java`. Two versions:

| Version | Input | Algorithm |
|---------|-------|-----------|
| **Full MESS** | Reference point values | Binary search for percentile; similarity = min(f, 100−f) × 2 |
| **Range MESS** | Min/max only | Simplified check against training range |

For each cell:
- MESS = minimum similarity across all variables
- MoD = index (1-based) of the variable with the lowest similarity

Negative MESS values indicate **novel environments** (extrapolation).

## C++ API

```cpp
using namespace maxent;

// --- Variable Importance ---
auto perm = VariableImportance::permutation_importance(
    model, env_grids, names, pres_rows, pres_cols, abs_rows, abs_cols);
// perm[k].name, perm[k].permutation_importance

auto contrib = VariableImportance::percent_contribution(model, names);
// contrib[k].name, contrib[k].contribution

// --- Response Curves ---
auto curve = ResponseCurve::marginal(model, grids, names, var_index, 100);
// curve[i].value, curve[i].prediction

auto curve2 = ResponseCurve::marginal_fixed(
    model, fixed_vals, names, var_index, var_min, var_max, 100);

// --- Clamping ---
auto result = Clamping::clamp(env_grids, var_mins, var_maxs);
// result.clamped_grids[k], result.clamp_grid

std::vector<double> mins, maxs;
Clamping::compute_ranges(env_grids, mins, maxs);

// --- MESS ---
auto mess = Novelty::mess(env_grids, reference_values, names);
// mess.mess_grid, mess.mod_grid

auto mess2 = Novelty::mess_range(env_grids, var_mins, var_maxs);
```

## R API

```r
# --- Variable Importance ---
imp <- maxent_permutation_importance(model, list(g1, g2),
         c("temp", "precip"), pres_rows, pres_cols, abs_rows, abs_cols)
imp$name                     # "temp", "precip"
imp$permutation_importance   # e.g. 72.3, 27.7

contrib <- maxent_percent_contribution(model, c("temp", "precip"))
contrib$contribution         # e.g. 85.1, 14.9

# --- Response Curves ---
curve <- maxent_response_curve(model, list(g1, g2),
           c("temp", "precip"), var_index = 0, n_steps = 100)
plot(curve$value, curve$prediction, type = "l",
     xlab = "Temperature", ylab = "Suitability")

curve2 <- maxent_response_curve_fixed(model, c(15, 100),
            c("temp", "precip"), 0, 0, 30, 50)

# --- Clamping ---
result <- maxent_clamp(list(g1, g2), c(0, 50), c(30, 200))
clamped <- result$clamped_grids
clamp_mat <- maxent_grid_to_matrix(result$clamp_grid)

ranges <- maxent_variable_ranges(list(g1, g2))
ranges$min   # [0.0, 50.0]
ranges$max   # [30.0, 200.0]

# --- MESS ---
result <- maxent_mess(list(g1, g2),
            list(train_temp, train_precip),
            c("temp", "precip"))
mess_mat <- maxent_grid_to_matrix(result$mess_grid)
mod_mat  <- maxent_grid_to_matrix(result$mod_grid)

result2 <- maxent_mess_range(list(g1, g2), c(0, 50), c(30, 200))
```

## Test Results

### C++ Tests (14 suites, 100% pass)

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
| ModelEvaluationTest | existing | AUC, kappa, correlation, logloss, MSE |
| ProjectionTest | existing | Raw/cloglog/logistic projection |
| **VariableImportanceTest** | **6 new** | Permutation importance, percent contribution, ranking |
| **ResponseCurveTest** | **7 new** | Marginal/fixed curves, range, predictions, errors |
| **ClampingTest** | **8 new** | Below/above/multi-var clamping, NODATA, ranges |
| **NoveltyTest** | **8 new** | MESS positive/negative, MoD, range MESS, errors |

### R Tests

6 testthat tests in `test-model-diagnostics.R` covering:
- Percent contribution structure and sum
- Clamping correctness (below/within/above range)
- Variable ranges computation
- MESS range novelty detection
- MESS with full reference values
- Response curve structure and prediction range

## References

- Original Java source: `density/tools/PermutationImportance.java`,
  `density/ResponsePlot.java`, `density/Project.java`,
  `density/tools/Novel.java`
- Elith, J., Kearney, M., & Phillips, S. (2010). The art of modelling
  range-shifting species. *Methods in Ecology and Evolution*, 1(4), 330-342.
- Phillips, S.J., Anderson, R.P., Schapire, R.E. (2006). Maximum entropy
  modeling of species geographic distributions. *Ecological Modelling*,
  190(3-4), 231-259.
