# Find Google Test (optional for now)
# For initial development, we'll create a simple test executable

add_executable(test_grid test_grid.cpp)
target_link_libraries(test_grid maxent)

add_executable(test_sample test_sample.cpp)
target_link_libraries(test_sample maxent)

add_executable(test_feature test_feature.cpp)
target_link_libraries(test_feature maxent)

add_executable(test_featured_space test_featured_space.cpp)
target_link_libraries(test_featured_space maxent)

add_executable(test_layer test_layer.cpp)
target_link_libraries(test_layer maxent)

add_executable(test_csv_reader test_csv_reader.cpp)
target_link_libraries(test_csv_reader maxent)

add_executable(test_csv_writer test_csv_writer.cpp)
target_link_libraries(test_csv_writer maxent)

add_executable(test_grid_io test_grid_io.cpp)
target_link_libraries(test_grid_io maxent)

add_executable(test_model_evaluation test_model_evaluation.cpp)
target_link_libraries(test_model_evaluation maxent)

add_executable(test_projection test_projection.cpp)
target_link_libraries(test_projection maxent)

# Add tests
add_test(NAME GridTest COMMAND test_grid)
add_test(NAME SampleTest COMMAND test_sample)
add_test(NAME FeatureTest COMMAND test_feature)
add_test(NAME FeaturedSpaceTest COMMAND test_featured_space)
add_test(NAME LayerTest COMMAND test_layer)
add_test(NAME CsvReaderTest COMMAND test_csv_reader)
add_test(NAME CsvWriterTest COMMAND test_csv_writer)
add_test(NAME GridIOTest COMMAND test_grid_io)
add_test(NAME ModelEvaluationTest COMMAND test_model_evaluation)
add_test(NAME ProjectionTest COMMAND test_projection)
