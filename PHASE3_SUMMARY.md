# Phase 3: Core Algorithm – FeaturedSpace, Model Training, and Lambda Optimization

## Summary

Phase 3 implements the **core MaxEnt algorithm**: the `FeaturedSpace` class, sequential
coordinate-ascent model training, lambda (weight) optimization, and an R interface for
training and prediction. This phase ports `density/FeaturedSpace.java` (789 lines) and the
relevant portions of `density/Sequential.java` (532 lines) to modern C++17.

## Files Created/Modified

### New Files

| File | Lines | Description |
|------|-------|-------------|
| `cpp/include/maxent/featured_space.hpp` | ~580 | `FeaturedSpace` class (header-only), `SampleInfo`, `Interval`, `TrainResult` |
| `cpp/src/featured_space.cpp` | 8 | Stub include file (implementation is header-only) |
| `cpp/tests/test_featured_space.cpp` | ~290 | 10 C++ unit tests |
| `R-package/src/rcpp_featured_space.cpp` | ~210 | Rcpp bindings for all FeaturedSpace operations |
| `R-package/R/featured_space.R` | ~180 | 8 user-friendly R wrapper functions with documentation |
| `R-package/tests/testthat/test-featured-space.R` | ~210 | 10 R testthat tests |
| `R-package/NAMESPACE` | ~55 | Full package namespace (all exported symbols) |
| `PHASE3_SUMMARY.md` | — | This file |

### Modified Files

| File | Changes |
|------|---------|
| `cpp/include/maxent/feature.hpp` | Added `expectation_`, `sample_expectation_`, `sample_deviation_` fields + accessors + `increase_lambda()` |
| `cpp/CMakeLists.txt` | Added `src/featured_space.cpp` to `MAXENT_SOURCES` |
| `cpp/tests/CMakeLists.txt` | Added `test_featured_space` executable and `FeaturedSpaceTest` |
| `R-package/DESCRIPTION` | Bumped version from 0.2.0 → 0.3.0 |
| `MIGRATION.md` | Phase 3 marked as ✅ Completed; directory structure updated |

## Key Design Decisions

### Header-only Implementation

Following the existing project pattern (all other headers — `feature.hpp`, `grid.hpp`,
`sample.hpp` — are fully self-contained), `featured_space.hpp` contains the complete
inline implementation. The `cpp/src/featured_space.cpp` stub exists only for CMake
consistency.

### Training Algorithm (from `Sequential.java`)

The training loop implements **sequential coordinate ascent** exactly as in the Java source:

1. **`set_sample_expectations()`**: For each feature, compute the mean over occurrence
   samples (= `sample_expectation`) and the regularization term
   `beta * std / sqrt(n)` (= `sample_deviation`). Mirrors
   `FeaturedSpace.setSampleExpectations()` in Java.

2. **`good_alpha()`**: Compute the optimal lambda increment for one feature using
   the closed-form formula from Sequential.java (`goodAlpha()`):
   ```
   alpha = log((N1 - beta) * W0 / ((N0 + beta) * W1))   [positive direction]
   alpha = log((N1 + beta) * W0 / ((N0 - beta) * W1))   [negative direction]
   ```
   where `N1` = sample expectation, `W1` = model expectation, `beta` = `sample_deviation`.

3. **`reduce_alpha()`**: Scale down alpha in early iterations (iter<10: /50,
   iter<20: /10, iter<50: /3) for stability. Direct port of `Sequential.reduceAlpha()`.

4. **Convergence check** every 20 iterations: stop if loss improvement < threshold.

### Feature Expectations

Feature model expectations (weighted means under the current distribution) are updated
automatically inside `set_density()`, which is called after every `increase_lambda()`.
This matches the Java design where `FeaturedSpace.setDensity(Feature[] toUpdate)` updates
a subset of feature expectations.

### Lambda File Format

The `.lambdas` file format matches the original Java Maxent format exactly:
```
featureName, lambda, min, max
linearPredictorNormalizer, <value>
densityNormalizer, <value>
numBackgroundPoints, <value>
entropy, <value>
```

## C++ API

```cpp
// Construct
FeaturedSpace fs(num_points, sample_indices, features);

// Train
TrainResult result = fs.train(500, 1e-5, 1.0);

// Query
auto weights = fs.get_weights();      // normalized distribution
double H      = fs.get_entropy();     // Shannon entropy
double loss   = fs.get_loss();        // negative log-likelihood

// Predict
auto scores = fs.predict(feature_matrix);  // raw Gibbs scores

// Persistence
fs.write_lambdas("model.lambdas");
fs.read_lambdas("model.lambdas");
```

## R API

```r
# Build features
env <- seq(0, 1, length.out = 100)
f1  <- maxent_linear_feature(env, "temperature")

# Create FeaturedSpace
fs  <- maxent_featured_space(100L, 90:99, list(f1))

# Train
result <- maxent_fit(fs, max_iter = 500L, convergence = 1e-5)

# Query
w    <- maxent_model_weights(fs)
H    <- maxent_model_entropy(fs)
loss <- maxent_model_loss(fs)

# Predict
preds <- maxent_predict_model(fs, matrix(runif(5), ncol = 1))

# Save/load
maxent_save_lambdas(fs, "model.lambdas")
maxent_load_lambdas(fs2, "model.lambdas")
```

## Test Results

### C++ Tests (4 total, 100% pass)

| Test | Description |
|------|-------------|
| GridTest | Grid dimension and raster operations |
| SampleTest | Sample point management |
| FeatureTest | All 6 feature types + generator |
| **FeaturedSpaceTest** | **10 new tests (construction, density, training, I/O, prediction)** |

### R Tests

10 new testthat tests in `test-featured-space.R` covering:
- FeaturedSpace creation
- Initial uniform distribution
- Weight normalization (sum to 1)
- Entropy bounds
- Training reduces loss
- Prediction output
- Lambda file round-trip
- Multi-feature training

## Mathematics

The MaxEnt probability distribution is:

```
p(x) = exp(sum_j lambda_j * f_j(x)) / Z
```

where `Z = sum_i exp(sum_j lambda_j * f_j(x_i))` is the partition function.

Training maximizes the regularized log-likelihood:

```
L(lambda) = sum_j lambda_j * E_samples[f_j] - log(Z) - sum_j |lambda_j| * beta_j
```

using sequential coordinate ascent with the `goodAlpha` closed-form step.

## References

- Phillips, S.J., Anderson, R.P., Schapire, R.E. (2006). Maximum entropy modeling of
  species geographic distributions. *Ecological Modelling*, 190(3-4), 231-259.
- Dudík, M., Phillips, S.J., Schapire, R.E. (2007). Maximum entropy density estimation
  with generalized regularization. *JMLR*, 8, 1217-1260.
- Original Java source: `density/FeaturedSpace.java`, `density/Sequential.java`
