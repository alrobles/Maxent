# Phase 2 Summary: Feature Engineering

## Overview

Phase 2 completes the feature engineering layer of the Maxent C++ migration.
Features transform raw environmental variable values into predictors used by
the Maxent model. This phase ports `density/Feature.java`,
`density/LinearFeature.java`, `density/SquareFeature.java`,
`density/ProductFeature.java`, `density/ThresholdFeature.java`, and
`density/HingeFeature.java` from the original Java codebase to C++17, and
exposes them to R via Rcpp.

## New Files

| File | Lines | Description |
|------|------:|-------------|
| `cpp/include/maxent/feature.hpp` | ~430 | Header-only C++ feature class hierarchy |
| `cpp/tests/test_feature.cpp` | ~230 | C++ unit tests for all feature types |
| `R-package/src/rcpp_features.cpp` | ~230 | Rcpp bindings (XPtr-based) |
| `R-package/R/features.R` | ~230 | R wrapper functions (Roxygen2 documented) |
| `R-package/tests/testthat/test-features.R` | ~220 | R testthat test suite |
| `PHASE2_SUMMARY.md` | — | This file |

## Updated Files

| File | Change |
|------|--------|
| `cpp/tests/CMakeLists.txt` | Added `test_feature` executable and `FeatureTest` CTest entry |
| `MIGRATION.md` | Phase 2 marked ✅ Completed; directory structure updated; roadmap updated |

## C++ Class Hierarchy

All classes live in the `maxent` namespace in `cpp/include/maxent/feature.hpp`
(header-only — no `feature.cpp` is required).

```
Feature  (abstract)
├── LinearFeature
├── QuadraticFeature
├── ProductFeature
├── ThresholdFeature
└── HingeFeature  (forward and reverse)

FeatureConfig  (plain struct — configuration for FeatureGenerator)
FeatureGenerator  (static factory)
```

### Feature Evaluation Formulae

| Class | Formula |
|-------|---------|
| `LinearFeature` | `(v - min) / (max - min)` ; 0 when `min == max` |
| `QuadraticFeature` | `linear_val²` |
| `ProductFeature` | `norm(v1) × norm(v2)` |
| `ThresholdFeature` | `v > threshold ? 1 : 0` |
| `HingeFeature` (forward) | `v > min_knot ? (v - min_knot) / range : 0` |
| `HingeFeature` (reverse) | `v < max_knot ? (max_knot - v) / range : 0` |

## R API Reference

### Feature Constructors

```r
maxent_linear_feature(values, name, min_val = NULL, max_val = NULL)
maxent_quadratic_feature(values, name, min_val = NULL, max_val = NULL)
maxent_product_feature(values1, values2, name,
                       min1 = NULL, max1 = NULL,
                       min2 = NULL, max2 = NULL)
maxent_threshold_feature(values, name, threshold)
maxent_hinge_feature(values, name, min_knot, max_knot, reverse = FALSE)
```

All constructors return an external pointer (`XPtr<Feature>`) to the C++ object.
When `min_val`/`max_val` (or `min*`/`max*`) are `NULL`, they are automatically
computed from the supplied `values` vector.

### Feature Utilities

```r
maxent_feature_eval(feature, index)   # evaluate at 1-based R index
maxent_feature_info(feature)          # returns list(name, type, lambda, min, max, size)
maxent_generate_features(data, types, n_thresholds, n_hinges)
```

`maxent_generate_features()` accepts a named list of numeric vectors and returns
a named list of feature external pointers.

## Building and Testing

### C++ Tests

```bash
cd cpp
mkdir -p build && cd build
cmake ..
make test_feature
./tests/test_feature
# or via CTest:
ctest --verbose -R FeatureTest
```

### R Tests

```r
devtools::load_all("R-package")
devtools::test("R-package", filter = "features")
```

## Design Notes

- **Header-only**: `feature.hpp` is fully self-contained; no `.cpp` file is needed
  because the feature classes are not templated on user-configurable types and the
  additional compilation overhead is acceptable.
- **`shared_ptr` data ownership**: Multiple features can share the same underlying
  data vector without copying. This mirrors the Java design where features hold a
  reference to the parent `Feature` object.
- **`FeatureConfig` decoupled from `FeatureGenerator`**: The config struct is
  defined outside the factory class to avoid a GCC restriction on default member
  initializers in nested structs used as default function arguments.
- **1-based R indexing**: `maxent_feature_eval(f, index)` subtracts 1 before
  calling into C++ so R users can use natural 1-based indices.
