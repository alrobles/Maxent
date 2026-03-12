# Maxent

Maxent is a stand-alone application for modelling species geographic distributions using the Maximum Entropy approach.  This repository contains both the original Java implementation and an ongoing C++ migration with R bindings.

For further information and the current Maxent software release, please see the Maxent home page at the American Museum of Natural History:  http://biodiversityinformatics.amnh.org/open_source/maxent.

If you are looking for the R package maxnet, which implements much of the functionality of the Java application, please look for "maxnet" rather than "Maxent" on GitHub (for the open source code) or on [CRAN](https://CRAN.R-project.org/package=maxnet) (for the R package).  Contributions to both the Java application and the R package are welcome.

## C++ Migration

The codebase is being migrated from Java to modern C++17 with R bindings via Rcpp. See [MIGRATION.md](MIGRATION.md) for full details.

| Phase | Status | Description |
|-------|--------|-------------|
| 1. Core Data Structures | ✅ Done | `GridDimension`, `Sample`, `Grid<T>` |
| 2. Feature Engineering | ✅ Done | Linear, Quadratic, Product, Threshold, Hinge features |
| 3. Core Algorithm | ✅ Done | `FeaturedSpace`, sequential training, lambda I/O |
| 4. Spatial I/O | 🔮 Planned | GDAL integration, raster format support |
| 5. Model Evaluation | 🔮 Planned | AUC, evaluation metrics |

### Building the C++ Core

```bash
# Install Eigen3 (Ubuntu/Debian)
sudo apt-get install libeigen3-dev

# Build and test
cd cpp
mkdir build && cd build
cmake ..
cmake --build .
ctest --verbose
```

### R Package

```r
install.packages(c("Rcpp", "RcppEigen", "testthat", "devtools"))
devtools::install("R-package")
```
