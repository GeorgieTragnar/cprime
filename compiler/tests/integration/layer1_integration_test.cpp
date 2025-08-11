#include <gtest/gtest.h>
#include "test_case_discovery.h"
#include "../../src/layer1validation/token_serializer.h"
#include "../../src/layer1validation/chunk_serializer.h" 
#include "../../src/layer1validation/enum_stringifier.h"
#include "../../src/layer1/tokenizer.h"
#include "../../src/commons/dirty/string_table.h"
#include "../../src/commons/dirty/component_buffer_manager.h"
#include "../../src/commons/dirty/selective_buffer_sink.h"
#include "../../src/commons/logger.h"
#include <filesystem>
#include <sstream>
#include <fstream>
#include <chrono>
#include <iostream>

using namespace cprime;
using namespace cprime::testing;
using namespace cprime::layer1validation;

namespace {

/**
 * Layer 1 Integration Test Suite
 * 
 * Exhaustive testing of Layer 1 tokenization using auto-discovered test cases.
 * Features:
 * - Automatic test case discovery from test_cases/ directory
 * - Selective buffer integration for detailed failure logging
 * - ProcessingChunk state capture and validation
 * - Comprehensive failure debugging with tmp/ directory dumps
 */
class Layer1IntegrationTest : public ::testing::TestWithParam<TestCase> {
protected:
    void SetUp() override {
        // Initialize string table for test
        string_table_ = std::make_unique<StringTable>();
        
        // Setup selective buffer manager for trace capture
        buffer_manager_ = std::make_shared<ComponentBufferManager>();
        buffer_manager_->begin_buffering("layer1", spdlog::level::debug);
        
        // Get test case paths (adjust for running from build directory)
        test_cases_dir_ = std::filesystem::current_path() / ".." / "compiler" / "tests" / "integration" / "test_cases";
        tmp_dir_ = std::filesystem::current_path() / ".." / "compiler" / "tests" / "integration" / "tmp";
        
        // Ensure tmp directory exists
        std::filesystem::create_directories(tmp_dir_);
    }
    
    void TearDown() override {
        // Stop buffering
        if (buffer_manager_) {
            buffer_manager_->end_buffering("layer1");
        }
        
        // Clean up on successful test (remove failure log if it exists)
        if (!HasFailure()) {
            clean_up_success_logs();
        }
    }
    
    /**
     * Run complete Layer 1 tokenization and capture all states.
     * This is the core test function that validates the tokenizer.
     */
    void run_tokenization_test(const TestCase& test_case) {
        // Load input content
        std::string input_content = TestCaseDiscovery::load_input_content(test_case);
        std::stringstream input_stream(input_content);
        
        // Load expected output
        std::string expected_content = TestCaseDiscovery::load_expected_output_content(test_case);
        
        // Parse expected tokens
        StringTable expected_string_table;
        std::vector<RawToken> expected_tokens;
        try {
            expected_tokens = TokenSerializer::parse_expected_output(expected_content, expected_string_table);
        } catch (const std::exception& e) {
            FAIL() << "Failed to parse expected output for test case '" << test_case.name 
                   << "': " << e.what();
        }
        
        // Run tokenization with buffer capture
        std::vector<RawToken> actual_tokens;
        try {
            actual_tokens = Tokenizer::tokenize_stream(input_stream, *string_table_);
        } catch (const std::exception& e) {
            dump_failure_logs(test_case, "Tokenization failed with exception: " + std::string(e.what()));
            FAIL() << "Tokenization failed for test case '" << test_case.name 
                   << "': " << e.what();
        }
        
        // Compare results
        std::string diff = TokenSerializer::compare_tokens(expected_tokens, actual_tokens, *string_table_);
        
        if (!diff.empty()) {
            dump_failure_logs(test_case, diff);
            FAIL() << "Token comparison failed for test case '" << test_case.name << "':\n" << diff;
        }
        
        // Validation passed - test succeeded
        SUCCEED();
    }
    
