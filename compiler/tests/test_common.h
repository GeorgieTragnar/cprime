#pragma once

// Common header for CPrime compiler tests
// Provides test utilities, fixtures, and helper functions

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <sstream>
#include <memory>

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
 */
class Layer1Test : public CPrimeTest {
protected:
    // Helper to create various token test cases
    struct TokenTestCase {
        std::string name;
        std::string input;
        size_t expected_token_count;
        std::vector<std::string> expected_tokens;  // Optional: specific tokens to check
    };
    
    std::vector<TokenTestCase> getBasicTokenTestCases() {
        return {
            {"empty", "", 0, {}},
            {"single_identifier", "foo", 1, {"foo"}},
            {"class_definition", "class Test {}", 4, {"class", "Test", "{", "}"}},
            // Add more test cases as needed
        };
    }
};

/**
 * Test fixture for Layer 2 (Context Enrichment) tests
 */
class Layer2Test : public CPrimeTest {
protected:
    // Layer 2 specific utilities
};

/**
 * Test fixture for Layer 3 (AST Building) tests
 */
class Layer3Test : public CPrimeTest {
protected:
    // AST validation helpers
    template<typename NodeType>
    bool validateASTNode(const NodeType* node) {
        return node != nullptr;
    }
};

/**
 * Test fixture for Layer 4 (RAII Injection) tests
 */
class Layer4Test : public CPrimeTest {
protected:
    // RAII specific test utilities
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

// Helper to compare token sequences
template<typename TokenContainer>
void expectTokenSequence(const TokenContainer& actual, 
                         const std::vector<std::string>& expected) {
    ASSERT_EQ(actual.size(), expected.size()) 
        << "Token count mismatch. Expected " << expected.size() 
        << " but got " << actual.size();
    
    size_t i = 0;
    for (const auto& token : actual) {
        EXPECT_EQ(token.to_string(), expected[i]) 
            << "Token mismatch at position " << i;
        ++i;
    }
}

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