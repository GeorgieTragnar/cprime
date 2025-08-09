#include "test_common.h"

using namespace cprime;

// ============================================================================
// Basic Layer 3 Contextualization Tests - Minimal Functionality Only
// ============================================================================

class ContextualizationTest : public CPrimeTest {
protected:
    void SetUp() override {
        CPrimeTest::SetUp();
        string_table_.clear();
    }
    
    StructuredTokens buildAndContextualize(const std::string& code) {
        // Layer 1: Tokenize
        RawTokenizer tokenizer(code, string_table_);
        auto tokens = tokenizer.tokenize();
        
        // Layer 2: Build structure
        StructureBuilder builder(tokens, string_table_);
        auto structured = builder.build_structure();
        
        // Layer 3: Contextualize
        Contextualizer contextualizer(string_table_);
        contextualizer.contextualize(structured);
        
        return structured;
    }
    
private:
    StringTable string_table_;
};

// ============================================================================
// Core Contextualization Tests
// ============================================================================

TEST_F(ContextualizationTest, ContextualizesRuntimeKeyword) {
    auto structured = buildAndContextualize("runtime x = 5;");
    
    EXPECT_TRUE(structured.is_contextualized());
    
    // Check root scope content for contextualized runtime
    const auto& content = structured.scopes[0].content;
    EXPECT_FALSE(content.empty());
    
    // First token should be contextualized runtime
    auto contextual_kind = static_cast<ContextualTokenKind>(content[0]);
    // Should resolve to variable declaration context by default
    EXPECT_EQ(contextual_kind, ContextualTokenKind::RUNTIME_VARIABLE_DECL);
}

TEST_F(ContextualizationTest, ContextualizesClassKeyword) {
    auto structured = buildAndContextualize("class Test {}");
    
    EXPECT_TRUE(structured.is_contextualized());
    
    // Check class scope signature
    const auto& signature = structured.scopes[1].signature_tokens;
    EXPECT_FALSE(signature.empty());
    
    // First token should be contextualized class
    auto contextual_kind = static_cast<ContextualTokenKind>(signature[0]);
    EXPECT_EQ(contextual_kind, ContextualTokenKind::DATA_CLASS);  // Default class type
}

TEST_F(ContextualizationTest, ContextualizesInPlace) {
    // Build structure first (not contextualized)
    RawTokenizer tokenizer("class Test {}", string_table_);
    auto tokens = tokenizer.tokenize();
    StructureBuilder builder(tokens, string_table_);
    auto structured = builder.build_structure();
    
    EXPECT_FALSE(structured.is_contextualized());
    
    // Get reference to signature before contextualization
    const auto& signature_before = structured.scopes[1].signature_tokens;
    auto original_ptr = signature_before.data();
    auto original_size = signature_before.size();
    
    // Contextualize in place
    Contextualizer contextualizer(string_table_);
    contextualizer.contextualize(structured);
    
    EXPECT_TRUE(structured.is_contextualized());
    
    // Verify same memory location and size (zero-copy)
    const auto& signature_after = structured.scopes[1].signature_tokens;
    EXPECT_EQ(signature_after.data(), original_ptr);
    EXPECT_EQ(signature_after.size(), original_size);
}

TEST_F(ContextualizationTest, PreservesStructure) {
    auto structured_before = buildAndContextualize("class Test { fn method() {} }");
    
    // Should have same scope count and hierarchy
    ASSERT_EQ(structured_before.scopes.size(), 3);  // root, class, method
    
    EXPECT_EQ(structured_before.scopes[0].type, Scope::TopLevel);
    EXPECT_EQ(structured_before.scopes[1].type, Scope::NamedClass); 
    EXPECT_EQ(structured_before.scopes[2].type, Scope::NamedFunction);
    
    // Parent relationships unchanged
    EXPECT_EQ(structured_before.scopes[1].parent_index, 0);
    EXPECT_EQ(structured_before.scopes[2].parent_index, 1);
    
    // Content sizes unchanged
    auto class_sig_size = structured_before.scopes[1].signature_tokens.size();
    auto method_sig_size = structured_before.scopes[2].signature_tokens.size();
    
    EXPECT_GT(class_sig_size, 0);
    EXPECT_GT(method_sig_size, 0);
}

TEST_F(ContextualizationTest, HandlesMultipleContextSensitiveKeywords) {
    auto structured = buildAndContextualize("runtime class Test { defer cleanup(); }");
    
    EXPECT_TRUE(structured.is_contextualized());
    
    // Should contextualize both runtime and defer appropriately
    const auto& class_signature = structured.scopes[1].signature_tokens;
    const auto& method_content = structured.scopes[1].content;
    
    // Find runtime in signature
    bool found_runtime = false;
    for (uint32_t token_value : class_signature) {
        auto kind = static_cast<ContextualTokenKind>(token_value);
        if (kind == ContextualTokenKind::RUNTIME_VARIABLE_DECL || 
            kind == ContextualTokenKind::RUNTIME_TYPE_PARAMETER) {
            found_runtime = true;
            break;
        }
    }
    
    // Find defer in content  
    bool found_defer = false;
    for (uint32_t token_value : method_content) {
        auto kind = static_cast<ContextualTokenKind>(token_value);
        if (kind == ContextualTokenKind::DEFER_RAII) {
            found_defer = true;
            break;
        }
    }
    
    EXPECT_TRUE(found_runtime);
    EXPECT_TRUE(found_defer);
}

TEST_F(ContextualizationTest, HandlesDirectMappingTokens) {
    auto structured = buildAndContextualize("x = 5 + 10;");
    
    EXPECT_TRUE(structured.is_contextualized());
    
    // Non-context-sensitive tokens should map directly
    const auto& content = structured.scopes[0].content;
    
    bool found_assign = false, found_plus = false;
    for (uint32_t token_value : content) {
        auto kind = static_cast<ContextualTokenKind>(token_value);
        if (kind == ContextualTokenKind::ASSIGN) found_assign = true;
        if (kind == ContextualTokenKind::PLUS) found_plus = true;
    }
    
    EXPECT_TRUE(found_assign);
    EXPECT_TRUE(found_plus);
}