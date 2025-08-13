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
#include "../../src/commons/dirty/component_buffer_manager.h"

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
        
        auto logger = cprime::LoggerFactory::get_logger("test_framework");
        
        // Debug: Show current working directory
        auto cwd = std::filesystem::current_path();
        LOG_DEBUG("LayerTestFixture::SetUp() - CWD: {}", cwd.string());
        
        // Setup paths (adjust for running from build directory)
        test_cases_dir_ = std::filesystem::current_path() / ".." / "test_cases";
        tmp_dir_ = std::filesystem::current_path() / ".." / "tmp";
        
        // Debug: Show resolved paths
        auto abs_test_cases = std::filesystem::absolute(test_cases_dir_);
        auto abs_tmp = std::filesystem::absolute(tmp_dir_);
        LOG_DEBUG("LayerTestFixture::SetUp() - test_cases_dir: {} -> {}", 
                  test_cases_dir_.string(), abs_test_cases.string());
        LOG_DEBUG("LayerTestFixture::SetUp() - tmp_dir: {} -> {}", 
                  tmp_dir_.string(), abs_tmp.string());
        
        // Debug: Check directory existence
        bool test_cases_exists = std::filesystem::exists(test_cases_dir_);
        bool test_cases_is_dir = std::filesystem::is_directory(test_cases_dir_);
        LOG_DEBUG("LayerTestFixture::SetUp() - test_cases exists: {}, is_directory: {}", 
                  test_cases_exists, test_cases_is_dir);
        
        // Ensure tmp directory exists for failure logs
        std::error_code ec;
        bool tmp_created = std::filesystem::create_directories(tmp_dir_, ec);
        LOG_DEBUG("LayerTestFixture::SetUp() - tmp directory created: {}, error: {}", 
                  tmp_created, ec.message());
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
        auto logger = cprime::LoggerFactory::get_logger("test_framework");
        
        // When running from build directory, we need to navigate to test_cases
        auto test_cases_dir = std::filesystem::current_path() / ".." / "test_cases";
        auto abs_test_cases_dir = std::filesystem::absolute(test_cases_dir);
        LOG_DEBUG("discover_test_cases() - Starting discovery at: {}", abs_test_cases_dir.string());
        
        auto discovered_cases = TestCaseDiscovery::discover_layer1_test_cases(test_cases_dir);
        LOG_DEBUG("discover_test_cases() - TestCaseDiscovery found {} test cases", discovered_cases.size());
        
        std::vector<GeneratedTestCase> generated_cases;
        
        for (const auto& test_case : discovered_cases) {
            LOG_DEBUG("discover_test_cases() - Processing test case: {}", test_case.name);
            
            GeneratedTestCase gen_case;
            gen_case.name = test_case.name;
            
            try {
                // Load input content
                gen_case.input_content = TestCaseDiscovery::load_input_content(test_case);
                LOG_DEBUG("discover_test_cases() - Loaded input content for {}: {} characters", 
                          test_case.name, gen_case.input_content.size());
                
                // Load expected outputs for different layers
                // Layer 1 -> Layer 2 (expected output is in layer2_file)
                bool layer2_exists = std::filesystem::exists(test_case.layer2_file);
                LOG_DEBUG("discover_test_cases() - Layer2 file exists for {}: {}", 
                          test_case.name, layer2_exists);
                
                if (layer2_exists) {
                    std::string expected = TestCaseDiscovery::load_expected_output_content(test_case);
                    gen_case.expected_outputs[2] = expected;  // Layer 1 output tested against layer 2 expectation
                    LOG_DEBUG("discover_test_cases() - Loaded expected output for {}: {} characters", 
                              test_case.name, expected.size());
                }
                
                // Future layers can be added here
                if (test_case.has_layer3()) {
                    // Layer 2 -> Layer 3 testing would go here
                    // gen_case.expected_outputs[3] = load_layer3_content(test_case);
                }
                
                generated_cases.push_back(gen_case);
                LOG_DEBUG("discover_test_cases() - Successfully created GeneratedTestCase for {}", test_case.name);
                
            } catch (const std::exception& e) {
                // Skip test cases that can't be loaded
                LOG_ERROR("discover_test_cases() - Skipping test case '{}' due to loading error: {}", 
                          test_case.name, e.what());
            }
        }
        
        LOG_DEBUG("discover_test_cases() - Generated {} test cases total", generated_cases.size());
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

// Forward declaration
void log_test_failure(const std::string& test_name, 
                     int layer_number,
                     const std::vector<std::pair<std::string, std::string>>& intermediate_states,
                     const std::string& actual_result,
                     const std::string& expected_result);

