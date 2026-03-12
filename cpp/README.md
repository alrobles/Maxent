# Maxent C++ Core Library

This directory contains the C++ core implementation of Maxent for species distribution modeling.

## Structure

- `include/maxent/` - Header files for the C++ library
- `src/` - Implementation files
- `tests/` - Unit tests
- `CMakeLists.txt` - CMake build configuration

## Building

### Prerequisites

- CMake 3.15 or later
- C++17 compatible compiler
- Eigen3 library

### Build Steps

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Running Tests

```bash
cd build
ctest --verbose
```

Or run individual tests:

```bash
./tests/test_grid
./tests/test_sample
```

## Current Implementation

### Core Data Structures

- **GridDimension** - Spatial dimensions and coordinate system
- **Sample** - Species occurrence point with geographic coordinates
- **Grid<T>** - Template-based raster grid for environmental variables

### Features

- Row/column to lat/lon coordinate conversion
- Bilinear interpolation for grid values
- Support for multiple data types (float, double, int, etc.)
- No-data value handling

## Usage Example

```cpp
#include "maxent/grid_dimension.hpp"
#include "maxent/sample.hpp"
#include "maxent/grid.hpp"

using namespace maxent;

// Create spatial dimensions
GridDimension dim(100, 100, -120.0, 35.0, 0.1);

// Create a sample point
Sample sample(0, 50, 50, 36.5, -118.5, "site1");
sample.setFeature("temperature", 25.5);

// Create environmental grid
GridFloat temp_grid(dim, "temperature");
temp_grid.setValue(50, 50, 20.5f);

// Evaluate at sample location
float value = temp_grid.getValue(sample.getRow(), sample.getCol());
```

## Next Steps

- Feature classes (Linear, Quadratic, Product, Threshold, Hinge)
- FeaturedSpace for model training
- GDAL integration for raster I/O
- Optimization algorithms (L-BFGS)

See [../MIGRATION.md](../MIGRATION.md) for the complete migration plan.
