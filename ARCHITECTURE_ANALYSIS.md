# Maxent Java Architecture Analysis

> **Purpose:** Reference document for creating a side-by-side numerical validation test suite comparing the Java Maxent implementation with a C++ reimplementation. All citations reference files in the `density/` package.

---

## Table of Contents

1. [Entry Points](#1-entry-points)
2. [Model Fitting Pipeline](#2-model-fitting-pipeline)
3. [Core Optimizer — `Sequential.java`](#3-core-optimizer--sequentialjava)
4. [Feature Architecture](#4-feature-architecture)
5. [Linear Features](#5-linear-features)
6. [Density Normalization](#6-density-normalization)
7. [Sample Expectations and Deviations](#7-sample-expectations-and-deviations)
8. [Regularization (Beta) Values](#8-regularization-beta-values)
9. [Cloglog Output Transformation](#9-cloglog-output-transformation)
10. [Data I/O](#10-data-io)
11. [Key Mathematical Functions — Summary Table](#11-key-mathematical-functions--summary-table)

---

## 1. Entry Points

### `density/MaxEnt.java` (lines 30–79)

The application entry point. In non-GUI mode:

```java
Params params = new Params();
params.readFromArgs(args);
Runner runner = new Runner(params);
runner.start();
runner.end();
```

### `density/Runner.java` — `MaxentRunResults` (lines 86–98)

Inner class wrapping the result of a single species model run:

```java
public static class MaxentRunResults {
    double gain, time;
    int iterations;
    FeaturedSpace X;
    String[] featureTypes;
}
```

`gain` is the regularized training gain reported at the end of each run.

---

## 2. Model Fitting Pipeline

```
Runner.start()
 ├── initializeGrids()              → GridSet (Extractor or GridSetFromFile)
 ├── makeFeatures(baseFeatures)     → Feature[] (linear, square, product, threshold, hinge)
 ├── maxentRun(features, ss, testss)
 │    ├── autoSetBeta()             → sets Feature.beta (regularization constants)
 │    ├── FeaturedSpace X = new FeaturedSpace(ss, features, params)
 │    │    ├── setSampleExpectations()  → feature means + deviations over presence samples
 │    │    ├── setLinearPredictor()     → LP[i] = Σ λ_j · f_j(i)
 │    │    └── setDensity()             → density[i] = biasDist(i) · exp(LP[i] − LPN)
 │    ├── Sequential.run()          → iterative coordinate ascent
 │    └── gain = log(bNumPoints) − Sequential.run()
 ├── X.writeFeatureWeights(lambdafile)  → writes .lambdas file
 └── Project.doProject()            → grid output (raw / logistic / cloglog / cumulative)
```

### `Runner.maxentRun()` (lines 1669–1720)

```java
MaxentRunResults maxentRun(Feature[] features, Sample[] ss, Sample[] testss) {
    autoSetBeta(features, ss.length);          // set per-feature beta
    // ... initialize lambdas to 0, set active flags ...
    FeaturedSpace X = new FeaturedSpace(ss, features, params);
    Sequential alg = new Sequential(X, params);
    double gain = Math.log(X.bNumPoints()) - alg.run();
    return new MaxentRunResults(gain, alg.iteration, X, alg.getTime(), types);
}
```

---

## 3. Core Optimizer — `Sequential.java`

The optimizer implements **Sequential Coordinate Ascent** with L1 regularization (analogous to the boosting / update step in Phillips et al. 2006).

### Main Loop (`run()`, lines 497–531)

```java
for (iteration = 0; iteration < maxIterations; iteration++) {
    if (iteration % parallelUpdateFrequency == 0)
        newLoss = doParallelUpdate();    // Newton step over subspace of changed features
    else {
        Feature f = getBestFeature();    // feature with lowest deltaLossBound
        newLoss = doSequentialUpdate(f); // update λ_f via Newton step
    }
    if (terminationTest(newLoss)) break;
}
return getLoss();  // returned to Runner, subtracted from log(Z_bg) to get gain
```

Convergence is tested every `convergenceTestFrequency = 20` iterations:
`previousLoss - newLoss < terminationThreshold` (default `1e-5`).

### Loss Function (`getLoss()`, line 62)

```java
double getLoss() { return X.getLoss() + reg; }
```

- **`X.getLoss()`** = negative regularized log-likelihood = `getN1() + log(densityNormalizer)`
  - **`getN1()`** = `-Σ_j λ_j · π̂_j + LPN`  (`FeaturedSpace.java` lines 200–205)
  - `π̂_j` = `sampleExpectation` = empirical mean of feature j over presence samples
  - `LPN` = `linearPredictorNormalizer` = `max_i LP[i]`
- **`reg`** = L1 regularization = `Σ_j |λ_j| · σ_j` where `σ_j` = `sampleDeviation`

The **gain** returned to the caller:

```
gain = log(numBackgroundPoints) − loss
     = log(Z_bg) + Σ_j λ_j π̂_j − log(Z_q) − Σ_j |λ_j| σ_j
```

### Feature Selection (`getBestFeature()`, lines 66–102)

Evaluates `deltaLossBound(h)` for every active feature and all candidate threshold/hinge positions. Selects the feature with the **lowest (most negative) bound**.

### Loss Decrease Bound (`deltaLossBound()`, lines 325–342)

Tight analytical bound on the loss decrease from a single λ update:

```
bound = −N1·α + log(W0 + W1·exp(α)) + β·(|λ+α| − |λ|)
```

where:
- `N1` = `h.sampleExpectation` (empirical feature mean over presence)
- `W1` = `h.expectation` = `E_q[h]` (model's expected feature value over background)
- `W0 = 1 − W1`, `N0 = 1 − N1`
- `α` = `goodAlpha(h)` — closed-form step
- `β` = `h.sampleDeviation` (per-feature regularization strength)

### Gradient (`deriv()`, lines 144–160)

Subgradient of the regularized loss with respect to λ_h:

```java
double unRegDeriv = W1 - N1;   // model expectation minus sample expectation
if (lambda > 0)  return unRegDeriv + beta;
if (lambda < 0)  return unRegDeriv - beta;
else {
    if (unRegDeriv + beta > 0)  return unRegDeriv + beta;
    if (unRegDeriv - beta < 0)  return unRegDeriv - beta;
    return 0.0;   // already optimal for this feature
}
```

### Newton Step (`newtonStep(h)`, lines 223–239)

```
stepSize = −deriv(h) / Var_q[h]
Var_q[h] = E_q[h²] − (E_q[h])²
```

Sign check ensures λ + stepSize does not overshoot zero (prevents sign reversal).

### Closed-Form Step (`goodAlpha()`, lines 294–317)

For binary features (and as initial estimate for continuous ones):

```java
if (N1 - beta > eps)
    alpha = log((N1 - beta) * W0 / ((N0 + beta) * W1));   // try positive direction
else if (N0 - beta > eps)
    alpha = log((N1 + beta) * W0 / ((N0 - beta) * W1));   // try negative direction
else
    alpha = -lambda;   // zero out
```

### Step Size Damping (`reduceAlpha()`, lines 467–472)

Early iterations use strongly damped steps to stabilize convergence:

| Iteration | Damping factor |
|-----------|---------------|
| < 10 | ÷ 50 |
| < 20 | ÷ 10 |
| < 50 | ÷ 3 |
| ≥ 50 | × 1 (full step) |

---

## 4. Feature Architecture

### Class Hierarchy (`Feature.java`, lines 29–152)

```
Feature (abstract)
 ├── LayerFeature         — raw raster data (L_CONT, L_CAT, L_F_BIAS_OUT, L_DEBIAS_AVG)
 ├── LinearFeature        — identity wrapper for continuous features
 ├── ScaledFeature        — (x − min) / (max − min) normalization
 ├── CachedFeature        — memoized evaluation
 ├── CachedScaledFeature  — memoized scaled evaluation
 ├── ClampedFeature       — clamped to [min, max]
 ├── SquareFeature        — x²
 ├── PolyhedralFeature    — (x − x²) / 4
 ├── ProductFeature       — x · y
 ├── BinaryFeature        — categorical one-hot
 ├── ThresholdFeature     — 1(x ≥ t)
 ├── HingeFeature         — max(0, (x − min) / range)
 ├── ThrGeneratorFeature  — lazy generator for threshold features
 ├── HingeGeneratorFeature— lazy generator for hinge features
 ├── ConstFeature         — constant 1.0
 └── FeatureWithSamples   — augments background with presence locations
```

### Feature Type Constants (`Feature.java`, lines 30–31)

```java
BINARY=0, LINEAR=1, SQUARE=2, PRODUCT=3, THR_GEN=4, THR=5,
L_CONT=6, L_CAT=7, L_F_BIAS_OUT=8, L_DEBIAS_AVG=9,
F_W_SAMPLES=10, HINGE_GEN=11, HINGE=12
```

### Per-Feature Fields (`Feature.java`, lines 81–84)

```java
double lambda;           // current weight
double sampleExpectation;// π̂_j: empirical mean over presence samples
double sampleDeviation;  // σ_j: regularization width
double expectation;      // E_q[f_j]: model expectation over background
double beta;             // raw regularization constant (before sample-size adjustment)
```

### `makeFeatures()` — Feature Construction (`Runner.java`, lines 2145–2218)

```java
// 1. Categorical → one-hot binary features
BinaryFeature.makeAll(f[i], names[i])

// 2. Continuous → always add LinearFeature (even if "linear" is off — needed for clamping)
features.add(new LinearFeature(cont[i], cont[i].name))

// 3. If quadratic enabled:
features.add(new SquareFeature(cont[i], cont[i].name))
if (polyhedral) features.add(new PolyhedralFeature(cont[i], cont[i].name))

// 4. If product enabled (pairwise):
features.add(new ProductFeature(cont[i], cont[j], ...))

// 5. If threshold enabled (lazy, positions selected during optimization):
features.add(new ThrGeneratorFeature(cont[i], cont[i].name))

// 6. If hinge enabled (forward + reverse):
features.add(new HingeGeneratorFeature(cont[i], ...))
features.add(new HingeGeneratorFeature(revFeature(cont[i]), name+"__rev"))

// 7. All wrapped with ScaledFeature (or CachedScaledFeature if cacheFeatures=true)
```

---

## 5. Linear Features

### `LinearFeature.java` (lines 26–38)

A pure delegation wrapper — no arithmetic transformation:

```java
class LinearFeature extends Feature {
    Feature f;
    public double eval(int p)    { return f.eval(p); }
    public double eval(Sample s) { return f.eval(s); }
}
```

Linear features are **always created** for every continuous variable, even when the `linear` feature type is disabled, because they are required for clamping during projection (`Runner.java` lines 303–307, `FeaturedSpace.java` lines 304–307).

### `ScaledFeature.java` (lines 26–65)

Normalizes continuous features to [0, 1]:

```java
public double eval(int p)    { return (f.eval(p) - min) / scale; }  // scale = max − min
public double eval(Sample s) { return (f.eval(s) - min) / scale; }
```

`min` and `max` are computed over all background points at construction time. Written to `.lambdas` file as the third and fourth columns, enabling identical scaling at projection time.

### Hinge Features (`HingeFeature.java`, lines 28–44)

Forward hinge (ascending ramp):

```java
double eval(double d) { return (d > min) ? (d - min) / range : 0.0; }
```

Reverse hinge: the base feature is negated (`revFeature()`, Runner.java lines 2220–2226), effectively creating a descending ramp. In the `.lambdas` file, reverse hinges are stored with `` ` `` prefix and negated min/max.

---

## 6. Density Normalization

### Core Density Computation (`FeaturedSpace.setDensity()`, lines 688–703)

```java
densityNormalizer = 0.0;
for (int i = 0; i < numPoints; i++) {
    double d = biasDist.eval(i) * Math.exp(linearPredictor[i] - linearPredictorNormalizer);
    density[i] = d;
    densityNormalizer += d;
    for each feature toUpdate:
        sum[j] += d * feature[j].eval(i);
}
for each feature toUpdate:
    feature.expectation = sum[j] / densityNormalizer;
```

The **`linearPredictorNormalizer`** (LPN) is the maximum LP value across all background points, preventing `exp()` overflow:

```java
// setLinearPredictorNormalizer() — FeaturedSpace.java lines 639–644
linearPredictorNormalizer = linearPredictor[0];
for (int i = 1; i < numPoints; i++)
    if (linearPredictor[i] > linearPredictorNormalizer)
        linearPredictorNormalizer = linearPredictor[i];
```

### Incremental Update (`increaseLambda()`, lines 655–665)

Used at each optimizer step to avoid a full O(N·F) recomputation:

```java
h.lambda += alpha;
for (int i = 0; i < numPoints; i++) {
    linearPredictor[i] += alpha * h.eval(i);
    if (linearPredictor[i] > linearPredictorNormalizer)
        linearPredictorNormalizer = linearPredictor[i];
}
setDensity(updateFeatures);  // recomputes density[], densityNormalizer, and feature expectations
```

### Normalized Density / Raw Output

```java
// getDensity(i) / densityNormalizer  →  raw output q(i) in [0, 1]
double[] getWeights() {
    double[] result = new double[numPoints];
    for (int i = 0; i < numPoints; i++)
        result[i] = getDensity(i) / densityNormalizer;
    return result;
}
```

### Entropy (`getEntropy()`, lines 87–96)

```java
entropy = 0.0;
for (int i = 0; i < numPointsForNormalizer; i++) {
    double d = getDensity(i) / densityNormalizer;  // q(i)
    if (d > 0) entropy += -d * Math.log(d);
}
```

Written to `.lambdas` file. Used as `exp(entropy)` ≈ effective number of background cells in output transformations.

---

## 7. Sample Expectations and Deviations

### `setSampleExpectations()` (`FeaturedSpace.java`, lines 469–493)

Called at construction from `setBiasDiv()`. For each feature:

```java
SampleInfo fInfo = getDividedSampleInfo(f, biasDiv);        // stats over presence samples
SampleInfo biasInfo = getDividedSampleInfo(const1, biasDiv); // stats of bias
Interval fInterval = new Interval(fInfo, biasInfo, f.beta);
f.sampleExpectation = fInterval.getMid();   // = avg ± beta*std/sqrt(n) midpoint
f.sampleDeviation   = fInterval.getDev();   // = beta*std/sqrt(n) half-width
```

Floor on deviation (`FeaturedSpace.java` lines 480–486):
```java
if (f.sampleDeviation < minDeviation) {    // minDeviation = 0.001 * betaMultiplier
    if (f.type() == BINARY && expectation == 1 && m > 0)
        f.sampleDeviation = 1 / (2.0 * m);  // special case: all-1 binary
    else
        f.sampleDeviation = minDeviation;
}
```

### `getDividedSampleInfo()` (`FeaturedSpace.java`, lines 429–465)

Computes mean and standard deviation of `f(s) / bias(s)` over presence samples:

```java
avg = Σ_s [f(s)/bias(s)] / cnt
std = sqrt((Σ_s [f(s)/bias(s)]² - cnt·avg²) / (cnt-1))
```

Edge cases:
- `cnt == 0`: feature deactivated
- `cnt == 1`: `std = 0.5 * (max - min)`
- `std > 0.5*(max-min)`: clamped to `0.5*(max-min)`

### `Interval` class (`FeaturedSpace.java`, lines 391–427)

Constructs the tolerance interval used for regularization:

```java
Interval(SampleInfo f, double beta) {
    low  = f.avg - beta / Math.sqrt(f.sample_cnt) * f.std;
    high = f.avg + beta / Math.sqrt(f.sample_cnt) * f.std;
}
// getMid() = (low + high) / 2  →  sampleExpectation
// getDev() = (high - low) / 2  →  sampleDeviation
```

---

## 8. Regularization (Beta) Values

### `autoSetBeta()` (`Runner.java`, lines 2253–2309)

Interpolates beta values from a lookup table indexed by number of presence samples:

| Feature type | n=0 | n=10 | n=17 | n=30 | n=100 |
|---|---|---|---|---|---|
| **LQP** (with product) | 2.6 | 1.6 | 0.9 | 0.55 | 0.05 |
| **LQ** (quadratic, no product) | 1.3 | 0.8 | 0.5 | 0.25 | 0.05 |
| **L** (linear only) | — | 1.0 | — | 0.2 | 0.05 |
| **threshold** | 2.0 | — | — | — | 1.0 |
| **hinge** | 0.5 (fixed) | | | | |
| **categorical** | 0.65 | 0.5 | 0.25 | — | — |

All multiplied by `betaMultiplier` (user parameter, default 1.0). User can override individual types via parameters `beta_lqp`, `beta_threshold`, `beta_hinge`, `beta_categorical`.

Per-feature betas are assigned at `autoSetBeta()` lines 2291–2308:

```java
if (feature instanceof BinaryFeature)          feature.setBeta(beta_cat * betaMultiplier());
else if (feature instanceof ThrGeneratorFeature) feature.setBeta(beta_thr * betaMultiplier());
else if (feature instanceof HingeGeneratorFeature) feature.setBeta(beta_hge * betaMultiplier());
else feature.setBeta(beta_lqp * betaMultiplier());  // linear, square, product
```

### L1 Regularization (`FeaturedSpace.getL1reg()`, lines 189–193)

```java
double getL1reg() {
    double result = 0.0;
    for (int j = 0; j < numFeatures; j++)
        result += Math.abs(features[j].lambda) * features[j].getSampleDeviation();
    return result;
}
```

Note: `sampleDeviation` already incorporates the beta (via `Interval` construction).

---

## 9. Cloglog Output Transformation

### `Project.cloglog()` (`Project.java`, lines 331–333)

```java
static double cloglog(double raw, double entropy) {
    return 1 - Math.exp(-raw * Math.exp(entropy));
}
```

- **`raw`** = `exp(LP − LPN) / densityNormalizer` — the normalized density, a value in [0, 1]
- **`entropy`** = `H[q]` = model entropy (written to `.lambdas` file)
- **`exp(entropy)`** ≈ effective number of background cells

This is applied in `Project.pred()` (line 280):

```java
if (params.cloglog())
    pred = cloglog(pred, entropy);
```

### `Project.logistic()` (`Project.java`, lines 327–330)

```java
static double logistic(double raw, double entropy, double dp) {
    double v = raw * Math.exp(entropy);
    return dp * v / ((1 - dp) + dp * v);
}
```

where `dp` = `defaultPrevalence` (default 0.5).

### Output Format Selection (`Project.pred()`, lines 271–291)

```java
double pred = Math.exp(sum - lPN) / dN;        // raw output in [0, 1]
if (entropy != -1 && params.logistic())
    pred = logistic(pred, entropy);
if (entropy != -1 && params.cloglog())
    pred = cloglog(pred, entropy);
if (cumulative())
    return interpolateCumulative(raw2cum, pred);
```

### Cumulative Output

The raw-to-cumulative mapping is built in `Runner.writeCumulativeIndex()` (lines 1782–1910) and stored in `<species>_omission.csv`. During projection, it is loaded and applied via binary-search interpolation (`Project.interpolateCumulative()`, lines 549–562).

---

## 10. Data I/O

### SWD (Samples With Data) Format

CSV files with mandatory columns: `Species, Longitude, Latitude` followed by environmental variable columns named to match selected layers.

Column indices (`SampleSet.java`, lines 36–38):
```java
speciesIndex = 0, xIndex = 1, yIndex = 2, firstEnvVar = 3
```

Reading is handled by `SampleSet2.java` which cross-references CSV header names against selected `Layer` names to detect SWD format.

### `GridSet` Hierarchy

| Class | Description |
|---|---|
| `Extractor.java` | Reads raster files (`.asc`, `.grd`, `.bil`, `.mxe`); samples random background; caches to `.mxe` |
| `GridSetFromFile.java` | Reads both background and samples from SWD CSV files |
| `GridIO.java` | Low-level raster format I/O |
| `LazyGrid.java` | Memory-mapped / lazy-loaded raster |

### Lambda File Format (`.lambdas`)

Written by `FeaturedSpace.writeWeights()` (lines 750–781). Plain CSV, one feature per line:

```
featureName, lambda, min, max
...
linearPredictorNormalizer, value
densityNormalizer, value
numBackgroundPoints, value
entropy, value
```

Feature name conventions:
- `varname` — linear feature
- `varname^2` — quadratic
- `varname1*varname2` — product
- `(varname=value)` — binary/categorical
- `(value<varname)` — threshold at value
- `'varname` — forward hinge (min, max in columns 3–4)
- `` `varname `` — reverse hinge (stored with negated min/max)

### Grid Output Formats

| Format | Transformation |
|---|---|
| `raw` | `exp(LP − LPN) / densityNormalizer` |
| `cumulative` | Percentile rank via `raw2cum` interpolation |
| `logistic` | `dp·v / ((1−dp) + dp·v)` where `v = raw·exp(H)` |
| `cloglog` | `1 − exp(−raw·exp(H))` |

---

## 11. Key Mathematical Functions — Summary Table

| Function | File | Lines | Formula |
|---|---|---|---|
| Linear predictor | `FeaturedSpace.java` | 716–725 | `LP[i] = Σ_j λ_j · f_j(i)` |
| LP normalizer | `FeaturedSpace.java` | 639–644 | `LPN = max_i LP[i]` |
| Density | `FeaturedSpace.java` | 688–703 | `density[i] = bias(i) · exp(LP[i] − LPN)` |
| Density normalizer | `FeaturedSpace.java` | 688–703 | `Z = Σ_i density[i]` |
| Feature expectation | `FeaturedSpace.java` | 688–703 | `E_q[f_j] = Σ_i density[i]·f_j(i) / Z` |
| Model entropy | `FeaturedSpace.java` | 87–96 | `H = −Σ_i q_i ln(q_i)`, `q_i = density[i]/Z` |
| `getN1` | `FeaturedSpace.java` | 200–205 | `−Σ_j λ_j π̂_j + LPN` |
| Loss | `FeaturedSpace.java` | 185–187 | `L = getN1() + log(Z)` |
| L1 regularization | `FeaturedSpace.java` | 189–193 | `reg = Σ_j |λ_j| σ_j` |
| Gain | `Runner.java` | 1716 | `log(numBgPoints) − (L + reg)` |
| Sample mean (π̂) | `FeaturedSpace.java` | 429–464 | `avg(f(s)/bias(s))` over presence |
| Sample deviation (σ) | `FeaturedSpace.java` | 469–493 | `β·std/√n`, floored at 0.001 |
| Delta loss bound | `Sequential.java` | 325–342 | `−N1α + log(W0+W1·exp(α)) + β(|λ+α|−|λ|)` |
| Gradient (subgradient) | `Sequential.java` | 144–160 | `E_q[f] − π̂ ± β` |
| Newton step | `Sequential.java` | 223–239 | `−deriv(h) / Var_q[h]` |
| Variance of feature | `Sequential.java` | 225–228 | `Var_q[h] = E_q[h²] − (E_q[h])²` |
| Good alpha (closed-form) | `Sequential.java` | 294–317 | Log ratio of presence/background proportions |
| Scale feature | `ScaledFeature.java` | 56–57 | `(val − min) / (max − min)` |
| Forward hinge | `HingeFeature.java` | 40 | `max(0, (x − min) / range)` |
| Threshold feature | `ThresholdFeature.java` | — | `1(x ≥ t)` |
| Cloglog output | `Project.java` | 331–333 | `1 − exp(−raw · exp(H))` |
| Logistic output | `Project.java` | 327–330 | `dp·v / ((1−dp) + dp·v)`, `v = raw·exp(H)` |
| Raw output | `Project.java` | 271–275 | `exp(sum − LPN) / Z` |
| Cumulative interp. | `Project.java` | 549–562 | Binary-search linear interpolation on `raw2cum` |
| Prevalence | `FeaturedSpace.java` | 76–84 | `Σ_i occurrenceProb(q_i) / numPoints` |

---

## 12. Notes for C++ Validation Test Suite

Key numerical identities to verify:

1. **LP consistency**: After optimization, `Σ_j λ_j · f_j(i)` should match stored `linearPredictor[i]` exactly.

2. **Density normalization**: `Σ_i density[i] / densityNormalizer = 1.0` exactly.

3. **Feature constraint satisfaction**: At convergence, `|E_q[f_j] − π̂_j| ≤ σ_j` for all active features with `λ_j ≠ 0`.

4. **Entropy invariant**: `entropy` is computed **after** `setBiasDist(null)` (bias distribution removed), so it reflects the unbiased model.

5. **Cloglog identity**: `cloglog(raw, H) = 1 − exp(−raw · exp(H))`. With `raw ≈ 1/exp(H)` at a "typical" background point, `cloglog ≈ 1 − 1/e ≈ 0.632`.

6. **Gain formula**: `gain = log(numBackgroundPoints) − getLoss()` ≡ `Σ_j λ_j π̂_j − log(Z) − reg + log(N_bg) − LPN`.

7. **Beta interpolation**: Beta values are piecewise-linearly interpolated by sample count, then multiplied by `betaMultiplier`. The table in §8 above gives the breakpoints.

8. **LPN tracking**: `linearPredictorNormalizer` only ever **increases** during optimization (it tracks the running maximum of all LP values). It is never decreased.
