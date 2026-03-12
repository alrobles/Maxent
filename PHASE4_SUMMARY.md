# Phase 4: Spatial I/O – ASC Grid Reader/Writer, CSV Reader/Writer, Layer Metadata

## Summary

Phase 4 implements **spatial data I/O**: reading and writing ESRI ASCII (.asc) grid
files, CSV parsing for species occurrence and SWD data, column-based CSV writing,
and environmental layer metadata. This phase ports `density/GridIO.java` (readASC),
`density/GridWriter.java` (writeASC), `density/Csv.java`, `density/CsvWriter.java`,
and `density/Layer.java` to modern C++17 with corresponding R bindings.

## Files Created/Modified

### New Files

| File | Lines | Description |
|------|-------|-------------|
| `cpp/include/maxent/layer.hpp` | ~130 | `Layer` class: name, type enum, path extraction |
| `cpp/include/maxent/csv_reader.hpp` | ~290 | `CsvReader`: comma/semicolon CSV, quoted fields, European mode |
| `cpp/include/maxent/csv_writer.hpp` | ~190 | `CsvWriter`: column-based CSV output, auto-header, quoting |
| `cpp/include/maxent/grid_io.hpp` | ~300 | `GridIO`: read/write ASC, format detection, decimal-comma support |
| `cpp/src/layer.cpp` | 2 | Stub include |
| `cpp/src/csv_reader.cpp` | 2 | Stub include |
| `cpp/src/csv_writer.cpp` | 2 | Stub include |
| `cpp/src/grid_io.cpp` | 2 | Stub include |
| `cpp/tests/test_layer.cpp` | ~100 | 8 C++ unit tests for Layer |
| `cpp/tests/test_csv_reader.cpp` | ~170 | 8 C++ unit tests for CsvReader |
| `cpp/tests/test_csv_writer.cpp` | ~130 | 5 C++ unit tests for CsvWriter |
| `cpp/tests/test_grid_io.cpp` | ~230 | 10 C++ unit tests for GridIO |
| `R-package/src/rcpp_grid_io.cpp` | ~280 | Rcpp bindings for all I/O operations |
| `R-package/R/grid_io.R` | ~170 | 13 user-friendly R wrapper functions |
| `R-package/tests/testthat/test-grid-io.R` | ~140 | 8 R testthat tests |
| `PHASE4_SUMMARY.md` | — | This file |

### Modified Files

| File | Changes |
|------|---------|
| `cpp/CMakeLists.txt` | Added 4 new source files to `MAXENT_SOURCES` |
| `cpp/tests/CMakeLists.txt` | Added 4 new test executables and `add_test()` calls |
| `R-package/DESCRIPTION` | Bumped version from 0.3.0 → 0.4.0 |
| `R-package/NAMESPACE` | Added 20+ new exported functions |
| `MIGRATION.md` | Phase 4 marked as ✅ Completed; directory structure updated |

## Key Design Decisions

### Header-only Implementation

Following the existing project pattern, all new headers (`layer.hpp`, `csv_reader.hpp`,
`csv_writer.hpp`, `grid_io.hpp`) are fully self-contained with inline implementations.
The `cpp/src/*.cpp` stubs exist only for CMake consistency.

### No GDAL Dependency

Rather than adding GDAL as a hard dependency, Phase 4 implements **native format
parsing** for the primary formats used by Java Maxent:

- **ASC** (ESRI ASCII Grid) – the most commonly used format
- **CSV** – for occurrence data and SWD files

This keeps the dependency footprint minimal (only Eigen3 required) and ensures the
library compiles on any platform without additional setup. GDAL integration can be
added as an optional dependency in a future phase for GeoTIFF/GRD/BIL support.

### Decimal Comma Handling

Both `GridIO` and `CsvReader` handle European locale conventions:

- **GridIO**: `parse_float()` and `parse_double()` check for partial parsing
  (e.g., `std::stof("1,5")` returns 1.0 with remainder). If the whole string
  wasn't consumed, commas are replaced with dots and re-parsed.
- **CsvReader**: Auto-detects semicolon separators on the first line. In European
  mode, commas in field values are converted to dots.

This matches the Java implementation in `GridIO.getDouble()` and `Csv.checkEuropean()`.

### CSV Architecture

- **CsvReader** (port of `Csv.java`): Streaming reader with header management,
  field-by-name access, quoted-field support, and bulk column extraction.
