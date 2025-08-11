#pragma once

// Common header for CPrime compiler tests
// Provides test utilities, fixtures, and helper functions

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <sstream>
#include <memory>

// Include existing compiler components
#include "../src/commons/dirty/string_table.h"
#include "../src/commons/logger.h"

// Enable access to protected/private members for testing if needed
#ifdef TESTING
// Uncomment if needed for testing private members
// #define protected public
// #define private public
#endif

namespace cprime {
namespace testing {

// ============================================================================
// Base Test Fixtures
// ============================================================================

/**
 * Base test fixture for all CPrime compiler tests
 * Provides common setup/teardown and utility functions
 */
class CPrimeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for all tests
    }
    
    void TearDown() override {
        // Common cleanup for all tests
    }
    
    // Utility function to create test code snippets
    std::string createTestCode(const std::string& content) {
        return content;
    }
    
    // Utility to capture output for testing
    class OutputCapture {
    private:
        std::stringstream buffer;
        std::streambuf* old_cout;
        std::streambuf* old_cerr;
        
    public:
        void startCapture() {
            old_cout = std::cout.rdbuf(buffer.rdbuf());
            old_cerr = std::cerr.rdbuf(buffer.rdbuf());
        }
        
        std::string stopCapture() {
            std::cout.rdbuf(old_cout);
            std::cerr.rdbuf(old_cerr);
            return buffer.str();
        }
        
        std::string getOutput() const {
            return buffer.str();
        }
    };
};

/**
 * Test fixture for Layer 1 (Tokenization) tests
 * Simplified for integration testing
 */
class Layer1Test : public CPrimeTest {
protected:
    // Basic string table for any layer 1 testing needs
    StringTable string_table_;
    
    void SetUp() override {
        CPrimeTest::SetUp();
        string_table_.clear();
    }
    
    // Get the string table used in testing
    const StringTable& getStringTable() const { return string_table_; }
    StringTable& getStringTable() { return string_table_; }
};

/**
 * Test fixture for Layer 2 tests (future use)
 */
class Layer2Test : public CPrimeTest {
protected:
    // Layer 2 specific utilities (to be implemented)
};

/**
 * Test fixture for Layer 3 tests (future use)
 */
class Layer3Test : public CPrimeTest {
protected:
    // Layer 3 specific utilities (to be implemented)
};

/**
 * Test fixture for Layer 4 tests (future use)
 */
class Layer4Test : public CPrimeTest {
protected:
    // Layer 4 specific utilities (to be implemented)
};

/**
 * Test fixture for Integration tests
 */
class IntegrationTest : public CPrimeTest {
protected:
    // Full pipeline testing utilities
};

// ============================================================================
// Test Macros and Helpers
// ============================================================================

// Macro to test that code compiles without throwing
#define EXPECT_NO_THROW_WITH_MESSAGE(statement, message) \
    EXPECT_NO_THROW(statement) << message

// Macro to test that code throws a specific exception type
#define EXPECT_THROW_WITH_MESSAGE(statement, exception_type, message) \
    EXPECT_THROW(statement, exception_type) << message

// ============================================================================
// Test Data Generators
// ============================================================================

/**
 * Generator for various CPrime code patterns for testing
 */
class TestCodeGenerator {
public:
    static std::string simpleClass(const std::string& name = "Test") {
        return "class " + name + " {}";
    }
    
    static std::string classWithMembers(const std::string& name = "Test") {
        return R"(
            class )" + name + R"( {
                value: i32,
                name: String,
            }
        )";
    }
    
    static std::string functionDefinition(const std::string& name = "test") {
        return "fn " + name + "() -> void {}";
    }
    
    static std::string complexCode() {
        return R"(
            class Connection {
                handle: DbHandle,
                buffer: [u8; 4096],
                
                fn connect(url: String) -> Result<Self> {
                    // Connection logic
                }
                
                fn send(data: &[u8]) -> Result<usize> {
                    // Send logic
                }
            }
            
            interface Serializable {
                fn serialize() -> Vec<u8>;
                fn deserialize(data: &[u8]) -> Result<Self>;
            }
        )";
    }
};

} // namespace testing
} // namespace cprime

// Make testing utilities available without namespace prefix
using namespace cprime::testing;