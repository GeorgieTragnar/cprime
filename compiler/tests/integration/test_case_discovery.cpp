#include "test_case_discovery.h"
#include "../../src/commons/logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace cprime::testing {

// ============================================================================
// Discovery Functions
// ============================================================================

std::vector<TestCase> TestCaseDiscovery::discover_layer1_test_cases(const std::filesystem::path& test_cases_dir) {
    auto logger = cprime::LoggerFactory::get_logger("test_discovery");
    LOG_DEBUG("discover_layer1_test_cases() - Input directory: {}", test_cases_dir.string());
    
    auto result = discover_test_cases(test_cases_dir, 1, 2);
    LOG_DEBUG("discover_layer1_test_cases() - Returning {} test cases", result.size());
    return result;
}

std::vector<TestCase> TestCaseDiscovery::discover_test_cases(const std::filesystem::path& test_cases_dir, 
                                                           int input_layer, 
                                                           int output_layer) {
    auto logger = cprime::LoggerFactory::get_logger("test_discovery");
    std::vector<TestCase> test_cases;
    
    LOG_DEBUG("discover_test_cases() - Directory: {}, input_layer: {}, output_layer: {}", 
              test_cases_dir.string(), input_layer, output_layer);
    
    bool dir_accessible = is_directory_accessible(test_cases_dir);
    LOG_DEBUG("discover_test_cases() - Directory accessible: {}", dir_accessible);
    
    if (!dir_accessible) {
        return test_cases; // Return empty vector if directory doesn't exist or isn't accessible
    }
    
    // Iterate through subdirectories
    size_t subdirs_found = 0;
    size_t valid_test_cases = 0;
    
    for (const auto& entry : std::filesystem::directory_iterator(test_cases_dir)) {
        if (entry.is_directory()) {
            subdirs_found++;
            std::string subdir_name = entry.path().filename().string();
            LOG_DEBUG("discover_test_cases() - Found subdirectory: {}", subdir_name);
            
            bool is_valid = is_valid_test_case(entry.path(), input_layer, output_layer);
            LOG_DEBUG("discover_test_cases() - {} is_valid_test_case: {}", subdir_name, is_valid);
            
            if (is_valid) {
                TestCase test_case = create_test_case(entry.path());
                test_cases.push_back(std::move(test_case));
                valid_test_cases++;
                LOG_DEBUG("discover_test_cases() - Created test case for: {}", subdir_name);
            }
        }
    }
    
    LOG_DEBUG("discover_test_cases() - Subdirectories found: {}, valid test cases: {}", 
              subdirs_found, valid_test_cases);
    
    // Sort by name for consistent ordering
    std::sort(test_cases.begin(), test_cases.end(), 
             [](const TestCase& a, const TestCase& b) { return a.name < b.name; });
    
    LOG_DEBUG("discover_test_cases() - Returning {} sorted test cases", test_cases.size());
    return test_cases;
}

bool TestCaseDiscovery::is_valid_test_case(const std::filesystem::path& test_case_dir, 
                                          int input_layer, 
                                          int output_layer) {
    if (!is_directory_accessible(test_case_dir)) {
        return false;
    }
    
    // Check required layer files exist and are readable
    auto input_file = get_layer_file_path(test_case_dir, input_layer);
    auto output_file = get_layer_file_path(test_case_dir, output_layer);
    
    return is_file_readable(input_file) && is_file_readable(output_file);
}

// ============================================================================
// Test Case Validation
// ============================================================================

std::string TestCaseDiscovery::validate_test_case(const TestCase& test_case) {
    std::ostringstream errors;
    
    // Check directory exists
    if (!is_directory_accessible(test_case.directory)) {
        errors << "Test case directory not accessible: " << test_case.directory << "\n";
    }
    
    // Check required files
    if (!is_file_readable(test_case.layer1_file)) {
        errors << "Layer 1 input file not readable: " << test_case.layer1_file << "\n";
    }
    
    if (!is_file_readable(test_case.layer2_file)) {
        errors << "Layer 2 expected output file not readable: " << test_case.layer2_file << "\n";
    }
    
    // Check file sizes (basic validation)
    if (std::filesystem::exists(test_case.layer1_file)) {
        auto size = std::filesystem::file_size(test_case.layer1_file);
        if (size == 0) {
            errors << "Layer 1 input file is empty: " << test_case.layer1_file << "\n";
        }
    }
    
    if (std::filesystem::exists(test_case.layer2_file)) {
        auto size = std::filesystem::file_size(test_case.layer2_file);
        if (size == 0) {
            errors << "Layer 2 expected output file is empty: " << test_case.layer2_file << "\n";
        }
    }
    
    return errors.str();
}

std::vector<std::string> TestCaseDiscovery::get_test_case_names(const std::filesystem::path& test_cases_dir) {
    std::vector<std::string> names;
    
    if (!is_directory_accessible(test_cases_dir)) {
        return names;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(test_cases_dir)) {
        if (entry.is_directory()) {
            names.push_back(entry.path().filename().string());
        }
    }
    
    std::sort(names.begin(), names.end());
    return names;
}

