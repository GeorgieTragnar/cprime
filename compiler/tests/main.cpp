// CPrime Compiler Test Suite - Main Runner
// Uses Google Test framework for unit and integration testing

#include <gtest/gtest.h>
#include <iostream>

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
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Add custom listener for better output
    ::testing::TestEventListeners& listeners = 
        ::testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new CPrimeTestListener());
    
    // Run all tests
    return RUN_ALL_TESTS();
}