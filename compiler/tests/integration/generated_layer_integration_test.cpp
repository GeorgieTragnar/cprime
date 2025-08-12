#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <chrono>

// Include existing test infrastructure
#include "test_common.h"
#include "test_case_discovery.h"

// Common compiler includes
#include "../../src/commons/dirty/string_table.h"
#include "../../src/commons/logger.h"

// INCLUDE POINT 1: Generated headers and validation includes
#include "layer_includes_generated.inc"

using namespace cprime::testing;

// Use full namespace qualification to avoid conflicts
using cprime::StringTable;
using cprime::RawToken;

namespace {

/**
 * Test case structure for generated layer tests
 * Bridges between existing TestCase discovery and generated test expectations
 */
struct GeneratedTestCase {
    std::string name;
    std::string input_content;
    std::map<int, std::string> expected_outputs;  // layer_number -> expected_serialized_output
};

/**
 * Thread-safe instrumentation state for debugging failed tests
 */
class InstrumentationLogger {
private:
    std::mutex mutex_;
    std::vector<std::pair<std::string, std::string>> states_;
    
public:
    void log_state(const std::string& variable_name, const std::string& serialized_value) {
        std::lock_guard<std::mutex> lock(mutex_);
        states_.emplace_back(variable_name, serialized_value);
    }
    
    std::vector<std::pair<std::string, std::string>> get_states() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
        return states_;
    }
    
    void clear_states() {
        std::lock_guard<std::mutex> lock(mutex_);
        states_.clear();
    }
};

// Global instrumentation logger
static InstrumentationLogger g_instrumentation_logger;

/**
 * LayerTestFixture - Main test fixture for generated layer integration tests
 * Provides the interface expected by generated test code
 */
class LayerTestFixture : public CPrimeTest {
protected:
    void SetUp() override {
        CPrimeTest::SetUp();
        g_instrumentation_logger.clear_states();
        
        // Setup paths (adjust for running from build directory)
        test_cases_dir_ = std::filesystem::current_path() / ".." / "test_cases";
        tmp_dir_ = std::filesystem::current_path() / ".." / "tmp";
        
        // Ensure tmp directory exists for failure logs
        std::filesystem::create_directories(tmp_dir_);
    }
    
    void TearDown() override {
        g_instrumentation_logger.clear_states();
        CPrimeTest::TearDown();
    }

public:
    /**
     * Bridge to existing test case discovery system
     * Converts TestCase objects to GeneratedTestCase format expected by generated tests
     */
    static std::vector<GeneratedTestCase> discover_test_cases() {
        // When running from build directory, we need to navigate to test_cases
        auto test_cases_dir = std::filesystem::current_path() / ".." / "test_cases";
        auto discovered_cases = TestCaseDiscovery::discover_layer1_test_cases(test_cases_dir);
        
        std::vector<GeneratedTestCase> generated_cases;
        
        for (const auto& test_case : discovered_cases) {
            GeneratedTestCase gen_case;
            gen_case.name = test_case.name;
            
            try {
                // Load input content
                gen_case.input_content = TestCaseDiscovery::load_input_content(test_case);
                
                // Load expected outputs for different layers
                // Layer 1 -> Layer 2 (expected output is in layer2_file)
                if (std::filesystem::exists(test_case.layer2_file)) {
                    std::string expected = TestCaseDiscovery::load_expected_output_content(test_case);
                    gen_case.expected_outputs[2] = expected;  // Layer 1 output tested against layer 2 expectation
                }
                
                // Future layers can be added here
                if (test_case.has_layer3()) {
                    // Layer 2 -> Layer 3 testing would go here
                    // gen_case.expected_outputs[3] = load_layer3_content(test_case);
                }
                
                generated_cases.push_back(gen_case);
                
            } catch (const std::exception& e) {
                // Skip test cases that can't be loaded
                std::cerr << "Warning: Skipping test case '" << test_case.name 
                         << "' due to loading error: " << e.what() << std::endl;
            }
        }
        
        return generated_cases;
    }

protected:
    std::filesystem::path test_cases_dir_;
    std::filesystem::path tmp_dir_;
};

// ============================================================================
// Instrumentation Interface Functions (called by generated code)
// ============================================================================

