// CPrime Compiler Test Suite - Main Runner
// Uses Google Test framework for unit and integration testing

#include <gtest/gtest.h>
#include <iostream>
#include <filesystem>
#include "../src/commons/logger.h"

// Custom test event listener for better output formatting
class CPrimeTestListener : public ::testing::EmptyTestEventListener {
    void OnTestProgramStart(const ::testing::UnitTest& /*test*/) override {
        std::cout << "\n"
                  << "========================================\n"
                  << "    CPrime Compiler Test Suite\n"
                  << "========================================\n\n";
    }

    void OnTestProgramEnd(const ::testing::UnitTest& test) override {
        std::cout << "\n"
                  << "========================================\n";
        if (test.Failed()) {
            std::cout << "    FAILED: " << test.failed_test_count() 
                     << " out of " << test.total_test_count() << " tests\n";
        } else {
            std::cout << "    SUCCESS: All " << test.total_test_count() 
                     << " tests passed!\n";
        }
        std::cout << "========================================\n\n";
    }
};

int main(int argc, char **argv) {
    try {
        // Create logs directory if it doesn't exist
        std::filesystem::create_directories("logs");
        
        // Initialize logger factory early (before any logging calls)
        cprime::LoggerFactory::initialize_selective_buffering();
        
        // Set debug log level so we can see all debug messages in tests
        cprime::LoggerFactory::set_global_level(cprime::LogLevel::Debug);
        
        // Create test logger to verify logging is working
        auto logger = cprime::LoggerFactory::get_logger("test_main");
        LOG_INFO("CPrime Test Suite - Logging system initialized");
        LOG_DEBUG("Debug logging enabled for test suite");
        
        // Initialize Google Test
        ::testing::InitGoogleTest(&argc, argv);
        
        // Add custom listener for better output
        ::testing::TestEventListeners& listeners = 
            ::testing::UnitTest::GetInstance()->listeners();
        listeners.Append(new CPrimeTestListener());
        
        // Run all tests
        int result = RUN_ALL_TESTS();
        
        LOG_INFO("Test suite completed with exit code {}", result);
        return result;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error in test suite: " << e.what() << std::endl;
        return 2;
    } catch (...) {
        std::cerr << "Unknown fatal error in test suite" << std::endl;
        return 2;
    }
}