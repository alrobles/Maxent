# Maxent Java to C++ Migration Guide

## Overview

This document describes the ongoing migration of Maxent from Java to C++ with R bindings using Rcpp. The migration follows an incremental approach, starting with core data structures and progressively adding functionality.

## Current Status

### Phase 1: Foundation & Core Data Structures ✅ (Completed)

**C++ Core:**
- ✅ `GridDimension` - Spatial dimensions and georeference
- ✅ `Sample` - Species occurrence points
- ✅ `Grid<T>` - Template-based raster grid (supports float, double, int, etc.)
- ✅ Basic CMake build system
- ✅ C++ unit tests

**R Package (maxentcpp):**
- ✅ Rcpp bindings for core classes
- ✅ R wrapper functions
- ✅ Package structure (DESCRIPTION, NAMESPACE)
- ✅ Unit tests with testthat
- ✅ Documentation

### Phase 2: Feature Engineering (In Progress)

**Next priorities:**
- [ ] `Feature` abstract base class
- [ ] `LinearFeature`, `QuadraticFeature`
- [ ] `ProductFeature`, `ThresholdFeature`, `HingeFeature`
- [ ] `FeatureGenerator` for automatic feature creation
- [ ] R bindings for features

### Phase 3: Core Algorithm (Planned)

- [ ] `FeaturedSpace` - Maximum entropy optimization
- [ ] Model training functionality
- [ ] Lambda (weight) optimization using L-BFGS
- [ ] R interface for model training

### Phase 4: Spatial I/O (Planned)

- [ ] GDAL integration for raster I/O
- [ ] Support for ASC, GRD, BIL, GeoTIFF formats
- [ ] CSV reading for occurrence data
- [ ] R integration with raster/terra packages

### Phase 5: Model Evaluation (Planned)

- [ ] AUC calculation
- [ ] Model evaluation metrics
- [ ] Prediction on new data
- [ ] Spatial projection

## Directory Structure

```
Maxent/
├── cpp/                      # C++ core implementation
│   ├── include/maxent/      # Header files
│   │   ├── grid_dimension.hpp
│   │   ├── sample.hpp
│   │   └── grid.hpp
│   ├── src/                 # Implementation files
│   │   ├── grid_dimension.cpp
│   │   ├── sample.cpp
│   │   └── grid.cpp
│   ├── tests/               # C++ unit tests
│   │   ├── test_grid.cpp
│   │   └── test_sample.cpp
│   └── CMakeLists.txt       # Build configuration
│
├── R-package/               # R package with Rcpp bindings
│   ├── src/                 # Rcpp wrapper code
│   │   ├── rcpp_maxent.cpp
│   │   ├── Makevars
│   │   └── Makevars.win
│   ├── R/                   # R code
│   │   └── maxent.R
│   ├── tests/               # R unit tests
│   │   └── testthat/
│   │       └── test-maxent.R
│   ├── man/                 # Documentation
│   ├── DESCRIPTION
│   ├── NAMESPACE
│   ├── LICENSE
│   └── README.md
│
├── density/                 # Original Java code (preserved)
├── CODE_DESCRIPTION.md      # Architecture documentation
└── MIGRATION.md            # This file
```

## Building and Testing

### C++ Core

**Prerequisites:**
- CMake 3.15+
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Eigen3 library

**Build steps:**

```bash
cd cpp
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build .

# Run tests
ctest --verbose
```

**Install Eigen3 (if needed):**

```bash
# Ubuntu/Debian
sudo apt-get install libeigen3-dev

# macOS
brew install eigen

# Windows (vcpkg)
vcpkg install eigen3
```

### R Package

**Prerequisites:**
- R >= 4.0.0
- Rcpp, RcppEigen, testthat packages
- C++17 compatible compiler

**Build and test:**

```r
# Install dependencies
install.packages(c("Rcpp", "RcppEigen", "testthat", "devtools"))

# Build package
devtools::load_all("R-package")

# Run tests
devtools::test("R-package")

# Install
devtools::install("R-package")
```

**Using the package:**

```r
library(maxentcpp)

# Create grid dimension
dim <- maxent_dimension(
  nrows = 100, ncols = 100,
  xll = -120.0, yll = 35.0,
  cellsize = 0.1
)

# Create sample point
sample <- maxent_sample(
  lon = -118.5, lat = 36.5,
  name = "occurrence1",
  dim = dim
)

# Add environmental data
sample_set_feature(sample, "temperature", 25.5)
sample_set_feature(sample, "precipitation", 100.0)

# Create environmental grid
temp_grid <- maxent_grid(dim, "temperature")

# Populate with data
temp_matrix <- matrix(rnorm(10000, mean = 20, sd = 5), 
                     nrow = 100, ncol = 100)
set_grid_matrix(temp_grid, temp_matrix)

# Extract as matrix
result <- as_matrix(temp_grid)
```

## Design Decisions

### Why C++17?

- Modern memory management (smart pointers)
- Structured bindings and improved type deduction
- Better standard library features
- Good compiler support across platforms

### Why Rcpp?

- Seamless R/C++ integration
- Mature and well-tested
- Large community and extensive documentation
- Compatible with R spatial ecosystem (sf, raster, terra)

### Template-based Grid

The `Grid<T>` class is template-based to support different data types:
- `GridFloat` - Most common for environmental variables
- `GridDouble` - Higher precision when needed
- `GridInt` - Integer data (e.g., land cover classes)
- `GridByte` - Memory-efficient for small value ranges

