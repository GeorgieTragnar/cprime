cmake_minimum_required(VERSION 3.16)
project(cprime VERSION 2.0.0 LANGUAGES CXX)

# C++17 is required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the src subdirectory which contains the actual compiler implementation
add_subdirectory(src)

# Copy examples to build directory for testing
file(COPY ${CMAKE_SOURCE_DIR}/../examples DESTINATION ${CMAKE_BINARY_DIR})