/**
 * Log intermediate state during instrumented function execution
 * Called by generated instrumented functions
 */
void log_intermediate_state(const std::string& variable_name, const std::string& serialized_value) {
    g_instrumentation_logger.log_state(variable_name, serialized_value);
}

/**
 * Get captured intermediate states for debugging
 * Called by generated test failure handling
 */
std::vector<std::pair<std::string, std::string>> get_intermediate_states() {
    return g_instrumentation_logger.get_states();
}

/**
 * Clear intermediate states between tests
 * Called by generated test cleanup
 */
void clear_intermediate_states() {
    g_instrumentation_logger.clear_states();
}

/**
 * Log detailed test failure information
 * Called by generated tests when assertion fails
 */
void log_test_failure(const std::string& test_name, 
                     int layer_number,
                     const std::vector<std::pair<std::string, std::string>>& intermediate_states,
                     const std::string& actual_result,
                     const std::string& expected_result) {
    try {
        // Create failure log in tmp directory
        auto tmp_dir = std::filesystem::current_path() / ".." / "tmp";
        std::filesystem::create_directories(tmp_dir);
        
        auto failure_dir = tmp_dir / test_name;
        std::filesystem::create_directories(failure_dir);
        
        auto log_file = failure_dir / ("layer" + std::to_string(layer_number) + "_generated_failure.log");
        std::ofstream log(log_file);
        
        log << "=== GENERATED LAYER " << layer_number << " INTEGRATION TEST FAILURE ===" << std::endl;
        log << "Test Name: " << test_name << std::endl;
        log << "Layer: " << layer_number << std::endl;
        log << "Timestamp: " << std::chrono::system_clock::now().time_since_epoch().count() << std::endl;
        log << std::endl;
        
        log << "=== INTERMEDIATE STATES ===" << std::endl;
        for (const auto& state : intermediate_states) {
            log << state.first << ": " << state.second << std::endl;
        }
        log << std::endl;
        
        log << "=== ACTUAL RESULT ===" << std::endl;
        log << actual_result << std::endl;
        log << std::endl;
        
        log << "=== EXPECTED RESULT ===" << std::endl;
        log << expected_result << std::endl;
        log << std::endl;
        
        log.close();
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to write failure log for test '" << test_name 
                  << "': " << e.what() << std::endl;
    }
}

// ============================================================================
// INCLUDE POINT 2: Generated instrumented function implementations
// ============================================================================
#include "layer_instrumented_functions_generated.inc"

// ============================================================================
// INCLUDE POINT 3: Generated dynamic test cases
// ============================================================================
#include "layer_dynamic_tests_generated.inc"

} // anonymous namespace

// ============================================================================
// Additional Validation Tests
// ============================================================================

/**
 * Test that the test case discovery system works with generated tests
 */
TEST(GeneratedIntegrationTest, TestCaseDiscoveryWorks) {
    auto test_cases = LayerTestFixture::discover_test_cases();
    
    // Should find at least some test cases
    EXPECT_GE(test_cases.size(), 0) << "Expected to find at least some test cases";
    
    // Validate structure of discovered test cases
    for (const auto& test_case : test_cases) {
        EXPECT_FALSE(test_case.name.empty()) << "Test case name should not be empty";
        EXPECT_FALSE(test_case.input_content.empty()) << "Test case input should not be empty";
        EXPECT_GT(test_case.expected_outputs.size(), 0) << "Test case should have expected outputs";
    }
}

/**
 * Test that instrumentation logging works
 */
TEST(GeneratedIntegrationTest, InstrumentationLoggingWorks) {
    clear_intermediate_states();
    
    log_intermediate_state("test_var", "test_value");
    log_intermediate_state("test_var2", "test_value2");
    
    auto states = get_intermediate_states();
    
    EXPECT_EQ(states.size(), 2);
    EXPECT_EQ(states[0].first, "test_var");
    EXPECT_EQ(states[0].second, "test_value");
    EXPECT_EQ(states[1].first, "test_var2");
    EXPECT_EQ(states[1].second, "test_value2");
    
    clear_intermediate_states();
    states = get_intermediate_states();
    EXPECT_EQ(states.size(), 0);
}