### R Package Design

- **External pointers** - C++ objects managed via XPtr
- **Wrapper functions** - User-friendly R interface
- **Memory safety** - Automatic cleanup via XPtr destructors
- **Documentation** - Roxygen2 for R documentation
- **Testing** - testthat for comprehensive testing

## Java to C++ Translation Guide

### Data Types

| Java | C++ |
|------|-----|
| `double` | `double` |
| `float` | `float` |
| `int` | `int` |
| `String` | `std::string` |
| `HashMap<K,V>` | `std::unordered_map<K,V>` |
| `ArrayList<T>` | `std::vector<T>` |
| `Array[]` | `std::vector<T>` or `T[]` |

### Memory Management

**Java:**
```java
Sample sample = new Sample(0, 50, 50, 36.5, -118.5, "site1");
// Garbage collected automatically
```

**C++ (with smart pointers):**
```cpp
auto sample = std::make_unique<Sample>(0, 50, 50, 36.5, -118.5, "site1");
// Automatically freed when out of scope
```

### Collections

**Java:**
```java
HashMap<String, Double> featureMap = new HashMap<>();
featureMap.put("temperature", 25.5);
double temp = featureMap.get("temperature");
```

**C++:**
```cpp
std::unordered_map<std::string, double> feature_map;
feature_map["temperature"] = 25.5;
double temp = feature_map["temperature"];
```

### Abstract Classes

**Java:**
```java
public abstract class Feature {
    public abstract double eval(int i);
}

public class LinearFeature extends Feature {
    public double eval(int i) { /* ... */ }
}
```

**C++:**
```cpp
class Feature {
public:
    virtual ~Feature() = default;
    virtual double eval(int i) const = 0;
};

class LinearFeature : public Feature {
public:
    double eval(int i) const override { /* ... */ }
};
```

## Migration Strategy

### Incremental Approach

1. **Keep Java code functional** - Don't break existing functionality
2. **Port core first** - Data structures, then algorithms
3. **Test continuously** - Compare outputs with Java version
4. **Parallel development** - Can maintain both versions during transition

### Testing Strategy

1. **Unit tests** - Test each component in isolation
2. **Integration tests** - Test component interactions
3. **Regression tests** - Compare outputs with Java Maxent
4. **Performance tests** - Verify speedup from C++

### Compatibility

**File format compatibility:**
- Lambda files (.lambdas) - Same format
- Grid files (.asc, .grd, .bil) - Same format
- CSV files - Same format

This ensures smooth migration and allows comparison with Java version.

## Performance Expectations

Expected speedups from C++ implementation:

- **Grid operations**: 2-5x faster (lower overhead)
- **Feature evaluation**: 3-7x faster (template specialization)
- **Model training**: 2-4x faster (optimized math libraries)
- **Memory usage**: 30-50% reduction (no JVM overhead)

Actual performance will vary based on dataset size and hardware.

## Contributing

### Adding New Features

1. **C++ implementation:**
   - Add header to `cpp/include/maxent/`
   - Add implementation to `cpp/src/`
   - Add test to `cpp/tests/`

2. **R bindings:**
   - Add Rcpp wrapper to `R-package/src/rcpp_maxent.cpp`
   - Add R functions to `R-package/R/`
   - Add documentation with Roxygen2
   - Add tests to `R-package/tests/testthat/`

3. **Documentation:**
   - Update this MIGRATION.md
   - Update CODE_DESCRIPTION.md if architecture changes
   - Update R package README.md

### Code Style

**C++:**
- Use C++17 features
- Smart pointers for ownership
- `snake_case` for functions and variables
- `PascalCase` for classes
- Comprehensive comments and documentation

**R:**
- Follow tidyverse style guide
- Use `snake_case` for functions
- Roxygen2 for documentation
- testthat for testing

## Resources

### Documentation

- [C++ Reference](https://en.cppreference.com/)
- [Rcpp](http://www.rcpp.org/)
- [RcppEigen](http://dirk.eddelbuettel.com/code/rcpp.eigen.html)
- [CMake](https://cmake.org/documentation/)

### Related Projects

- [Original Java Maxent](https://github.com/alrobles/Maxent)
- [maxnet R package](https://CRAN.R-project.org/package=maxnet)
- [ENMeval](https://CRAN.R-project.org/package=ENMeval)

## Roadmap

### Short-term (1-2 months)
- [x] Core data structures (Sample, Grid, GridDimension)
- [x] R package structure and basic bindings
- [ ] Feature classes (Linear, Quadratic, Product, Threshold, Hinge)
- [ ] R interface for features

### Medium-term (3-6 months)
- [ ] FeaturedSpace and model training
- [ ] Spatial data I/O with GDAL
- [ ] Model evaluation (AUC, metrics)
- [ ] Complete R workflow examples

### Long-term (6-12 months)
- [ ] All utility tools ported
- [ ] Performance optimization
- [ ] Comprehensive documentation
- [ ] CRAN release of R package

## Support

For questions or issues:
- GitHub Issues: https://github.com/alrobles/Maxent/issues
- Discussion: GitHub Discussions

## License

MIT License - See LICENSE.md

Original Maxent by Steven Phillips, Miro Dudík, and Rob Schapire.
C++ migration by Maxent Contributors.
