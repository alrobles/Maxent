# Quick Start Guide: Maxent C++ with R

This guide will get you started with the new C++ implementation of Maxent with R bindings.

## Installation

### Step 1: Install Prerequisites

**For R Users:**
```r
install.packages(c("Rcpp", "RcppEigen", "devtools", "testthat"))
```

**For C++ Developers:**
```bash
# Ubuntu/Debian
sudo apt-get install cmake libeigen3-dev build-essential

# macOS
brew install cmake eigen

# Windows
# Use vcpkg or install CMake and Eigen manually
```

### Step 2: Build C++ Core (Optional)

If you want to test the C++ library directly:

```bash
cd cpp
mkdir build
cd build
cmake ..
cmake --build .
ctest --verbose
```

### Step 3: Install R Package

```r
# From R
devtools::load_all("R-package")

# Or install
devtools::install("R-package")
```

## Basic Usage

### Example 1: Create a Sample Point

```r
library(maxentcpp)

# Create a grid dimension (spatial reference)
dim <- maxent_dimension(
  nrows = 100, 
  ncols = 100,
  xll = -120.0,    # Lower-left X coordinate
  yll = 35.0,      # Lower-left Y coordinate
  cellsize = 0.1   # Cell size in degrees
)

# Create a species occurrence sample
sample <- maxent_sample(
  lon = -118.5,
  lat = 36.5,
  name = "california_site_1",
  dim = dim
)

# View sample info
get_sample_info(sample)

# Add environmental variable values
sample_set_feature(sample, "temperature", 25.5)
sample_set_feature(sample, "precipitation", 100.0)
sample_set_feature(sample, "elevation", 500.0)

# Retrieve values
temp <- sample_get_feature(sample, "temperature")
print(temp)  # 25.5
```

### Example 2: Work with Environmental Grids

```r
library(maxentcpp)

# Create spatial dimension
dim <- maxent_dimension(50, 50, -120.0, 35.0, 0.1)

# Create a temperature grid
temp_grid <- maxent_grid(dim, "temperature", nodata_value = -9999)

# Set values manually
for (i in 0:49) {
  for (j in 0:49) {
    # Create temperature gradient
    value <- 15 + (i / 49) * 10 + rnorm(1, 0, 2)
    grid_set_value(temp_grid, i, j, value)
  }
}

# Get specific value
val <- grid_get_value(temp_grid, 25, 25)

# Check if cell has data
has_data <- grid_has_data(temp_grid, 25, 25)

# Convert to R matrix for visualization
temp_matrix <- as_matrix(temp_grid)
image(temp_matrix, main = "Temperature Grid")
```

### Example 3: Populate Grid from Matrix

```r
library(maxentcpp)

# Create dimension and grid
dim <- maxent_dimension(20, 20, -120.0, 35.0, 0.1)
precip_grid <- maxent_grid(dim, "precipitation")

# Create data in R
precip_data <- matrix(
  rnorm(400, mean = 100, sd = 30),
  nrow = 20, 
  ncol = 20
)

# Transfer to C++ grid
set_grid_matrix(precip_grid, precip_data)

# Verify
result <- as_matrix(precip_grid)
all.equal(result, precip_data)  # Should be TRUE
```

### Example 4: Multiple Samples and Grids

```r
library(maxentcpp)

# Set up spatial domain
dim <- maxent_dimension(100, 100, -125.0, 30.0, 0.1)

# Create multiple environmental grids
temp_grid <- maxent_grid(dim, "temperature")
precip_grid <- maxent_grid(dim, "precipitation")
elev_grid <- maxent_grid(dim, "elevation")

# Populate with realistic data (simplified)
set_grid_matrix(temp_grid, matrix(rnorm(10000, 20, 5), 100, 100))
set_grid_matrix(precip_grid, matrix(rnorm(10000, 100, 30), 100, 100))
set_grid_matrix(elev_grid, matrix(runif(10000, 0, 2000), 100, 100))

# Create multiple occurrence samples
samples <- list()
coords <- data.frame(
  lon = runif(50, -125, -115),
  lat = runif(50, 30, 40),
  name = paste0("site_", 1:50)
)

for (i in 1:nrow(coords)) {
  sample <- maxent_sample(
    coords$lon[i],
    coords$lat[i],
    coords$name[i],
    dim
  )
  
  # Get row/col for this sample
  info <- get_sample_info(sample)
  
  # Extract environmental values at sample location
  if (grid_has_data(temp_grid, info$row, info$col)) {
    sample_set_feature(sample, "temperature", 
                      grid_get_value(temp_grid, info$row, info$col))
    sample_set_feature(sample, "precipitation",
                      grid_get_value(precip_grid, info$row, info$col))
    sample_set_feature(sample, "elevation",
                      grid_get_value(elev_grid, info$row, info$col))
  }
  
  samples[[i]] <- sample
}

# Examine a sample
get_sample_info(samples[[1]])
sample_get_feature(samples[[1]], "temperature")
```

