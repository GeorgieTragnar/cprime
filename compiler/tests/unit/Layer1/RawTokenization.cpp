#include "test_common.h"
#include "layer1/raw_token.h"
#include <string>
#include <vector>

using namespace cprime;

// ============================================================================
// Raw Tokenization Tests - Layer 1
// ============================================================================

class RawTokenizationTest : public BufferedLayer1Test {
protected:
    // Helper to get token strings for easier comparison
    std::vector<std::string> getTokenStrings(const std::vector<RawToken>& tokens) {
        std::vector<std::string> result;
        result.reserve(tokens.size());
        for (const auto& token : tokens) {
            result.push_back(token.to_string());
        }
        return result;
    }
    
    // Helper to get token values only (without type info)
    std::vector<std::string> getTokenValues(const std::vector<RawToken>& tokens) {
        std::vector<std::string> result;
        result.reserve(tokens.size());
        for (const auto& token : tokens) {
            result.push_back(token.value);
        }
        return result;
    }
    
    // Helper to validate token types
    void validateTokenTypes(const std::vector<RawToken>& tokens,
                           const std::vector<RawTokenType>& expected_types,
                           const std::string& context) {
        test_logger_->debug("Validating token types for {}", context);
        
        ASSERT_EQ(tokens.size(), expected_types.size())
            << "Token type count mismatch in " << context;
        
        for (size_t i = 0; i < expected_types.size(); ++i) {
            test_logger_->debug("Checking token[{}] type: expected {}, got {}", 
                               i, static_cast<int>(expected_types[i]), static_cast<int>(tokens[i].type));
            
            EXPECT_EQ(tokens[i].type, expected_types[i])
                << "Token type mismatch at position " << i << " in " << context
                << " (expected: " << static_cast<int>(expected_types[i]) 
                << ", got: " << static_cast<int>(tokens[i].type) << ")";
        }
    }
};

// ============================================================================
// Basic Tokenization Tests
// ============================================================================

TEST_F(RawTokenizationTest, EmptyInput) {
    auto tokens = tokenizeWithLogging("", "EmptyInput");
    ASSERT_EQ(tokens.size(), 1) << "Empty input should produce EOF token";
    EXPECT_EQ(tokens[0].type, RawTokenType::EOF_TOKEN);
}

TEST_F(RawTokenizationTest, SingleIdentifier) {
    auto tokens = tokenizeWithLogging("identifier", "SingleIdentifier");
    
    ASSERT_EQ(tokens.size(), 2) << "Single identifier should produce identifier + EOF token";
    EXPECT_EQ(tokens[0].value, "identifier");
    EXPECT_EQ(tokens[0].type, RawTokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].type, RawTokenType::EOF_TOKEN);
}

TEST_F(RawTokenizationTest, BasicClassDefinition) {
    std::string test_code = "class Connection {}";
    auto tokens = tokenizeWithLogging(test_code, "BasicClassDefinition");
    
    std::vector<std::string> expected_values = {"class", "Connection", "{", "}", ""};
    std::vector<RawTokenType> expected_types = {
        RawTokenType::KEYWORD, 
        RawTokenType::IDENTIFIER, 
        RawTokenType::PUNCTUATION, 
        RawTokenType::PUNCTUATION,
        RawTokenType::EOF_TOKEN
    };
    
    validateTokenSequence(tokens, expected_values, "BasicClassDefinition");
    validateTokenTypes(tokens, expected_types, "BasicClassDefinition");
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
    std::vector<std::string> expected = {"class", "Test", "{", "}", "EOF"};
    
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
    
    auto tokens = tokenizeWithLogging(large_code, "LargeInput");
    EXPECT_GT(tokens.size(), 500) << "Large input should produce many tokens";
}

// ============================================================================
// Enhanced CPrime Language Feature Tests
// ============================================================================