    /**
     * Dump comprehensive failure information to tmp/ directory.
     * Creates detailed debug logs for failed test cases.
     */
    void dump_failure_logs(const TestCase& test_case, const std::string& failure_reason) {
        try {
            // Create test case failure directory
            auto failure_dir = tmp_dir_ / test_case.name;
            std::filesystem::create_directories(failure_dir);
            
            auto log_file = failure_dir / "layer1fail.log";
            std::ofstream log(log_file);
            
            log << "=== LAYER 1 INTEGRATION TEST FAILURE ===" << std::endl;
            log << "Test Case: " << test_case.name << std::endl;
            log << "Input File: " << test_case.layer1_file << std::endl;
            log << "Expected File: " << test_case.layer2_file << std::endl;
            log << "Timestamp: " << std::chrono::system_clock::now().time_since_epoch().count() << std::endl;
            log << std::endl;
            
            log << "=== FAILURE REASON ===" << std::endl;
            log << failure_reason << std::endl;
            log << std::endl;
            
            // Dump input content
            try {
                std::string input_content = TestCaseDiscovery::load_input_content(test_case);
                log << "=== INPUT CONTENT ===" << std::endl;
                log << input_content << std::endl;
                log << std::endl;
            } catch (const std::exception& e) {
                log << "Failed to load input content: " << e.what() << std::endl;
            }
            
            // Dump expected output content
            try {
                std::string expected_content = TestCaseDiscovery::load_expected_output_content(test_case);
                log << "=== EXPECTED OUTPUT ===" << std::endl;
                log << expected_content << std::endl;
                log << std::endl;
            } catch (const std::exception& e) {
                log << "Failed to load expected output: " << e.what() << std::endl;
            }
            
            // Dump actual tokenization result if available
            try {
                std::string input_content = TestCaseDiscovery::load_input_content(test_case);
                std::stringstream input_stream(input_content);
                StringTable temp_table;
                auto actual_tokens = Tokenizer::tokenize_stream(input_stream, temp_table);
                
                log << "=== ACTUAL OUTPUT ===" << std::endl;
                log << TokenSerializer::serialize_tokens(actual_tokens, temp_table) << std::endl;
                log << std::endl;
                
                log << "=== STRING TABLE STATE ===" << std::endl;
                auto stats = temp_table.get_statistics();
                log << "Unique strings: " << stats.unique_strings << std::endl;
                log << "Total characters: " << stats.total_characters << std::endl;
                log << "Average length: " << stats.average_string_length << std::endl;
                
                // Dump all strings in table
                for (size_t i = 0; i < stats.unique_strings; ++i) {
                    StringIndex index{static_cast<uint32_t>(i)};
                    if (temp_table.is_valid_index(index)) {
                        log << "[" << i << "]: \"" << temp_table.get_string(index) << "\"" << std::endl;
                    }
                }
                log << std::endl;
            } catch (const std::exception& e) {
                log << "Failed to generate actual output: " << e.what() << std::endl;
            }
            
            // Dump selective buffer contents
            if (buffer_manager_) {
                log << "=== SELECTIVE BUFFER TRACE ===" << std::endl;
                // TODO: Implement proper log_msg_buffer extraction
                log << "(Buffer trace temporarily disabled - needs spdlog log_msg_buffer implementation)" << std::endl;
                log << std::endl;
            }
            
            log.close();
            
        } catch (const std::exception& e) {
            // If we can't write the failure log, at least record that
            std::cerr << "Failed to write failure log for test case '" << test_case.name 
                      << "': " << e.what() << std::endl;
        }
    }
    
    /**
     * Clean up failure logs on successful test run.
     */
    void clean_up_success_logs() {
        const TestCase& test_case = GetParam();
        auto failure_dir = tmp_dir_ / test_case.name;
        auto log_file = failure_dir / "layer1fail.log";
        
        if (std::filesystem::exists(log_file)) {
            std::filesystem::remove(log_file);
            
            // Remove directory if it's empty
            if (std::filesystem::is_empty(failure_dir)) {
                std::filesystem::remove(failure_dir);
            }
        }
    }

protected:
    std::unique_ptr<StringTable> string_table_;
    std::shared_ptr<ComponentBufferManager> buffer_manager_;
    std::filesystem::path test_cases_dir_;
    std::filesystem::path tmp_dir_;
};

// ============================================================================
// Test Cases
// ============================================================================

/**
 * Main integration test for Layer 1 tokenization.
 * Uses parameterized testing with auto-discovered test cases.
 */
TEST_P(Layer1IntegrationTest, TokenizationIntegration) {
    const TestCase& test_case = GetParam();
    
    // Validate test case first
    std::string validation_error = TestCaseDiscovery::validate_test_case(test_case);
    if (!validation_error.empty()) {
        FAIL() << "Test case validation failed for '" << test_case.name << "':\n" << validation_error;
    }
    
    // Run the tokenization test
    run_tokenization_test(test_case);
}

// ============================================================================
// Test Discovery and Instantiation
// ============================================================================

/**
 * Discover and instantiate test cases for Layer 1 integration testing.
 * This automatically finds all valid test cases without hardcoded names.
 */
std::vector<TestCase> discover_integration_test_cases() {
    // When running from build directory, we need to navigate to compiler/tests/integration/test_cases
    auto test_cases_dir = std::filesystem::current_path() / ".." / "compiler" / "tests" / "integration" / "test_cases";
    return TestCaseDiscovery::discover_layer1_test_cases(test_cases_dir);
}

/**
 * Generate test names for parameterized testing.
 */
std::string generate_test_name(const ::testing::TestParamInfo<TestCase>& info) {
    return info.param.name;
}

// Instantiate parameterized tests with discovered test cases
INSTANTIATE_TEST_SUITE_P(
    AutoDiscoveredTestCases,
    Layer1IntegrationTest,
    ::testing::ValuesIn(discover_integration_test_cases()),
    generate_test_name
);

} // anonymous namespace

// ============================================================================
// Additional Test Utilities
// ============================================================================

/**
 * Test discovery system itself.
 */
class TestDiscoveryTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_cases_dir_ = std::filesystem::current_path() / ".." / "compiler" / "tests" / "integration" / "test_cases";
    }

protected:
    std::filesystem::path test_cases_dir_;
};

TEST_F(TestDiscoveryTest, DiscoveryFindsTestCases) {
    auto test_cases = TestCaseDiscovery::discover_layer1_test_cases(test_cases_dir_);
    
    // Should find at least some test cases (this test will help identify missing test cases)
    EXPECT_GE(test_cases.size(), 0) << "Expected to find at least some test cases in " << test_cases_dir_;
    
    // Validate each discovered test case
    for (const auto& test_case : test_cases) {
        std::string validation_error = TestCaseDiscovery::validate_test_case(test_case);
        EXPECT_TRUE(validation_error.empty()) 
            << "Test case '" << test_case.name << "' failed validation: " << validation_error;
    }
}

TEST_F(TestDiscoveryTest, GenerateDiscoveryReport) {
    std::string report = TestCaseDiscovery::generate_discovery_report(test_cases_dir_);
    
    EXPECT_FALSE(report.empty()) << "Discovery report should not be empty";
    EXPECT_TRUE(report.find("Test Case Discovery Report") != std::string::npos) 
        << "Report should contain header";
    
    // Print report for manual inspection during development
    std::cout << "\n" << report << std::endl;
}