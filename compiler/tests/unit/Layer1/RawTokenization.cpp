#include "test_common.h"
#include "layer1/raw_token.h"
#include <string>
#include <vector>

using namespace cprime;

// ============================================================================
// Raw Tokenization Tests - Layer 1
// ============================================================================

class RawTokenizationTest : public Layer1Test {
protected:
    // Helper to tokenize and return tokens
    std::vector<RawToken> tokenize(const std::string& code) {
        RawTokenizer tokenizer(code);
        return tokenizer.tokenize();
    }
    
    // Helper to get token strings for easier comparison
    std::vector<std::string> getTokenStrings(const std::vector<RawToken>& tokens) {
        std::vector<std::string> result;
        result.reserve(tokens.size());
        for (const auto& token : tokens) {
            result.push_back(token.to_string());
        }
        return result;
    }
};

TEST_F(RawTokenizationTest, EmptyInput) {
    auto tokens = tokenize("");
    EXPECT_TRUE(tokens.empty()) << "Empty input should produce no tokens";
}

TEST_F(RawTokenizationTest, SingleIdentifier) {
    auto tokens = tokenize("identifier");
    
    ASSERT_EQ(tokens.size(), 1) << "Single identifier should produce exactly one token";
    EXPECT_EQ(tokens[0].to_string(), "identifier");
}

TEST_F(RawTokenizationTest, BasicClassDefinition) {
    std::string test_code = "class Connection {}";
    auto tokens = tokenize(test_code);
    
    ASSERT_GE(tokens.size(), 3) << "Class definition should have at least 3 tokens";
    
    auto token_strings = getTokenStrings(tokens);
    std::vector<std::string> expected = {"class", "Connection", "{", "}"};
    
    ASSERT_EQ(token_strings.size(), expected.size()) 
        << "Token count mismatch for class definition";
    
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(token_strings[i], expected[i]) 
            << "Token mismatch at position " << i;
    }
}

TEST_F(RawTokenizationTest, ComplexSyntaxTokenization) {
    std::string test_code = R"(
        class Connection {
            handle: DbHandle,
            buffer: [u8; 4096],
            
            fn connect(url: String) -> Result<Self> {
                // Connection logic
            }
        }
    )";
    
    auto tokens = tokenize(test_code);
    
    EXPECT_FALSE(tokens.empty()) << "Complex code should produce tokens";
    EXPECT_GE(tokens.size(), 10) << "Complex syntax should produce multiple tokens";
    
    // Check for presence of key tokens
    auto token_strings = getTokenStrings(tokens);
    EXPECT_NE(std::find(token_strings.begin(), token_strings.end(), "class"), 
              token_strings.end()) << "Should contain 'class' token";
    EXPECT_NE(std::find(token_strings.begin(), token_strings.end(), "Connection"), 
              token_strings.end()) << "Should contain 'Connection' token";
    EXPECT_NE(std::find(token_strings.begin(), token_strings.end(), "fn"), 
              token_strings.end()) << "Should contain 'fn' token";
}

TEST_F(RawTokenizationTest, WhitespaceHandling) {
    std::string test_code = "  class   Test   {  }  ";
    auto tokens = tokenize(test_code);
    
    auto token_strings = getTokenStrings(tokens);
    std::vector<std::string> expected = {"class", "Test", "{", "}"};
    
    ASSERT_EQ(token_strings.size(), expected.size()) 
        << "Whitespace should not affect tokenization";
    
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(token_strings[i], expected[i]) 
            << "Token mismatch at position " << i << " with extra whitespace";
    }
}

TEST_F(RawTokenizationTest, Comments) {
    std::string test_code = R"(
        class Test {  // This is a comment
            // Another comment
            value: i32,  /* Block comment */
        }
    )";
    
    auto tokens = tokenize(test_code);
    EXPECT_FALSE(tokens.empty()) << "Code with comments should still produce tokens";
    
    auto token_strings = getTokenStrings(tokens);
    EXPECT_NE(std::find(token_strings.begin(), token_strings.end(), "class"), 
              token_strings.end()) << "Should contain 'class' despite comments";
    EXPECT_NE(std::find(token_strings.begin(), token_strings.end(), "Test"), 
              token_strings.end()) << "Should contain 'Test' despite comments";
}

TEST_F(RawTokenizationTest, SpecialCharacters) {
    std::string test_code = "value: [u8; 4096] -> Result<Self>";
    auto tokens = tokenize(test_code);
    
    EXPECT_FALSE(tokens.empty()) << "Special characters should be tokenized";
    
    auto token_strings = getTokenStrings(tokens);
    // Should contain brackets, semicolon, arrow, angle brackets
    EXPECT_NE(std::find(token_strings.begin(), token_strings.end(), "["), 
              token_strings.end()) << "Should tokenize '[' bracket";
    EXPECT_NE(std::find(token_strings.begin(), token_strings.end(), "]"), 
              token_strings.end()) << "Should tokenize ']' bracket";
    EXPECT_NE(std::find(token_strings.begin(), token_strings.end(), ";"), 
              token_strings.end()) << "Should tokenize ';' semicolon";
}

TEST_F(RawTokenizationTest, Numbers) {
    std::string test_code = "42 3.14159 0xFF 0b1010";
    auto tokens = tokenize(test_code);
    
    EXPECT_FALSE(tokens.empty()) << "Numbers should be tokenized";
    EXPECT_GE(tokens.size(), 4) << "Should tokenize all number formats";
}

TEST_F(RawTokenizationTest, Strings) {
    std::string test_code = R"("hello world" 'c' "escaped\"string")";
    auto tokens = tokenize(test_code);
    
    EXPECT_FALSE(tokens.empty()) << "Strings should be tokenized";
    EXPECT_GE(tokens.size(), 3) << "Should tokenize all string types";
}

// Note: Parameterized tests removed for now to fix compilation
// TODO: Re-add parameterized tests after fixing TokenTestCase structure

// Test for tokenization errors/edge cases
TEST_F(RawTokenizationTest, UnterminatedString) {
    std::string test_code = R"("unterminated string)";
    
    // Depending on how the tokenizer handles errors, this might:
    // 1. Throw an exception
    // 2. Create an error token
    // 3. Treat as partial token
    
    // For now, just ensure it doesn't crash
    EXPECT_NO_THROW({
        auto tokens = tokenize(test_code);
        // If no exception, at least verify we get some result
        (void)tokens;  // Suppress unused variable warning
    }) << "Tokenizer should handle unterminated strings gracefully";
}

TEST_F(RawTokenizationTest, LargeInput) {
    // Test with a larger input to ensure tokenizer scales
    std::string large_code;
    for (int i = 0; i < 100; ++i) {
        large_code += "class Test" + std::to_string(i) + " { value: i32, } ";
    }
    
    auto tokens = tokenize(large_code);
    EXPECT_GT(tokens.size(), 500) << "Large input should produce many tokens";
}