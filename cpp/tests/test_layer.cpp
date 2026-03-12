#include <iostream>
#include <cassert>
#include <string>
#include "maxent/layer.hpp"

int main() {
    using namespace maxent;

    std::cout << "Testing Layer class..." << std::endl;

    // Test 1: Default construction
    {
        Layer l;
        assert(l.name().empty());
        assert(l.type() == Layer::UNKNOWN);
        assert(l.type_name() == "Unknown");
        std::cout << "  Test 1 (default construction): OK" << std::endl;
    }

    // Test 2: Construct with name and int type
    {
        Layer l("bio1", Layer::CONTINUOUS);
        assert(l.name() == "bio1");
        assert(l.type() == Layer::CONTINUOUS);
        assert(l.type_name() == "Continuous");
        std::cout << "  Test 2 (name + int type): OK" << std::endl;
    }

    // Test 3: Construct with name and string type
    {
        Layer l("precip", "Categorical");
        assert(l.name() == "precip");
        assert(l.type() == Layer::CATEGORICAL);
        std::cout << "  Test 3 (name + string type): OK" << std::endl;
    }

    // Test 4: Case-insensitive type parsing
    {
        Layer l("mask_layer", "MASK");
        assert(l.type() == Layer::MASK);

        l.set_type("bias");
        assert(l.type() == Layer::BIAS);

        l.set_type("Probability");
        assert(l.type() == Layer::PROBABILITY);

        l.set_type("cumulative");
        assert(l.type() == Layer::CUMULATIVE);

        l.set_type("debiasavg");
        assert(l.type() == Layer::DEBIAS_AVG);
        std::cout << "  Test 4 (case-insensitive type): OK" << std::endl;
    }

    // Test 5: Unknown type string
    {
        Layer l("unknown", "SomethingElse");
        assert(l.type() == Layer::UNKNOWN);
        std::cout << "  Test 5 (unknown type string): OK" << std::endl;
    }

    // Test 6: All type constants
    {
        assert(Layer::type_name(0) == "Unknown");
        assert(Layer::type_name(1) == "Continuous");
        assert(Layer::type_name(2) == "Categorical");
        assert(Layer::type_name(3) == "Bias");
        assert(Layer::type_name(4) == "Mask");
        assert(Layer::type_name(5) == "Probability");
        assert(Layer::type_name(6) == "Cumulative");
        assert(Layer::type_name(7) == "DebiasAvg");
        assert(Layer::type_name(99) == "Unknown");
        std::cout << "  Test 6 (all type names): OK" << std::endl;
    }

    // Test 7: name_from_path
    {
        assert(Layer::name_from_path("/data/bio1.asc") == "bio1");
        assert(Layer::name_from_path("C:\\data\\precip.grd") == "precip");
        assert(Layer::name_from_path("temperature.tif") == "temperature");
        assert(Layer::name_from_path("noext") == "noext");
        assert(Layer::name_from_path("/path/to/elev.bil") == "elev");
        std::cout << "  Test 7 (name_from_path): OK" << std::endl;
    }

    // Test 8: set_name
    {
        Layer l("old", Layer::CONTINUOUS);
        l.set_name("new_name");
        assert(l.name() == "new_name");
        std::cout << "  Test 8 (set_name): OK" << std::endl;
    }

    std::cout << "All Layer tests passed!" << std::endl;
    return 0;
}
