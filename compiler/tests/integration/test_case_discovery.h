#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <optional>

namespace cprime::testing {

/**
 * Test case discovery system for Layer integration tests.
 * 
 * Automatically scans test_cases/ directory and discovers valid test cases
 * without requiring hardcoded test names. Supports future layer expansion.
 * 
 * Test Case Structure:
 * tests/integration/test_cases/
 * ├── hello_world/
 * │   ├── layer1        # Input source code
 * │   └── layer2        # Expected RawToken output
 * ├── string_literals/
 * │   ├── layer1
 * │   └── layer2
 * └── complex_operators/
 *     ├── layer1
 *     ├── layer2
 *     └── layer3        # Future layer expected output
 */

struct TestCase {
    std::string name;                           // Test case name (directory name)
    std::filesystem::path directory;            // Full path to test case directory
    std::filesystem::path layer1_file;          // Input file path
    std::filesystem::path layer2_file;          // Expected output file path
    
    // Future layer files (optional)
    std::filesystem::path layer3_file;
    std::filesystem::path layer4_file;
    
    bool has_layer3() const { return std::filesystem::exists(layer3_file); }
    bool has_layer4() const { return std::filesystem::exists(layer4_file); }
};

class TestCaseDiscovery {
public:
    // ========================================================================
    // Discovery Functions
    // ========================================================================
    
    /**
     * Discover all valid test cases for Layer 1 integration testing.
     * Scans test_cases/ directory and validates required files exist.
     * 
     * @param test_cases_dir Path to test_cases directory
     * @return Vector of valid TestCase objects for Layer 1 testing
     */
    static std::vector<TestCase> discover_layer1_test_cases(const std::filesystem::path& test_cases_dir);
    
    /**
     * Discover all valid test cases for any layer.
     * Generic discovery that works for Layer 1, 2, 3, etc.
     * 
     * @param test_cases_dir Path to test_cases directory
     * @param input_layer Layer number for input (e.g., 1 for Layer 1 tests)
     * @param output_layer Layer number for expected output (e.g., 2 for Layer 1 tests)
     * @return Vector of valid TestCase objects
     */
    static std::vector<TestCase> discover_test_cases(const std::filesystem::path& test_cases_dir, 
                                                    int input_layer, 
                                                    int output_layer);
    
    /**
     * Check if a directory is a valid test case for specific layers.
     * Validates that required layer files exist and are readable.
     * 
     * @param test_case_dir Path to potential test case directory
     * @param input_layer Required input layer number
     * @param output_layer Required output layer number
     * @return true if directory contains valid test case files
     */
    static bool is_valid_test_case(const std::filesystem::path& test_case_dir, 
                                  int input_layer, 
                                  int output_layer);
    
    // ========================================================================
    // Test Case Validation
    // ========================================================================
    
    /**
     * Validate that a test case directory structure is correct.
     * Checks file existence, readability, and basic format validation.
     * 
     * @param test_case TestCase object to validate
     * @return Empty string if valid, error message if invalid
     */
    static std::string validate_test_case(const TestCase& test_case);
    
    /**
     * Get list of all test case names in directory.
     * Simple directory listing for CLI help messages.
     * 
     * @param test_cases_dir Path to test_cases directory
     * @return Vector of test case names (directory names)
     */
    static std::vector<std::string> get_test_case_names(const std::filesystem::path& test_cases_dir);
    
    /**
     * Find specific test case by name.
     * Searches for test case directory and validates it.
     * 
     * @param test_cases_dir Path to test_cases directory
     * @param test_case_name Name of test case to find
     * @param input_layer Input layer number
     * @param output_layer Output layer number
     * @return TestCase object if found and valid, nullopt otherwise
     */
    static std::optional<TestCase> find_test_case(const std::filesystem::path& test_cases_dir,
                                                 const std::string& test_case_name,
                                                 int input_layer,
                                                 int output_layer);
    
    // ========================================================================
    // File Content Operations
    // ========================================================================
    
    /**
     * Load test case input content.
     * Reads the input layer file content as string.
     * 
     * @param test_case TestCase object with valid layer1_file
     * @return File content as string
     * @throws std::runtime_error if file cannot be read
     */
    static std::string load_input_content(const TestCase& test_case);
    
    /**
     * Load test case expected output content.
     * Reads the expected output layer file content as string.
     * 
     * @param test_case TestCase object with valid layer2_file
     * @return File content as string
     * @throws std::runtime_error if file cannot be read
     */
    static std::string load_expected_output_content(const TestCase& test_case);
    
    // ========================================================================
    // Statistics and Reporting
    // ========================================================================
    
    /**
     * Generate discovery statistics report.
     * Shows counts of valid/invalid test cases, layer coverage, etc.
     * 
     * @param test_cases_dir Path to test_cases directory
     * @return Human-readable statistics report
     */
    static std::string generate_discovery_report(const std::filesystem::path& test_cases_dir);

private:
    // Helper functions
    static bool is_directory_accessible(const std::filesystem::path& dir);
    static bool is_file_readable(const std::filesystem::path& file);
    static std::filesystem::path get_layer_file_path(const std::filesystem::path& test_case_dir, int layer);
    static TestCase create_test_case(const std::filesystem::path& test_case_dir);
    static std::string format_file_size(std::uintmax_t size);
};

} // namespace cprime::testing