TEST_F(RawTokenizationTest, KeywordRecognition) {
    std::string code = "int main void class auto true false";
    auto tokens = tokenizeWithLogging(code, "KeywordRecognition");
    
    std::vector<std::string> expected_values = {"int", "main", "void", "class", "auto", "true", "false"};
    std::vector<RawTokenType> expected_types = {
        RawTokenType::KEYWORD,     // int
        RawTokenType::IDENTIFIER,  // main (function name, not keyword)
        RawTokenType::KEYWORD,     // void
        RawTokenType::KEYWORD,     // class
        RawTokenType::KEYWORD,     // auto
        RawTokenType::KEYWORD,     // true
        RawTokenType::KEYWORD      // false
    };
    
    validateTokenSequence(tokens, expected_values, "KeywordRecognition");
    validateTokenTypes(tokens, expected_types, "KeywordRecognition");
}

TEST_F(RawTokenizationTest, OperatorRecognition) {
    std::string code = "= + - * / == != > < >= <= ++ --";
    auto tokens = tokenizeWithLogging(code, "OperatorRecognition");
    
    std::vector<std::string> expected_values = {"=", "+", "-", "*", "/", "==", "!=", ">", "<", ">=", "<=", "++", "--"};
    
    // All should be recognized as operators
    for (const auto& token : tokens) {
        EXPECT_EQ(token.type, RawTokenType::OPERATOR) 
            << "Token '" << token.value << "' should be recognized as operator";
    }
    
    validateTokenSequence(tokens, expected_values, "OperatorRecognition");
}

TEST_F(RawTokenizationTest, PunctuationRecognition) {
    std::string code = "{ } ( ) [ ] ; , :: .";
    auto tokens = tokenizeWithLogging(code, "PunctuationRecognition");
    
    std::vector<std::string> expected_values = {"{", "}", "(", ")", "[", "]", ";", ",", "::", "."};
    
    // All should be recognized as punctuation
    for (const auto& token : tokens) {
        EXPECT_EQ(token.type, RawTokenType::PUNCTUATION) 
            << "Token '" << token.value << "' should be recognized as punctuation";
    }
    
    validateTokenSequence(tokens, expected_values, "PunctuationRecognition");
}

TEST_F(RawTokenizationTest, NumericLiterals) {
    std::string code = "42 3.14159 0xFF 0b1010 123u 456l";
    auto tokens = tokenizeWithLogging(code, "NumericLiterals");
    
    EXPECT_FALSE(tokens.empty()) << "Numeric literals should be tokenized";
    
    // Check that we have at least the expected number of literals
    size_t literal_count = 0;
    for (const auto& token : tokens) {
        if (token.type == RawTokenType::LITERAL) {
            literal_count++;
            test_logger_->debug("Found numeric literal: {}", token.value);
        }
    }
    
    EXPECT_GE(literal_count, 4) << "Should tokenize at least basic numeric formats";
}

TEST_F(RawTokenizationTest, StringLiterals) {
    std::string code = R"("hello world" "escaped\"string" 'c' "multiline
string")";
    auto tokens = tokenizeWithLogging(code, "StringLiterals");
    
    EXPECT_FALSE(tokens.empty()) << "String literals should be tokenized";
    
    // Find string literals in tokens
    std::vector<std::string> found_strings;
    for (const auto& token : tokens) {
        if (token.type == RawTokenType::LITERAL && 
            (token.value.front() == '"' || token.value.front() == '\'')) {
            found_strings.push_back(token.value);
        }
    }
    
    EXPECT_GE(found_strings.size(), 2) << "Should find multiple string literals";
}

TEST_F(RawTokenizationTest, CompleteMainFunction) {
    std::string code = R"(
        int main(int argc, char* argv[]) {
            auto x = 42;
            print("Hello {}", x);
            return 0;
        }
    )";
    
    auto tokens = tokenizeWithLogging(code, "CompleteMainFunction");
    
    // Verify key components are tokenized
    auto token_values = getTokenValues(tokens);
    
    EXPECT_NE(std::find(token_values.begin(), token_values.end(), "int"), token_values.end())
        << "Should contain 'int' keyword";
    EXPECT_NE(std::find(token_values.begin(), token_values.end(), "main"), token_values.end())
        << "Should contain 'main' identifier";
    EXPECT_NE(std::find(token_values.begin(), token_values.end(), "auto"), token_values.end())
        << "Should contain 'auto' keyword";
    EXPECT_NE(std::find(token_values.begin(), token_values.end(), "print"), token_values.end())
        << "Should contain 'print' identifier";
    EXPECT_NE(std::find(token_values.begin(), token_values.end(), "return"), token_values.end())
        << "Should contain 'return' keyword";
    
    // Should have reasonable number of tokens for this complex function
    EXPECT_GT(tokens.size(), 20) << "Complex main function should produce many tokens";
}