- **CsvWriter** (port of `CsvWriter.java`): Column-based writer where values are
  added by name and rows are flushed explicitly. Auto-generates headers on first row.

### Layer Type System

The `Layer` class preserves the 8 layer types from Java Maxent:
`Unknown`, `Continuous`, `Categorical`, `Bias`, `Mask`, `Probability`,
`Cumulative`, `DebiasAvg`. Case-insensitive type parsing is supported.

## C++ API

```cpp
// --- GridIO ---
auto grid  = GridIO::read_asc("bio1.asc");        // Read float grid
auto gridD = GridIO::read_asc_double("elev.asc");  // Read double grid
auto gridA = GridIO::read_grid("bio1.asc");         // Auto-detect format

GridIO::write_asc(grid, "output.asc");              // Write with sci notation
GridIO::write_asc(grid, "output.asc", false);       // Write fixed notation

// --- CsvReader ---
CsvReader csv("occurrences.csv");
while (csv.next_record()) {
    std::string species = csv.get("species");
    double lon = csv.get_double("longitude");
    double lat = csv.get_double("latitude");
}

auto temps = csv.read_double_column("temperature");  // Bulk column read

// --- CsvWriter ---
CsvWriter writer("results.csv");
writer.print("species", "oak");
writer.print("score", 0.85);
writer.println();  // Flush row
writer.close();

// --- Layer ---
Layer l("bio1", Layer::CONTINUOUS);
Layer l2("mask", "Mask");
std::string name = Layer::name_from_path("/data/bio1.asc");  // "bio1"
```

## R API

```r
# --- ASC Grid I/O ---
g <- maxent_read_asc("bio1.asc")           # Read
info <- maxent_grid_info(g)                 # Metadata
mat  <- maxent_grid_to_matrix(g)            # To R matrix (NA for nodata)
maxent_write_asc(g, "output.asc")           # Write

# Build grid from R matrix
g2 <- maxent_grid_from_matrix(mat, xll = -120, yll = 35, cellsize = 0.1)

# --- CSV ---
reader <- maxent_csv_open("occurrences.csv")
hdrs   <- maxent_csv_headers(reader)
rec    <- maxent_csv_next(reader)           # Next row as character vector
temps  <- maxent_csv_read_column(reader, "temperature")
maxent_csv_close(reader)

# --- Layer ---
l    <- maxent_layer("bio1", "Continuous")
info <- maxent_layer_info(l)
name <- maxent_layer_name("/data/bio1.asc")  # "bio1"
```

## Test Results

### C++ Tests (8 suites, 100% pass)

| Test | Count | Description |
|------|-------|-------------|
| GridTest | existing | Grid dimension and raster operations |
| SampleTest | existing | Sample point management |
| FeatureTest | existing | All 6 feature types + generator |
| FeaturedSpaceTest | existing | Training, prediction, lambda I/O |
| **LayerTest** | **8 new** | Construction, type parsing, name extraction |
| **CsvReaderTest** | **8 new** | Comma/semicolon, quotes, European mode, bulk read |
| **CsvWriterTest** | **5 new** | Write + read round-trip, integers, quoting |
| **GridIOTest** | **10 new** | ASC read/write, round-trip, decimal commas, errors |

### R Tests

8 new testthat tests in `test-grid-io.R` covering:
- Layer creation and type parsing
- Layer name extraction from path
- ASC write/read round-trip with nodata
- Auto-format detection
- CSV reader: headers, records, EOF
- CSV read_double_column
- Grid from/to matrix round-trip with NA
- Grid metadata inspection

## ASC File Format

The ESRI ASCII Grid format consists of a 6-line header and row-major data:

```
ncols         100
nrows         100
xllcorner     -120.0
yllcorner     35.0
cellsize      0.1
NODATA_value  -9999
<data row 1: space-separated values>
<data row 2>
...
```

The implementation supports:
- Integer and floating-point data values
- Scientific notation (e.g., `1.5e6`)
- Decimal commas (European locales, e.g., `1,5` → `1.5`)
- NaN values (mapped to NODATA)

## References

- Original Java source: `density/GridIO.java`, `density/GridWriter.java`,
  `density/Csv.java`, `density/CsvWriter.java`, `density/Layer.java`
- ESRI ASCII Grid format specification
- Phillips, S.J., Anderson, R.P., Schapire, R.E. (2006). Maximum entropy modeling
  of species geographic distributions. *Ecological Modelling*, 190(3-4), 231-259.