std::optional<TestCase> TestCaseDiscovery::find_test_case(const std::filesystem::path& test_cases_dir,
                                                         const std::string& test_case_name,
                                                         int input_layer,
                                                         int output_layer) {
    auto test_case_path = test_cases_dir / test_case_name;
    
    if (is_valid_test_case(test_case_path, input_layer, output_layer)) {
        return create_test_case(test_case_path);
    }
    
    return std::nullopt;
}

// ============================================================================
// File Content Operations
// ============================================================================

std::string TestCaseDiscovery::load_input_content(const TestCase& test_case) {
    std::ifstream file(test_case.layer1_file);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open input file: " + test_case.layer1_file.string());
    }
    
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

std::string TestCaseDiscovery::load_expected_output_content(const TestCase& test_case) {
    std::ifstream file(test_case.layer2_file);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open expected output file: " + test_case.layer2_file.string());
    }
    
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

// ============================================================================
// Statistics and Reporting
// ============================================================================

std::string TestCaseDiscovery::generate_discovery_report(const std::filesystem::path& test_cases_dir) {
    std::ostringstream report;
    
    report << "=== Test Case Discovery Report ===\n";
    report << "Directory: " << test_cases_dir << "\n\n";
    
    if (!is_directory_accessible(test_cases_dir)) {
        report << "ERROR: Test cases directory not accessible\n";
        return report.str();
    }
    
    // Discover Layer 1 test cases
    auto layer1_cases = discover_layer1_test_cases(test_cases_dir);
    
    report << "Layer 1 Integration Test Cases: " << layer1_cases.size() << "\n";
    
    if (layer1_cases.empty()) {
        report << "  No valid Layer 1 test cases found\n";
    } else {
        report << "\nValid Test Cases:\n";
        for (const auto& test_case : layer1_cases) {
            auto input_size = std::filesystem::exists(test_case.layer1_file) ? 
                             std::filesystem::file_size(test_case.layer1_file) : 0;
            auto output_size = std::filesystem::exists(test_case.layer2_file) ? 
                              std::filesystem::file_size(test_case.layer2_file) : 0;
            
            report << "  - " << test_case.name 
                   << " (input: " << format_file_size(input_size)
                   << ", output: " << format_file_size(output_size) << ")\n";
        }
    }
    
    // Check for invalid directories
    std::vector<std::string> invalid_cases;
    for (const auto& entry : std::filesystem::directory_iterator(test_cases_dir)) {
        if (entry.is_directory()) {
            std::string name = entry.path().filename().string();
            
            // Check if this case is already in the valid list
            bool found = std::any_of(layer1_cases.begin(), layer1_cases.end(),
                                   [&name](const TestCase& tc) { return tc.name == name; });
            
            if (!found) {
                invalid_cases.push_back(name);
            }
        }
    }
    
    if (!invalid_cases.empty()) {
        report << "\nInvalid/Incomplete Test Cases: " << invalid_cases.size() << "\n";
        for (const auto& name : invalid_cases) {
            report << "  - " << name << " (missing layer1 or layer2 files)\n";
        }
    }
    
    return report.str();
}

// ============================================================================
// Helper Functions
// ============================================================================

bool TestCaseDiscovery::is_directory_accessible(const std::filesystem::path& dir) {
    std::error_code ec;
    return std::filesystem::exists(dir, ec) && std::filesystem::is_directory(dir, ec) && !ec;
}

bool TestCaseDiscovery::is_file_readable(const std::filesystem::path& file) {
    std::error_code ec;
    if (!std::filesystem::exists(file, ec) || ec) {
        return false;
    }
    
    // Try to open the file
    std::ifstream test_stream(file);
    return test_stream.good();
}

std::filesystem::path TestCaseDiscovery::get_layer_file_path(const std::filesystem::path& test_case_dir, int layer) {
    return test_case_dir / ("layer" + std::to_string(layer));
}

TestCase TestCaseDiscovery::create_test_case(const std::filesystem::path& test_case_dir) {
    TestCase test_case;
    test_case.name = test_case_dir.filename().string();
    test_case.directory = test_case_dir;
    test_case.layer1_file = get_layer_file_path(test_case_dir, 1);
    test_case.layer2_file = get_layer_file_path(test_case_dir, 2);
    test_case.layer3_file = get_layer_file_path(test_case_dir, 3);
    test_case.layer4_file = get_layer_file_path(test_case_dir, 4);
    
    return test_case;
}

std::string TestCaseDiscovery::format_file_size(std::uintmax_t size) {
    if (size == 0) return "0B";
    if (size < 1024) return std::to_string(size) + "B";
    if (size < 1024 * 1024) return std::to_string(size / 1024) + "KB";
    return std::to_string(size / (1024 * 1024)) + "MB";
}

} // namespace cprime::testing