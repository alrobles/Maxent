#include <iostream>
#include <cassert>
#include "maxent/sample.hpp"

int main() {
    using namespace maxent;

    std::cout << "Testing Sample class..." << std::endl;

    // Create a grid dimension
    GridDimension dim(100, 100, -120.0, 35.0, 0.1);

    // Create a sample
    Sample sample(0, 50, 50, 36.5, -118.5, "test_sample");

    // Test basic properties
    assert(sample.getPoint() == 0);
    assert(sample.getRow() == 50);
    assert(sample.getCol() == 50);
    assert(sample.getLat() == 36.5);
    assert(sample.getLon() == -118.5);
    assert(sample.getName() == "test_sample");

    // Test feature map
    sample.setFeature("temperature", 25.5);
    sample.setFeature("precipitation", 100.0);

    assert(sample.hasFeature("temperature"));
    assert(sample.getFeature("temperature") == 25.5);
    assert(sample.getFeature("precipitation") == 100.0);
    assert(!sample.hasFeature("elevation"));
    assert(sample.getFeature("elevation", -999.0) == -999.0);

    // Test row/col calculation from coordinates
    int calc_row = sample.getRow(dim);
    int calc_col = sample.getCol(dim);
    std::cout << "Calculated row: " << calc_row << ", col: " << calc_col << std::endl;

    std::cout << "All Sample tests passed!" << std::endl;
    return 0;
}
