#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>
#include "maxent/csv_reader.hpp"

static bool near(double a, double b, double tol = 1e-6) {
    return std::fabs(a - b) < tol;
}

// Helper: write a temporary file and return its path
static std::string write_temp(const std::string& name, const std::string& content) {
    std::string path = "/tmp/" + name;
    std::ofstream out(path);
    out << content;
    out.close();
    return path;
}

int main() {
    using namespace maxent;

    std::cout << "Testing CsvReader..." << std::endl;

    // Test 1: Basic comma-separated CSV
    {
        std::string csv =
            "species,longitude,latitude\n"
            "oak,-118.5,36.5\n"
            "pine,-119.0,37.0\n"
            "fir,-120.1,38.2\n";
        auto path = write_temp("test_csv1.csv", csv);
        CsvReader reader(path);

        assert(reader.headers().size() == 3);
        assert(reader.headers()[0] == "species");
        assert(reader.headers()[1] == "longitude");
        assert(reader.headers()[2] == "latitude");
        assert(reader.has_field("species"));
        assert(reader.has_field("longitude"));
        assert(!reader.has_field("nonexistent"));
        assert(reader.field_index("latitude") == 2);

        assert(reader.next_record());
        assert(reader.get("species") == "oak");
        assert(near(reader.get_double("longitude"), -118.5));
        assert(near(reader.get_double("latitude"), 36.5));

        assert(reader.next_record());
        assert(reader.get(0) == "pine");

        assert(reader.next_record());
        assert(reader.get("species") == "fir");

        assert(!reader.next_record());  // EOF
        std::cout << "  Test 1 (basic CSV): OK" << std::endl;
    }

    // Test 2: Quoted fields with commas
    {
        std::string csv =
            "name,description,value\n"
            "\"test, item\",\"has, commas\",42.5\n"
            "normal,simple,10.0\n";
        auto path = write_temp("test_csv2.csv", csv);
        CsvReader reader(path);

        assert(reader.next_record());
        assert(reader.get("name") == "test, item");
        assert(reader.get("description") == "has, commas");
        assert(near(reader.get_double("value"), 42.5));

        assert(reader.next_record());
        assert(reader.get("name") == "normal");
        std::cout << "  Test 2 (quoted fields): OK" << std::endl;
    }

    // Test 3: European mode (semicolons, decimal commas)
    {
        std::string csv =
            "species;lon;lat\n"
            "oak;-118,5;36,5\n"
            "pine;-119,0;37,0\n";
        auto path = write_temp("test_csv3.csv", csv);
        CsvReader reader(path);

        assert(reader.headers().size() == 3);
        assert(reader.next_record());
        assert(near(reader.get_double("lon"), -118.5));
        assert(near(reader.get_double("lat"), 36.5));

        assert(reader.next_record());
        assert(near(reader.get_double("lon"), -119.0));
        std::cout << "  Test 3 (European mode): OK" << std::endl;
    }

    // Test 4: read_double_column
    {
        std::string csv =
            "x,y,temperature\n"
            "1.0,2.0,25.5\n"
            "3.0,4.0,26.0\n"
            "5.0,6.0,27.5\n";
        auto path = write_temp("test_csv4.csv", csv);
        CsvReader reader(path);

        auto temps = reader.read_double_column("temperature");
        assert(temps.size() == 3);
        assert(near(temps[0], 25.5));
        assert(near(temps[1], 26.0));
        assert(near(temps[2], 27.5));
        std::cout << "  Test 4 (read_double_column): OK" << std::endl;
    }

    // Test 5: read_all_doubles
    {
        std::string csv =
            "species,bio1,bio12\n"
            "oak,20.5,500.0\n"
            "pine,15.3,800.0\n";
        auto path = write_temp("test_csv5.csv", csv);
        CsvReader reader(path);

        auto data = reader.read_all_doubles(1);  // skip column 0 (species)
        assert(data.size() == 2);
        assert(data[0].size() == 2);
        assert(near(data[0][0], 20.5));
        assert(near(data[0][1], 500.0));
        assert(near(data[1][0], 15.3));
        assert(near(data[1][1], 800.0));
        std::cout << "  Test 5 (read_all_doubles): OK" << std::endl;
    }

    // Test 6: Auto-append .csv extension
    {
        std::string csv = "a,b\n1,2\n";
        write_temp("test_csv6.csv", csv);

        CsvReader reader("/tmp/test_csv6");  // no .csv extension
        assert(reader.headers().size() == 2);
        assert(reader.next_record());
        assert(reader.get(0) == "1");
        std::cout << "  Test 6 (auto .csv extension): OK" << std::endl;
    }

    // Test 7: Malformed lines are skipped
    {
        std::string csv =
            "a,b,c\n"
            "1,2,3\n"
            "4,5\n"        // too few fields - skipped
            "7,8,9\n";
        auto path = write_temp("test_csv7.csv", csv);
        CsvReader reader(path);

        assert(reader.next_record());
        assert(reader.get(0) == "1");

        // The malformed line (4,5) is skipped; next record is (7,8,9)
        assert(reader.next_record());
        assert(reader.get(0) == "7");

        assert(!reader.next_record());
        std::cout << "  Test 7 (skip malformed lines): OK" << std::endl;
    }

    // Test 8: Scientific notation
    {
        std::string csv =
            "val\n"
            "1.5e2\n"
            "-3.14e-1\n";
        auto path = write_temp("test_csv8.csv", csv);
        CsvReader reader(path);

        assert(reader.next_record());
        assert(near(reader.get_double(0), 150.0));

        assert(reader.next_record());
        assert(near(reader.get_double(0), -0.314));
        std::cout << "  Test 8 (scientific notation): OK" << std::endl;
    }

    // Cleanup
    std::remove("/tmp/test_csv1.csv");
    std::remove("/tmp/test_csv2.csv");
    std::remove("/tmp/test_csv3.csv");
    std::remove("/tmp/test_csv4.csv");
    std::remove("/tmp/test_csv5.csv");
    std::remove("/tmp/test_csv6.csv");
    std::remove("/tmp/test_csv7.csv");
    std::remove("/tmp/test_csv8.csv");

    std::cout << "All CsvReader tests passed!" << std::endl;
    return 0;
}
