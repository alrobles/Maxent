# Find Google Test (optional for now)
# For initial development, we'll create a simple test executable

add_executable(test_grid test_grid.cpp)
target_link_libraries(test_grid maxent)

add_executable(test_sample test_sample.cpp)
target_link_libraries(test_sample maxent)

# Add tests
add_test(NAME GridTest COMMAND test_grid)
add_test(NAME SampleTest COMMAND test_sample)