### Example 5: Coordinate Conversions

```r
library(maxentcpp)

# Create dimension
dim <- maxent_dimension(100, 100, -120.0, 35.0, 0.1)

# Convert geographic coordinates to grid indices
coords <- coords_to_rowcol(dim, lon = -118.5, lat = 36.5)
row_idx <- coords[1]
col_idx <- coords[2]

print(paste("Lon/Lat (-118.5, 36.5) maps to row:", row_idx, "col:", col_idx))

# Get dimension info
dim_info <- get_grid_dimension_info(dim)
print(dim_info)
```

### Example 6: Spatial I/O – Read and Write ASC Grids (Phase 4)

```r
library(maxentcpp)

# Build a small grid from an R matrix
mat <- matrix(c(20.5, 21.0, 19.8,
                22.1, NA,   18.3), nrow = 2, byrow = TRUE)

g <- maxent_grid_from_matrix(mat,
                              xll = -120.0, yll = 35.0,
                              cellsize = 0.1, name = "temperature")

# Write to ESRI ASCII format
maxent_write_asc(g, "temperature.asc", scientific = FALSE)

# Read it back
g2   <- maxent_read_asc("temperature.asc")
info <- maxent_grid_info(g2)
print(info)  # nrows=2, ncols=3, count_data=5

mat2 <- maxent_grid_to_matrix(g2)
print(mat2)  # NA cell preserved
```

### Example 7: CSV I/O – Write and Read Occurrence Data (Phase 4)

```r
library(maxentcpp)

# ---- Write occurrence records ----
w <- maxent_csv_write_open("occurrences.csv")
for (i in seq_len(3)) {
    maxent_csv_write(w, "species", "Quercus agrifolia")
    maxent_csv_write_num(w, "longitude", -118.0 - i * 0.5)
    maxent_csv_write_num(w, "latitude",  36.0 + i * 0.1)
    maxent_csv_write_row(w)
}
maxent_csv_write_close(w)

# ---- Read it back ----
reader <- maxent_csv_open("occurrences.csv")
hdrs   <- maxent_csv_headers(reader)
print(hdrs)  # "species" "longitude" "latitude"

while (!is.null(rec <- maxent_csv_next(reader))) {
    print(rec)
}
maxent_csv_close(reader)

# ---- Read a numeric column directly ----
reader2 <- maxent_csv_open("occurrences.csv")
lons    <- maxent_csv_read_column(reader2, "longitude")
maxent_csv_close(reader2)
print(lons)

# ---- Layer metadata ----
l    <- maxent_layer("temperature", "Continuous")
info <- maxent_layer_info(l)
print(info)  # $name="temperature" $type="Continuous"

# Extract name from a file path
name <- maxent_layer_name("/data/bio1.asc")
print(name)  # "bio1"
```

## Integration with Existing R Packages

### Using with raster package

```r
library(maxentcpp)
library(raster)

# Create maxent grid
dim <- maxent_dimension(50, 50, -120.0, 35.0, 0.1)
mx_grid <- maxent_grid(dim, "temperature")

# Populate with data
temp_data <- matrix(rnorm(2500, 20, 5), 50, 50)
set_grid_matrix(mx_grid, temp_data)

# Convert to raster
temp_raster <- raster(as_matrix(mx_grid))
extent(temp_raster) <- c(-120.0, -115.0, 35.0, 40.0)

# Plot
plot(temp_raster, main = "Temperature from Maxent Grid")
```

### Using with terra package

```r
library(maxentcpp)
library(terra)

# Create maxent grid and populate
dim <- maxent_dimension(30, 30, -120.0, 35.0, 0.1)
mx_grid <- maxent_grid(dim, "elevation")
elev_data <- matrix(runif(900, 0, 2000), 30, 30)
set_grid_matrix(mx_grid, elev_data)

# Convert to SpatRaster
elev_matrix <- as_matrix(mx_grid)
elev_terra <- rast(elev_matrix)
ext(elev_terra) <- c(-120.0, -117.0, 35.0, 38.0)

# Visualize
plot(elev_terra, main = "Elevation from Maxent Grid")
```

## Running Tests

### R Package Tests

```r
# Load package in development mode
devtools::load_all("R-package")

# Run all tests
devtools::test("R-package")

# Run specific test file
testthat::test_file("R-package/tests/testthat/test-maxent.R")
```

### C++ Tests

```bash
cd cpp/build
ctest --verbose

# Or run specific tests
./tests/test_grid
./tests/test_sample
```

## Next Steps

Future phases will add:

1. **Complete end-to-end workflow** examples
2. **Performance optimization** for large datasets
3. **CRAN release** of the R package

### Example: Model Evaluation (Phase 5)

