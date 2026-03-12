#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>
#include <string>
#include <cstdio>
#include "maxent/csv_writer.hpp"
#include "maxent/csv_reader.hpp"

static bool near(double a, double b, double tol = 1e-4) {
    return std::fabs(a - b) < tol;
}

int main() {
    using namespace maxent;

    std::cout << "Testing CsvWriter..." << std::endl;

    // Test 1: Basic CSV writing
    {
        std::string path = "/tmp/test_csvw1.csv";
        {
            CsvWriter writer(path);
            writer.print("species", "oak");
            writer.print("longitude", -118.5);
            writer.print("latitude", 36.5);
            writer.println();

            writer.print("species", "pine");
            writer.print("longitude", -119.0);
            writer.print("latitude", 37.0);
            writer.println();
            writer.close();
        }

        // Verify with CsvReader
        CsvReader reader(path);
        assert(reader.headers().size() == 3);
        assert(reader.headers()[0] == "species");

        assert(reader.next_record());
        assert(reader.get("species") == "oak");
        assert(near(reader.get_double("longitude"), -118.5));

        assert(reader.next_record());
        assert(reader.get("species") == "pine");
        assert(near(reader.get_double("latitude"), 37.0));

        assert(!reader.next_record());
        std::cout << "  Test 1 (basic write + read): OK" << std::endl;
        std::remove(path.c_str());
    }

    // Test 2: Integer values
    {
        std::string path = "/tmp/test_csvw2.csv";
        {
            CsvWriter writer(path);
            writer.print("id", 1);
            writer.print("count", 42);
            writer.println();

            writer.print("id", 2);
            writer.print("count", 100);
            writer.println();
            writer.close();
        }

        CsvReader reader(path);
        assert(reader.next_record());
        assert(reader.get("id") == "1");
        assert(reader.get("count") == "42");

        assert(reader.next_record());
        assert(reader.get("id") == "2");
        assert(reader.get("count") == "100");
        std::cout << "  Test 2 (integer values): OK" << std::endl;
        std::remove(path.c_str());
    }

    // Test 3: Values with commas get quoted
    {
        std::string path = "/tmp/test_csvw3.csv";
        {
            CsvWriter writer(path);
            writer.print("name", std::string("has, comma"));
            writer.print("val", 1.0);
            writer.println();
            writer.close();
        }

        CsvReader reader(path);
        assert(reader.next_record());
        assert(reader.get("name") == "has, comma");
        std::cout << "  Test 3 (comma quoting): OK" << std::endl;
        std::remove(path.c_str());
    }

    // Test 4: Empty rows are not written
    {
        std::string path = "/tmp/test_csvw4.csv";
        {
            CsvWriter writer(path);
            writer.print("a", 1);
            writer.println();
            writer.println();  // empty row - should be skipped
            writer.print("a", 2);
            writer.println();
            writer.close();
        }

        CsvReader reader(path);
        assert(reader.next_record());
        assert(reader.get(0) == "1");
        assert(reader.next_record());
        assert(reader.get(0) == "2");
        assert(!reader.next_record());
        std::cout << "  Test 4 (empty rows skipped): OK" << std::endl;
        std::remove(path.c_str());
    }

    // Test 5: Column access
    {
        std::string path = "/tmp/test_csvw5.csv";
        CsvWriter writer(path);
        writer.print("x", 1.0);
        writer.print("y", 2.0);
        writer.println();
        writer.close();

        assert(writer.columns().size() == 2);
        assert(writer.columns()[0] == "x");
        assert(writer.columns()[1] == "y");
        std::cout << "  Test 5 (column access): OK" << std::endl;
        std::remove(path.c_str());
    }

    std::cout << "All CsvWriter tests passed!" << std::endl;
    return 0;
}