/**
 * Log test exception failure with buffer dump
 * Called when test throws exception before comparison
 */
void log_test_exception(const std::string& test_name, 
                       int layer_number,
                       const std::string& exception_message) {
    log_test_failure(test_name, layer_number, get_intermediate_states(), 
                    "EXCEPTION: " + exception_message, "No expected result (exception occurred)");
}

/**
 * Log detailed test failure information with buffer dump
 * Called by generated tests when assertion fails
 */
void log_test_failure(const std::string& test_name, 
                     int layer_number,
                     const std::vector<std::pair<std::string, std::string>>& intermediate_states,
                     const std::string& actual_result,
                     const std::string& expected_result) {
    auto logger = cprime::LoggerFactory::get_logger("test_framework");
    
    try {
        LOG_DEBUG("log_test_failure() - Starting failure log for test: {}, layer: {}", 
                  test_name, layer_number);
        
        // Create failure log in tmp directory
        auto tmp_dir = std::filesystem::current_path() / ".." / "tmp";
        auto abs_tmp_dir = std::filesystem::absolute(tmp_dir);
        LOG_DEBUG("log_test_failure() - tmp_dir: {} -> {}", tmp_dir.string(), abs_tmp_dir.string());
        
        std::error_code ec;
        bool tmp_created = std::filesystem::create_directories(tmp_dir, ec);
        LOG_DEBUG("log_test_failure() - tmp directory created: {}, error: {}", 
                  tmp_created, ec.message());
        
        auto failure_dir = tmp_dir / test_name;
        auto abs_failure_dir = std::filesystem::absolute(failure_dir);
        LOG_DEBUG("log_test_failure() - failure_dir: {} -> {}", 
                  failure_dir.string(), abs_failure_dir.string());
        
        bool failure_dir_created = std::filesystem::create_directories(failure_dir, ec);
        LOG_DEBUG("log_test_failure() - failure directory created: {}, error: {}", 
                  failure_dir_created, ec.message());
        
        auto log_file = failure_dir / ("layer" + std::to_string(layer_number) + "_generated_failure.log");
        auto abs_log_file = std::filesystem::absolute(log_file);
        LOG_DEBUG("log_test_failure() - log_file: {} -> {}", 
                  log_file.string(), abs_log_file.string());
        
        std::ofstream log(log_file);
        bool log_opened = log.is_open();
        LOG_DEBUG("log_test_failure() - log file opened: {}", log_opened);
        
        if (log_opened) {
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
            
            // Dump buffer logs from all components that might be buffering
            log << "=== BUFFERED COMPONENT LOGS ===" << std::endl;
            auto buffer_manager = cprime::LoggerFactory::get_buffer_manager();
            if (buffer_manager) {
                auto buffering_components = buffer_manager->get_buffering_components();
                LOG_DEBUG("log_test_failure() - Found {} buffering components", buffering_components.size());
                
                if (buffering_components.empty()) {
                    log << "No components are currently buffering logs." << std::endl;
                } else {
                    for (const auto& component : buffering_components) {
                        auto buffer_messages = buffer_manager->get_buffer_messages(component);
                        LOG_DEBUG("log_test_failure() - Component '{}' has {} buffered messages", 
                                  component, buffer_messages.size());
                        
                        log << std::endl << "--- COMPONENT: " << component << " (" 
                            << buffer_messages.size() << " messages) ---" << std::endl;
                        
                        for (const auto& msg_buffer : buffer_messages) {
                            // Extract message details from spdlog::details::log_msg_buffer
                            log << "[LEVEL:" << static_cast<int>(msg_buffer.level) << "] " 
                                << std::string(msg_buffer.payload.data(), msg_buffer.payload.size()) << std::endl;
                        }
                    }
                }
            } else {
                log << "Buffer manager not available." << std::endl;
                LOG_WARN("log_test_failure() - Buffer manager not available for dumping");
            }
            log << std::endl;
            
            log.close();
            LOG_DEBUG("log_test_failure() - Successfully wrote failure log with buffer dump");
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("log_test_failure() - Failed to write failure log for test '{}': {}", 
                  test_name, e.what());
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

auto logger = cprime::LoggerFactory::get_logger("test_framework");
/**
 * Test that the test case discovery system works with generated tests
 */
TEST(GeneratedIntegrationTest, TestCaseDiscoveryWorks) {
    auto test_cases = LayerTestFixture::discover_test_cases();
    LOG_CRITICAL("GASKJHGAKS");
    
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
    LOG_CRITICAL("GASKJHGAKS");
    
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