```r
library(maxentcpp)

# Prediction scores at presence and absence sites
presence_scores <- c(0.85, 0.92, 0.78, 0.96, 0.88)
absence_scores  <- c(0.12, 0.25, 0.08, 0.35, 0.18)

# Compute AUC
auc_result <- maxent_auc(presence_scores, absence_scores)
cat("AUC:", auc_result$auc, "\n")
cat("Max Kappa:", auc_result$max_kappa, "\n")

# Full model evaluation
metrics <- maxent_evaluate(presence_scores, absence_scores)
cat("AUC:", metrics$auc, "\n")
cat("Correlation:", metrics$correlation, "\n")
cat("Log-loss:", metrics$logloss, "\n")
cat("Squared Error:", metrics$square_error, "\n")
cat("Misclassification:", metrics$misclassification, "\n")
```

## Troubleshooting

### C++ Compilation Issues

**Error: Cannot find Eigen**
```r
# Install RcppEigen
install.packages("RcppEigen")
```

**Error: C++17 not supported**
- Update your compiler (GCC 7+, Clang 5+, MSVC 2017+)
- On older systems, you may need to install a newer compiler

### R Package Issues

**Error: Package 'maxentcpp' is not available**
- Make sure you're using `devtools::load_all()` or `devtools::install()` with the local path
- This package is not yet on CRAN

**Error: Unable to load shared object**
- Rebuild the package: `devtools::clean_dll("R-package"); devtools::load_all("R-package")`

## Getting Help

- **Documentation**: See `MIGRATION.md` for detailed technical info
- **Issues**: Report bugs on GitHub Issues
- **Discussions**: Use GitHub Discussions for questions

## Performance Tips

1. **Batch operations**: Set multiple grid values before converting to matrix
2. **Reuse objects**: Create dimension objects once and reuse
3. **Memory**: Large grids are memory-efficient in C++, but matrix conversion copies data
4. **Testing**: Use small grids for testing, scale up for production

## Example Workflow

Complete example combining all concepts:

```r
library(maxentcpp)

# 1. Define study area
dim <- maxent_dimension(100, 100, -125.0, 30.0, 0.1)

# 2. Load or create environmental data
temp <- maxent_grid(dim, "temperature")
precip <- maxent_grid(dim, "precipitation")

# Populate from R matrices or external sources
set_grid_matrix(temp, matrix(rnorm(10000, 20, 5), 100, 100))
set_grid_matrix(precip, matrix(rnorm(10000, 100, 30), 100, 100))

# 3. Create occurrence samples
sample_coords <- data.frame(
  lon = c(-122.5, -121.0, -120.5, -119.0),
  lat = c(35.5, 36.0, 36.5, 37.0),
  species = c("oak", "oak", "oak", "oak")
)

samples <- lapply(1:nrow(sample_coords), function(i) {
  s <- maxent_sample(
    sample_coords$lon[i],
    sample_coords$lat[i],
    sample_coords$species[i],
    dim
  )
  
  # Extract environment at sample
  info <- get_sample_info(s)
  sample_set_feature(s, "temperature", 
                    grid_get_value(temp, info$row, info$col))
  sample_set_feature(s, "precipitation",
                    grid_get_value(precip, info$row, info$col))
  s
})

# 4. Examine samples
for (i in 1:length(samples)) {
  info <- get_sample_info(samples[[i]])
  temp_val <- sample_get_feature(samples[[i]], "temperature")
  print(sprintf("Sample %s: temp=%.2f", info$name, temp_val))
}

# 5. Visualize
par(mfrow = c(1, 2))
image(as_matrix(temp), main = "Temperature")
image(as_matrix(precip), main = "Precipitation")

# Add sample points
points_df <- do.call(rbind, lapply(samples, function(s) {
  info <- get_sample_info(s)
  data.frame(lon = info$lon, lat = info$lat)
}))
```

## What's Implemented (Phase 5)

✅ Core data structures (Sample, Grid, GridDimension)  
✅ Coordinate transformations  
✅ Grid value get/set  
✅ Matrix conversions  
✅ Feature storage on samples  
✅ R/C++ integration via Rcpp  
✅ Feature classes (Linear, Quadratic, Product, Threshold, Hinge)  
✅ Model training (FeaturedSpace, sequential coordinate ascent)  
✅ Lambda file I/O (model persistence)  
✅ Spatial data I/O (ESRI ASCII .asc read/write)  
✅ CSV reader/writer (occurrence data, SWD files)  
✅ Environmental layer metadata (Layer class)  
✅ Model evaluation (AUC, kappa, correlation, log-loss, squared error)  
✅ Spatial projection (raw, cloglog, logistic output)  
✅ Unit tests (10 C++ test suites, R testthat tests)  

## Coming Next

⏳ Complete end-to-end workflow examples  
⏳ Performance optimization  
⏳ CRAN release of R package

See `MIGRATION.md` for the complete roadmap!
