#include <iostream>
#include <cassert>
#include "maxent/grid.hpp"

int main() {
    using namespace maxent;

    std::cout << "Testing Grid class..." << std::endl;

    // Create a grid dimension
    GridDimension dim(10, 10, -120.0, 35.0, 0.1);

    // Create a float grid
    GridFloat grid(dim, "test_grid");

    // Test basic properties
    assert(grid.getRows() == 10);
    assert(grid.getCols() == 10);
    assert(grid.size() == 100);
    assert(grid.getName() == "test_grid");

    // Test setting and getting values
    grid.setValue(5, 5, 42.5f);
    assert(grid.getValue(5, 5) == 42.5f);
    assert(grid.hasData(5, 5) == true);

    // Test nodata
    float nodata = grid.getNodataValue();
    grid.setValue(3, 3, nodata);
    assert(grid.hasData(3, 3) == false);

    // Test geographic coordinates
    double lon = dim.toLon(5);
    double lat = dim.toLat(5);
    grid.setValue(5, 5, 100.0f);
    assert(grid.eval(lon, lat) == 100.0f);

    std::cout << "All Grid tests passed!" << std::endl;
    return 0;
}
