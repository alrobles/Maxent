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
sample <- maxent_sample(
  lon = -118.5,
  lat = 36.5,
  name = "location1",
  dim = maxent_dimension(100, 100, -120.0, 35.0, 0.1)
)

# Create a grid for environmental variables
dim <- maxent_dimension(
  nrows = 100,
  ncols = 100,
  xll = -120.0,
  yll = 35.0,
  cellsize = 0.1
)

grid <- maxent_grid(dim, "temperature")

# Read an ESRI ASCII raster file
# g <- maxent_read_asc("bio1.asc")

# Write occurrence data to CSV
w <- maxent_csv_write_open("occurrences.csv")
maxent_csv_write(w, "species", "Quercus agrifolia")
maxent_csv_write_num(w, "longitude", -118.5)
maxent_csv_write_num(w, "latitude",  36.5)
maxent_csv_write_row(w)
maxent_csv_write_close(w)
```

## Development Status

This package is under active development. Current status:

- [x] Core data structures (Sample, Grid, GridDimension)
- [x] R bindings for core structures
- [x] Feature classes (Linear, Quadratic, Product, Threshold, Hinge)
- [x] Model training (FeaturedSpace, sequential coordinate ascent)
- [x] Lambda file I/O (model persistence)
- [x] Spatial data I/O (ESRI ASCII .asc read/write)
- [x] CSV reader/writer (occurrence data, SWD files)
- [x] Environmental layer metadata (Layer class)
- [x] Model evaluation (AUC, kappa, correlation, log-loss, metrics)
- [x] Spatial projection (raw, cloglog, logistic output)
- [ ] Complete end-to-end workflow examples
- [ ] CRAN release

## Contributing

Contributions are welcome! This is a community-driven migration from the Java implementation.

## License

MIT License - See LICENSE file for details

## Related Projects

- [Java Maxent](https://github.com/alrobles/Maxent) - Original Java implementation
- [maxnet](https://CRAN.R-project.org/package=maxnet) - R implementation using glmnet

## Acknowledgments

Based on the original Maxent software by Steven Phillips, Miro Dudík, and Rob Schapire.
