#include "test_common.h"

using namespace cprime;

// ============================================================================  
// Basic Layer 2 Structure Building Tests - Minimal Functionality Only
// ============================================================================

class StructureBuildingTest : public CPrimeTest {
protected:
    void SetUp() override {
        CPrimeTest::SetUp();
        string_table_.clear();
    }
    
    StructuredTokens buildStructure(const std::string& code) {
        // Tokenize first
        RawTokenizer tokenizer(code, string_table_);
        auto tokens = tokenizer.tokenize();
        
        // Build structure
        StructureBuilder builder(tokens, string_table_);
        return builder.build_structure();
    }
    
private:
    StringTable string_table_;
};

// ============================================================================
// Core Structure Building Tests  
// ============================================================================

TEST_F(StructureBuildingTest, CreatesBasicScope) {
    auto structured = buildStructure("class Test {}");
    
    // Should have root scope + class scope
    ASSERT_EQ(structured.scopes.size(), 2);
    
    // Root scope
    EXPECT_EQ(structured.scopes[0].type, Scope::TopLevel);
    EXPECT_EQ(structured.scopes[0].parent_index, StructuredTokens::INVALID_PARENT_INDEX);
    
    // Class scope  
    EXPECT_EQ(structured.scopes[1].type, Scope::NamedClass);
    EXPECT_EQ(structured.scopes[1].parent_index, 0);  // Child of root
    
    // Class should have signature tokens
    EXPECT_FALSE(structured.scopes[1].signature_tokens.empty());
    EXPECT_TRUE(structured.scopes[1].content.empty());  // Empty class body
}

TEST_F(StructureBuildingTest, GroupsTokensByBoundaries) {
    auto structured = buildStructure("x = 5; y = 10;");
    
    // Should have only root scope
    ASSERT_EQ(structured.scopes.size(), 1);
    
    // Root scope should contain both instructions
    const auto& root_content = structured.scopes[0].content;
    EXPECT_FALSE(root_content.empty());
    
    // Count semicolons to verify instruction boundaries
    int instruction_count = 0;
    for (uint32_t token_value : root_content) {
        if (static_cast<TokenKind>(token_value) == TokenKind::SEMICOLON) {
            instruction_count++;
        }
    }
    EXPECT_EQ(instruction_count, 2);  // Two semicolons = two instructions
}

TEST_F(StructureBuildingTest, DetectsNamedScopes) {
    auto structured = buildStructure("fn test() { return; }");
    
    ASSERT_EQ(structured.scopes.size(), 2);
    
    // Function scope
    EXPECT_EQ(structured.scopes[1].type, Scope::NamedFunction);
    
    // Should have signature: fn test()
    EXPECT_FALSE(structured.scopes[1].signature_tokens.empty());
    
    // Should have content: return; 
    EXPECT_FALSE(structured.scopes[1].content.empty());
}

TEST_F(StructureBuildingTest, BuildsCorrectHierarchy) {
    auto structured = buildStructure("class Test { fn method() { x = 5; } }");
    
    // Should have: root, class, function
    ASSERT_EQ(structured.scopes.size(), 3);
    
    // Verify hierarchy
    EXPECT_EQ(structured.scopes[0].type, Scope::TopLevel);          // root
    EXPECT_EQ(structured.scopes[1].type, Scope::NamedClass);        // class  
    EXPECT_EQ(structured.scopes[2].type, Scope::NamedFunction);     // method
    
    // Verify parent relationships
    EXPECT_EQ(structured.scopes[0].parent_index, StructuredTokens::INVALID_PARENT_INDEX);
    EXPECT_EQ(structured.scopes[1].parent_index, 0);  // class parent = root
    EXPECT_EQ(structured.scopes[2].parent_index, 1);  // method parent = class
}

TEST_F(StructureBuildingTest, HandlesScopeTypes) {
    // Test different scope type detection
    auto structured = buildStructure("if (true) { x = 1; }");
    
    ASSERT_EQ(structured.scopes.size(), 2);
    EXPECT_EQ(structured.scopes[1].type, Scope::ConditionalScope);
    
    structured = buildStructure("for (i = 0; i < 10; i++) { print(i); }");
    ASSERT_EQ(structured.scopes.size(), 2);
    EXPECT_EQ(structured.scopes[1].type, Scope::LoopScope);
}

TEST_F(StructureBuildingTest, PreservesContextualizedFlag) {
    auto structured = buildStructure("class Test {}");
    
    // Structure building should not set contextualized flag
    EXPECT_FALSE(structured.is_contextualized());
    
    // Tokens should be interpretable as TokenKind  
    EXPECT_NO_THROW({
        auto kind = static_cast<TokenKind>(structured.scopes[1].signature_tokens[0]);
        (void)kind;  // Suppress unused warning
    });
}