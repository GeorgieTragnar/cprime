#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <exception>

namespace cprime::testing {

/**
 * Minimalistic Test Framework for CPrime Compiler
 * 
 * Provides clean success/verbose failure testing pattern:
 * - Success: ✓ Test Name
 * - Failure: ✗ Test Name - Reason + full debug dump
 * 
 * Usage:
 *   bool my_test() {
 *       TestLogger logger("My Test Description");
 *       
 *       try {
 *           logger << "Debug info: " << some_value << "\n";
 *           logger << "More debug info...\n";
 *           
 *           if (test_condition) {
 *               TEST_SUCCESS(logger);
 *           } else {
 *               TEST_FAILURE(logger, "Condition failed");
 *           }
 *       } catch (const std::exception& e) {
 *           logger.test_exception(e);
 *           return false;
 *       }
 *   }
 */
class TestLogger {
private:
    std::stringstream debug_stream;
    const char* test_name;
    
public:
    explicit TestLogger(const char* name) : test_name(name) {}
    
    /// Stream operator for collecting debug information
    template<typename T>
    TestLogger& operator<<(const T& value) {
        debug_stream << value;
        return *this;
    }
    
    /// Call on test success - outputs minimal success message
    void test_success() {
        std::cout << "✓ " << test_name << "\n";
    }
    
    /// Call on test failure - outputs failure message + full debug dump
    void test_failure(const std::string& reason = "") {
        std::cout << "✗ " << test_name;
        if (!reason.empty()) {
            std::cout << " - " << reason;
        }
        std::cout << "\n";
        
        // Dump all collected debug information
        std::string debug_content = debug_stream.str();
        if (!debug_content.empty()) {
            std::cout << debug_content;
            if (debug_content.back() != '\n') {
                std::cout << "\n";
            }
        }
    }
    
    /// Call on test exception - converts exception to failure
    void test_exception(const std::exception& e) {
        test_failure("Exception: " + std::string(e.what()));
    }
    
    /// Get the debug stream contents (for advanced usage)
    std::string get_debug_content() const {
        return debug_stream.str();
    }
    
    /// Clear the debug stream (for reuse)
    void clear_debug() {
        debug_stream.str("");
        debug_stream.clear();
    }
};

/**
 * Test Suite Runner - aggregates multiple test results
 */
class TestSuite {
private:
    std::string suite_name;
    int passed = 0;
    int total = 0;
    
public:
    explicit TestSuite(const std::string& name) : suite_name(name) {}
    
    /// Run a test function and track results
    void run_test(bool (*test_func)()) {
        total++;
        if (test_func()) {
            passed++;
        }
    }
    
    /// Print final test suite results
    void print_results() {
        std::cout << "\n=== " << suite_name << " Results ===\n";
        std::cout << "Passed: " << passed << "/" << total << "\n";
        
        if (passed == total) {
            std::cout << "✓ All tests passed!\n";
        } else {
            std::cout << "✗ " << (total - passed) << " test(s) failed\n";
        }
    }
    
    /// Get pass/fail status
    bool all_passed() const { return passed == total; }
    int get_passed() const { return passed; }
    int get_total() const { return total; }
};

} // namespace cprime::testing

// Helper macros for clean test control flow
#define TEST_SUCCESS(logger) \
    do { logger.test_success(); return true; } while(0)

#define TEST_FAILURE(logger, reason) \
    do { logger.test_failure(reason); return false; } while(0)

// Convenience macro for try-catch test pattern
#define TEST_WITH_EXCEPTION_HANDLING(logger, test_code) \
    do { \
        try { \
            test_code \
        } catch (const std::exception& e) { \
            logger.test_exception(e); \
            return false; \
        } \
    } while(0)