TEST_F(RawTokenizationTest, PrintStatementWithPlaceholders) {
    std::string code = R"(print("x = {}, y = {}", x, y);)";
    auto tokens = tokenizeWithLogging(code, "PrintStatementWithPlaceholders");
    
    auto token_values = getTokenValues(tokens);
    
    EXPECT_NE(std::find(token_values.begin(), token_values.end(), "print"), token_values.end())
        << "Should contain 'print' function call";
    
    // Should find the format string with placeholders
    bool found_format_string = false;
    for (const auto& token : tokens) {
        if (token.type == RawTokenType::LITERAL && token.value.find("{}") != std::string::npos) {
            found_format_string = true;
            test_logger_->debug("Found format string with placeholders: {}", token.value);
        }
    }
    
    EXPECT_TRUE(found_format_string) << "Should find format string with placeholders";
}

TEST_F(RawTokenizationTest, VariableDeclarations) {
    std::string code = R"(
        int x = 42;
        auto y = 10; 
        bool flag = true;
        char* name = "test";
    )";
    
    auto tokens = tokenizeWithLogging(code, "VariableDeclarations");
    
    auto token_values = getTokenValues(tokens);
    
    // Check for variable declaration keywords
    EXPECT_NE(std::find(token_values.begin(), token_values.end(), "int"), token_values.end());
    EXPECT_NE(std::find(token_values.begin(), token_values.end(), "auto"), token_values.end());
    EXPECT_NE(std::find(token_values.begin(), token_values.end(), "bool"), token_values.end());
    EXPECT_NE(std::find(token_values.begin(), token_values.end(), "char"), token_values.end());
    
    // Check for assignment operators
    size_t assignment_count = std::count(token_values.begin(), token_values.end(), "=");
    EXPECT_EQ(assignment_count, 4) << "Should find 4 assignment operators";
}

// ============================================================================
// Error Condition and Edge Case Tests
// ============================================================================

TEST_F(RawTokenizationTest, MixedWhitespace) {
    std::string code = "int\t\tmain  (   )    {   return\n\n0;   }";
    auto tokens = tokenizeWithLogging(code, "MixedWhitespace");
    
    // Extract non-whitespace tokens
    std::vector<std::string> non_whitespace_values;
    for (const auto& token : tokens) {
        if (token.type != RawTokenType::WHITESPACE) {
            non_whitespace_values.push_back(token.value);
        }
    }
    
    std::vector<std::string> expected = {"int", "main", "(", ")", "{", "return", "0", ";", "}", ""};
    
    ASSERT_EQ(non_whitespace_values.size(), expected.size()) 
        << "Whitespace should not affect token recognition";
    
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(non_whitespace_values[i], expected[i])
            << "Token mismatch at position " << i << " with mixed whitespace";
    }
}

TEST_F(RawTokenizationTest, CommentsWithCode) {
    std::string code = R"(
        int main() { // Entry point
            /* Multi-line
               comment */
            return 0; // Success
        }
    )";
    
    auto tokens = tokenizeWithLogging(code, "CommentsWithCode");
    
    // Should still tokenize the actual code despite comments
    auto token_values = getTokenValues(tokens);
    
    EXPECT_NE(std::find(token_values.begin(), token_values.end(), "int"), token_values.end())
        << "Should find 'int' despite comments";
    EXPECT_NE(std::find(token_values.begin(), token_values.end(), "main"), token_values.end())
        << "Should find 'main' despite comments";
    EXPECT_NE(std::find(token_values.begin(), token_values.end(), "return"), token_values.end())
        << "Should find 'return' despite comments";
    
    // Should also preserve comments as tokens
    size_t comment_count = 0;
    for (const auto& token : tokens) {
        if (token.type == RawTokenType::COMMENT) {
            comment_count++;
            test_logger_->debug("Found comment: {}", token.value);
        }
    }
    
    EXPECT_GT(comment_count, 0) << "Should preserve comments as tokens";
}