cmake_minimum_required(VERSION 3.15)
project(MaxentCPP VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Find dependencies
find_package(Eigen3 REQUIRED NO_MODULE)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${EIGEN3_INCLUDE_DIR}
)

# Source files
set(MAXENT_SOURCES
    src/sample.cpp
    src/grid.cpp
    src/grid_dimension.cpp
    src/featured_space.cpp
    src/layer.cpp
    src/csv_reader.cpp
    src/csv_writer.cpp
    src/grid_io.cpp
    src/model_evaluation.cpp
    src/projection.cpp
    src/variable_importance.cpp
    src/response_curve.cpp
    src/clamping.cpp
    src/novelty.cpp
)

# Create library
add_library(maxent ${MAXENT_SOURCES})
target_link_libraries(maxent PUBLIC Eigen3::Eigen)

# Set include directories for the library
target_include_directories(maxent PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS maxent
    EXPORT MaxentTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)
