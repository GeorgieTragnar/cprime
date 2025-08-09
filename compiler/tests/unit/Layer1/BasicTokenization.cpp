#include "test_common.h"

using namespace cprime;

// ============================================================================
// Basic Layer 1 Tokenization Tests - Minimal Functionality Only
// ============================================================================

class BasicTokenizationTest : public BufferedLayer1Test {
    // Simple test fixture for basic tokenization testing
};

// ============================================================================
// Core Tokenization Tests
// ============================================================================

TEST_F(BasicTokenizationTest, TokenizesSimpleIdentifier) {
    auto tokens = tokenize("hello");
    
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].kind, TokenKind::IDENTIFIER);
    EXPECT_EQ(tokens[1].kind, TokenKind::EOF_TOKEN);
    
    // Verify string table integration
    EXPECT_TRUE(tokens[0].has_string_value());
    EXPECT_EQ(getStringTable().get_string(tokens[0].string_index), "hello");
}

TEST_F(BasicTokenizationTest, TokenizesBasicClass) {
    auto tokens = tokenize("class Test {}");
    
    std::vector<TokenKind> expected = {
        TokenKind::CLASS, TokenKind::IDENTIFIER, 
        TokenKind::LEFT_BRACE, TokenKind::RIGHT_BRACE, 
        TokenKind::EOF_TOKEN
    };
    
    validateTokenSequence(tokens, expected, "BasicClass");
    
    // Verify identifier in string table
    EXPECT_EQ(getStringTable().get_string(tokens[1].string_index), "Test");
}

TEST_F(BasicTokenizationTest, PopulatesStringTable) {
    tokenize("hello world test");
    
    EXPECT_EQ(getStringTable().size(), 3);
    EXPECT_EQ(getStringTable().get_string(0), "hello");
    EXPECT_EQ(getStringTable().get_string(1), "world"); 
    EXPECT_EQ(getStringTable().get_string(2), "test");
}

TEST_F(BasicTokenizationTest, HandlesKeywords) {
    auto tokens = tokenize("class struct union runtime defer");
    
    std::vector<TokenKind> expected = {
        TokenKind::CLASS, TokenKind::STRUCT, TokenKind::UNION,
        TokenKind::RUNTIME, TokenKind::DEFER, TokenKind::EOF_TOKEN
    };
    
    validateTokenSequence(tokens, expected, "Keywords");
}

TEST_F(BasicTokenizationTest, HandlesOperators) {
    auto tokens = tokenize("+ - * / = == < >");
    
    std::vector<TokenKind> expected = {
        TokenKind::PLUS, TokenKind::MINUS, TokenKind::MULTIPLY, TokenKind::DIVIDE,
        TokenKind::ASSIGN, TokenKind::EQUAL_EQUAL, TokenKind::LESS_THAN, TokenKind::GREATER_THAN,
        TokenKind::EOF_TOKEN
    };
    
    validateTokenSequence(tokens, expected, "Operators");
}

TEST_F(BasicTokenizationTest, HandlesBracesAndParens) {
    auto tokens = tokenize("{ } ( ) [ ] ;");
    
    std::vector<TokenKind> expected = {
        TokenKind::LEFT_BRACE, TokenKind::RIGHT_BRACE,
        TokenKind::LEFT_PAREN, TokenKind::RIGHT_PAREN, 
        TokenKind::LEFT_BRACKET, TokenKind::RIGHT_BRACKET,
        TokenKind::SEMICOLON, TokenKind::EOF_TOKEN
    };
    
    validateTokenSequence(tokens, expected, "BracesAndParens");
}