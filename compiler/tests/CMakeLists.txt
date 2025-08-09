# CPrime Compiler Test Suite using Google Test

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
)

# Link libraries
target_link_libraries(cprime_tests
    cprime_compiler  # The compiler library from src/
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