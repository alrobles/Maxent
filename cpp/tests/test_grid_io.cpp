#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>
#include <string>
#include <cstdio>
#include "maxent/grid_io.hpp"

static bool near(double a, double b, double tol = 1e-4) {
    return std::fabs(a - b) < tol;
}

// Helper: write a simple ASC file
static void write_sample_asc(const std::string& path,
                              int nrows, int ncols,
                              double xll, double yll, double cellsize,
                              int nodata,
                              const std::vector<std::vector<float>>& data) {
    std::ofstream out(path);
    out << "ncols         " << ncols << "\n";
    out << "nrows         " << nrows << "\n";
    out << "xllcorner     " << xll << "\n";
    out << "yllcorner     " << yll << "\n";
    out << "cellsize      " << cellsize << "\n";
    out << "NODATA_value  " << nodata << "\n";
    for (int i = 0; i < nrows; ++i) {
        for (int j = 0; j < ncols; ++j) {
            if (j > 0) out << " ";
            out << data[i][j];
        }
        out << "\n";
    }
    out.close();
}

int main() {
    using namespace maxent;

    std::cout << "Testing GridIO..." << std::endl;

    // Test 1: Read a simple ASC file
    {
        int nrows = 3, ncols = 4;
        std::vector<std::vector<float>> data = {
            {1.0f, 2.0f, 3.0f, -9999.0f},
            {5.0f, 6.0f, -9999.0f, 8.0f},
            {9.0f, 10.0f, 11.0f, 12.0f}
        };
        std::string path = "/tmp/test_asc1.asc";
        write_sample_asc(path, nrows, ncols, -120.0, 35.0, 0.5, -9999, data);

        auto grid = GridIO::read_asc(path);

        assert(grid.getRows() == 3);
        assert(grid.getCols() == 4);
        assert(near(grid.getDimension().xll, -120.0));
        assert(near(grid.getDimension().yll, 35.0));
        assert(near(grid.getDimension().cellsize, 0.5));
        assert(near(grid.getNodataValue(), -9999.0f));

        // Check data values
        assert(near(grid.getValue(0, 0), 1.0f));
        assert(near(grid.getValue(0, 1), 2.0f));
        assert(near(grid.getValue(1, 1), 6.0f));
        assert(near(grid.getValue(2, 3), 12.0f));

        // Check nodata
        assert(!grid.hasData(0, 3));
        assert(!grid.hasData(1, 2));
        assert(grid.hasData(0, 0));

        // Check name extraction
        assert(grid.getName() == "test_asc1");

        std::cout << "  Test 1 (read ASC): OK" << std::endl;
        std::remove(path.c_str());
    }

    // Test 2: Write and read back a GridFloat
    {
        GridDimension dim(5, 4, -118.0, 34.0, 0.25);
        GridFloat grid(dim, "temperature", -9999.0f);

        for (int i = 0; i < 5; ++i) {
            for (int j = 0; j < 4; ++j) {
                grid.setValue(i, j, static_cast<float>(i * 10 + j));
            }
        }
        // Set one cell as nodata
        grid.setValue(2, 2, -9999.0f);

        std::string path = "/tmp/test_asc2.asc";
        GridIO::write_asc(grid, path, false);

        auto grid2 = GridIO::read_asc(path);

        assert(grid2.getRows() == 5);
        assert(grid2.getCols() == 4);
        assert(near(grid2.getDimension().xll, -118.0));
        assert(near(grid2.getDimension().yll, 34.0));
        assert(near(grid2.getDimension().cellsize, 0.25));

        for (int i = 0; i < 5; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (i == 2 && j == 2) {
                    assert(!grid2.hasData(i, j));
                } else {
                    assert(near(grid2.getValue(i, j), grid.getValue(i, j)));
                }
            }
        }

        std::cout << "  Test 2 (write + read round-trip): OK" << std::endl;
        std::remove(path.c_str());
    }

    // Test 3: Write/read GridDouble
    {
        GridDimension dim(3, 3, 0.0, 0.0, 1.0);
        GridDouble grid(dim, "elevation", -9999.0);

        grid.setValue(0, 0, 100.123456789);
        grid.setValue(0, 1, 200.987654321);
        grid.setValue(0, 2, -9999.0);
        grid.setValue(1, 0, 300.0);
        grid.setValue(1, 1, 400.5);
        grid.setValue(1, 2, 500.75);
        grid.setValue(2, 0, 600.0);
        grid.setValue(2, 1, 700.0);
        grid.setValue(2, 2, 800.0);

        std::string path = "/tmp/test_asc3.asc";
        GridIO::write_asc(grid, path, false);

        auto grid2 = GridIO::read_asc_double(path);
        assert(grid2.getRows() == 3);
        assert(grid2.getCols() == 3);
        assert(near(grid2.getValue(0, 0), 100.1235, 0.01));
        assert(!grid2.hasData(0, 2));
        assert(near(grid2.getValue(1, 1), 400.5));

        std::cout << "  Test 3 (GridDouble write + read): OK" << std::endl;
        std::remove(path.c_str());
    }

    // Test 4: Write/read GridInt
    {
        GridDimension dim(2, 3, 0.0, 0.0, 1.0);
        GridInt grid(dim, "landcover", -9999);

        grid.setValue(0, 0, 1);
        grid.setValue(0, 1, 2);
        grid.setValue(0, 2, 3);
        grid.setValue(1, 0, -9999);
        grid.setValue(1, 1, 5);
        grid.setValue(1, 2, 6);

        std::string path = "/tmp/test_asc4.asc";
        GridIO::write_asc(grid, path);

        // Read back as float (integers will round-trip through float)
        auto grid2 = GridIO::read_asc(path);
        assert(grid2.getRows() == 2);
        assert(grid2.getCols() == 3);
        assert(near(grid2.getValue(0, 0), 1.0f));
        assert(near(grid2.getValue(0, 2), 3.0f));
        assert(!grid2.hasData(1, 0));
        assert(near(grid2.getValue(1, 1), 5.0f));

        std::cout << "  Test 4 (GridInt write + read): OK" << std::endl;
        std::remove(path.c_str());
    }

    // Test 5: read_grid auto-detection
    {
        int nrows = 2, ncols = 2;
        std::vector<std::vector<float>> data = {
            {1.0f, 2.0f},
            {3.0f, 4.0f}
        };
        std::string path = "/tmp/test_asc5.asc";
        write_sample_asc(path, nrows, ncols, 0.0, 0.0, 1.0, -9999, data);

        auto grid = GridIO::read_grid(path);
        assert(grid.getRows() == 2);
        assert(near(grid.getValue(0, 0), 1.0f));
        assert(near(grid.getValue(1, 1), 4.0f));

        std::cout << "  Test 5 (read_grid auto-detect): OK" << std::endl;
        std::remove(path.c_str());
    }

    // Test 6: Decimal comma handling
    {
        std::string path = "/tmp/test_asc6.asc";
        std::ofstream out(path);
        out << "ncols         2\n";
        out << "nrows         2\n";
        out << "xllcorner     0,0\n";  // decimal comma
        out << "yllcorner     0,0\n";
        out << "cellsize      1,0\n";
        out << "NODATA_value  -9999\n";
        out << "1,5 2,5\n";  // decimal commas in data
        out << "3,5 4,5\n";
        out.close();

        auto grid = GridIO::read_asc(path);
        assert(near(grid.getValue(0, 0), 1.5f));
        assert(near(grid.getValue(0, 1), 2.5f));
        assert(near(grid.getValue(1, 0), 3.5f));
        assert(near(grid.getValue(1, 1), 4.5f));

        std::cout << "  Test 6 (decimal comma handling): OK" << std::endl;
        std::remove(path.c_str());
    }

    // Test 7: countData on read grid
    {
        int nrows = 2, ncols = 3;
        std::vector<std::vector<float>> data = {
            {1.0f, -9999.0f, 3.0f},
            {-9999.0f, 5.0f, 6.0f}
        };
        std::string path = "/tmp/test_asc7.asc";
        write_sample_asc(path, nrows, ncols, 0.0, 0.0, 1.0, -9999, data);

        auto grid = GridIO::read_asc(path);
        assert(grid.countData() == 4);

        std::cout << "  Test 7 (countData): OK" << std::endl;
        std::remove(path.c_str());
    }

    // Test 8: Error on missing file
    {
        bool caught = false;
        try {
            GridIO::read_asc("/tmp/nonexistent_grid.asc");
        } catch (const std::runtime_error&) {
            caught = true;
        }
        assert(caught);
        std::cout << "  Test 8 (missing file error): OK" << std::endl;
    }

    // Test 9: Error on unsupported format
    {
        bool caught = false;
        try {
            GridIO::read_grid("/tmp/test.xyz");
        } catch (const std::runtime_error&) {
            caught = true;
        }
        assert(caught);
        std::cout << "  Test 9 (unsupported format error): OK" << std::endl;
    }

    // Test 10: Scientific notation in ASC output
    {
        GridDimension dim(2, 2, 0.0, 0.0, 1.0);
        GridFloat grid(dim, "sci", -9999.0f);
        grid.setValue(0, 0, 0.000123f);
        grid.setValue(0, 1, 1.5e6f);
        grid.setValue(1, 0, -0.0042f);
        grid.setValue(1, 1, 99.99f);

        std::string path = "/tmp/test_asc10.asc";
        GridIO::write_asc(grid, path, true);  // scientific notation

        auto grid2 = GridIO::read_asc(path);
        assert(near(grid2.getValue(0, 0), 0.000123f, 1e-6));
        assert(near(grid2.getValue(0, 1), 1.5e6f, 100.0));
        assert(near(grid2.getValue(1, 0), -0.0042f, 1e-5));
        assert(near(grid2.getValue(1, 1), 99.99f, 0.01));

        std::cout << "  Test 10 (scientific notation): OK" << std::endl;
        std::remove(path.c_str());
    }

    std::cout << "All GridIO tests passed!" << std::endl;
    return 0;
}
