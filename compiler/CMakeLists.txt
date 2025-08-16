cmake_minimum_required(VERSION 3.16)

# Setup vcpkg script with CMake (note: should be placed before project() call)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/../../vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

project(cprime VERSION 2.0.0 LANGUAGES CXX)

# C++17 is required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Option to build tests (default OFF, enabled by -DBUILD_TESTS=ON)
option(BUILD_TESTS "Build Google Test suite" OFF)

# Find dependencies if tests are requested
if(BUILD_TESTS)
    find_package(GTest CONFIG)
    if(NOT GTest_FOUND)
        message(FATAL_ERROR "Google Test not found but BUILD_TESTS=ON. Install with: vcpkg install gtest")
    endif()
endif()

# Find spdlog for logging
find_package(spdlog CONFIG REQUIRED)
message(STATUS "spdlog found - logging will be available")

# Find Lua for exec block runtime
find_package(Lua REQUIRED)
message(STATUS "Lua found - exec block scripting will be available")

# Add the src subdirectory which contains the actual compiler implementation
add_subdirectory(src)

# Add tests directory if Google Test is available
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Copy examples to build directory for testing
file(COPY ${CMAKE_SOURCE_DIR}/../examples DESTINATION ${CMAKE_BINARY_DIR})