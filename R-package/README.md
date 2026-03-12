# maxentcpp

Maximum Entropy Species Distribution Modeling - C++ Implementation with R Interface

## Overview

`maxentcpp` is a high-performance C++ implementation of the Maxent species distribution modeling algorithm, with seamless R integration through Rcpp. This package aims to provide:

- **Performance**: Faster execution through optimized C++ code
- **Compatibility**: File format compatibility with the Java Maxent
- **Integration**: Native R interface for easy use in R workflows
- **Modern**: Built with modern C++17 and R best practices

## Installation

### Prerequisites

- R >= 4.0.0
- C++17 compatible compiler
- Eigen3 library (usually installed via Rcpp/RcppEigen)

### From Source

```r
# Install dependencies
install.packages(c("Rcpp", "RcppEigen", "testthat"))

# Build and install
devtools::install("path/to/R-package")
```

## Quick Start

```r
library(maxentcpp)

# Create a sample point
sample <- create_sample(
  lon = -118.5,
  lat = 36.5,
  name = "location1"
)

# Create a grid for environmental variables
dim <- create_grid_dimension(
  nrows = 100,
  ncols = 100,
  xll = -120.0,
  yll = 35.0,
  cellsize = 0.1
)

grid <- create_grid_float(dim, "temperature")

# More functionality coming soon...
```

## Development Status

This package is under active development. Current status:

- [x] Core data structures (Sample, Grid, GridDimension)
- [x] R bindings for core structures
- [ ] Feature classes (Linear, Quadratic, Product, etc.)
- [ ] Model training (FeaturedSpace)
- [ ] Model evaluation and prediction
- [ ] Spatial data I/O (GDAL integration)
- [ ] Complete workflow examples

## Contributing

Contributions are welcome! This is a community-driven migration from the Java implementation.

## License

MIT License - See LICENSE file for details

## Related Projects

- [Java Maxent](https://github.com/alrobles/Maxent) - Original Java implementation
- [maxnet](https://CRAN.R-project.org/package=maxnet) - R implementation using glmnet

## Acknowledgments

Based on the original Maxent software by Steven Phillips, Miro Dudík, and Rob Schapire.
