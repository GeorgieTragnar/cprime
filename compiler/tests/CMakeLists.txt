# CPrime Compiler Test Suite using Google Test

# Include CMake modules for function discovery and test generation
include(${CMAKE_SOURCE_DIR}/../cmake/FunctionDiscovery.cmake)
include(${CMAKE_SOURCE_DIR}/../cmake/TestGeneration.cmake)

# Set paths for discovery system
set(COMPILER_SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(TEST_CASES_DIR "${CMAKE_SOURCE_DIR}/../test_cases")
set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")

# Create generated directory
file(MAKE_DIRECTORY ${GENERATED_DIR})

# Phase 1: Discover layer functions
set(DISCOVERY_REGISTRY "${CMAKE_BINARY_DIR}/layer_discovery.txt")
discover_layer_functions(${COMPILER_SRC_DIR} ${DISCOVERY_REGISTRY})

# Display discovery summary
display_discovery_summary(${DISCOVERY_REGISTRY})

# Phase 2: Generate the 2 files (includes + test cases)
generate_layer_test_files(${DISCOVERY_REGISTRY} ${GENERATED_DIR})

# Collect all test files
file(GLOB_RECURSE UNIT_TEST_FILES "unit/*.cpp")
file(GLOB_RECURSE INTEGRATION_TEST_FILES "integration/*.cpp")

# Create test executable
add_executable(cprime_tests 
    main.cpp
    ${UNIT_TEST_FILES}
    ${INTEGRATION_TEST_FILES}
)

# Include directories
target_include_directories(cprime_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src  # For accessing compiler headers
    ${CMAKE_CURRENT_SOURCE_DIR}  # For test_common.h
    ${GENERATED_DIR}  # For generated test files
)

# Link libraries
target_link_libraries(cprime_tests
    cprime_commons
    cprime_layer0 
    cprime_layer1
    cprime_layer2
    cprime_layer1validation
    cprime_layer0validation
    cprime_orchestrator
    GTest::gtest
    GTest::gtest_main
)

# Add compile definitions for testing
target_compile_definitions(cprime_tests PRIVATE TESTING)

# Discover tests for CTest
include(GoogleTest)
gtest_discover_tests(cprime_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    PROPERTIES LABELS "unit"
)

# Add a custom target to run tests with verbose output
add_custom_target(run_tests
    COMMAND cprime_tests --gtest_color=yes
    DEPENDS cprime_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running CPrime compiler tests"
)