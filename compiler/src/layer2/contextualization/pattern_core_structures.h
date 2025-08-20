#pragma once

#include "../../commons/enum/token.h"
#include "../../commons/enum/contextualToken.h"
#include <vector>
#include <string>
#include <memory>

namespace cprime::layer2_contextualization {

// Pattern element types define the building blocks of patterns
enum class PatternElementType {
    CONCRETE_TOKEN,        // Specific EToken (e.g., EToken::CLASS)
    CONCRETE_TOKEN_GROUP,  // Multiple token options (e.g., CLASS|STRUCT|PLEX)
    OPTIONAL_WHITESPACE,   // Zero or more whitespace tokens
    REQUIRED_WHITESPACE,   // One or more whitespace tokens
    NAMESPACED_IDENTIFIER, // Variable-length namespace::identifier patterns
    END_OF_PATTERN        // Tree termination marker for exact matching
};

// Individual pattern element definition
struct PatternElement {
    PatternElementType type;
    std::vector<EToken> accepted_tokens;        // For CONCRETE_TOKEN_GROUP
    EContextualToken target_contextual_token;   // What contextual token this element generates
    
    // Constructor for concrete token
    PatternElement(EToken token, EContextualToken contextual_token)
        : type(PatternElementType::CONCRETE_TOKEN), accepted_tokens({token}), target_contextual_token(contextual_token) {}
    
    // Constructor for token group
    PatternElement(const std::vector<EToken>& tokens, EContextualToken contextual_token)
        : type(PatternElementType::CONCRETE_TOKEN_GROUP), accepted_tokens(tokens), target_contextual_token(contextual_token) {}
    
    // Constructor for special pattern types
    PatternElement(PatternElementType pattern_type, EContextualToken contextual_token = EContextualToken::INVALID)
        : type(pattern_type), target_contextual_token(contextual_token) {}
};

// Complete pattern definition
struct Pattern {
    std::string pattern_name;
    std::vector<PatternElement> elements;
    
    Pattern(const std::string& name, const std::vector<PatternElement>& pattern_elements)
        : pattern_name(name), elements(pattern_elements) {}
};

// Forward declaration for tree structure
struct PatternNode;

// Tree node for pattern matching - patterns are duplicated for optional element variations
struct PatternNode {
    PatternElement element;
    std::vector<std::unique_ptr<PatternNode>> children;
    bool is_end_of_pattern = false;
    Pattern* complete_pattern = nullptr;  // Set when is_end_of_pattern = true
    
    PatternNode(const PatternElement& elem) : element(elem) {}
};

// Result of matching a single contextual token with its source token indices
struct ContextualTokenResult {
    EContextualToken contextual_token;    // TYPE_REFERENCE, SCOPE_REFERENCE, etc.
    std::vector<size_t> token_indices;    // Indices to original tokens in instruction
    
    ContextualTokenResult(EContextualToken token, const std::vector<size_t>& indices)
        : contextual_token(token), token_indices(indices) {}
};

// Complete result of pattern matching attempt
struct PatternMatchResult {
    Pattern* matched_pattern = nullptr;
    std::vector<ContextualTokenResult> contextual_tokens;
    bool success = false;
    
    PatternMatchResult() = default;
    PatternMatchResult(Pattern* pattern, const std::vector<ContextualTokenResult>& tokens)
        : matched_pattern(pattern), contextual_tokens(tokens), success(true) {}
};

} // namespace cprime::layer2_contextualization