cmake_minimum_required(VERSION 3.16)
project(cprime VERSION 2.0.0 LANGUAGES CXX)

# C++17 is required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Setup vcpkg if available
if(EXISTS "${CMAKE_SOURCE_DIR}/../../vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/../../vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")
endif()

# Find Google Test package
find_package(GTest CONFIG)
if(GTest_FOUND)
    message(STATUS "Google Test found - tests will be built")
    set(BUILD_TESTS ON)
else()
    message(WARNING "Google Test not found - tests will be skipped")
    set(BUILD_TESTS OFF)
endif()

# Add the src subdirectory which contains the actual compiler implementation
add_subdirectory(src)

# Add tests directory if Google Test is available
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Copy examples to build directory for testing
file(COPY ${CMAKE_SOURCE_DIR}/../examples DESTINATION ${CMAKE_BINARY_DIR})