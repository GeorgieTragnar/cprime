#pragma once

#include "../../commons/token.h"
#include "../../commons/contextualToken.h"
#include "../../commons/contextualizationError.h"
#include <vector>
#include <string>
#include <functional>

namespace cprime::layer2_contextualization {

// Base pattern element type - each context extends this with specific ranges
enum class BasePatternElement : uint32_t {
    // Generic token types (0-999)
    ANY_IDENTIFIER = 100,
    ANY_LITERAL = 101,
    ANY_STRING_LITERAL = 102,
    ANY_INT_LITERAL = 103,
    
    // Specific operators and punctuation (200-299)
    LITERAL_ASSIGN = 200,           // =
    LITERAL_PLUS = 201,             // +
    LITERAL_MINUS = 202,            // -
    LITERAL_MULTIPLY = 203,         // *
    LITERAL_DIVIDE = 204,           // /
    LITERAL_SEMICOLON = 205,        // ;
    LITERAL_COLON = 206,            // :
    LITERAL_COMMA = 207,            // ,
    LITERAL_DOT = 208,              // .
    
    // Brackets and delimiters (300-399)
    LITERAL_PAREN_L = 300,          // (
    LITERAL_PAREN_R = 301,          // )
    LITERAL_BRACE_L = 302,          // {
    LITERAL_BRACE_R = 303,          // }
    LITERAL_BRACKET_L = 304,        // [
    LITERAL_BRACKET_R = 305,        // ]
    LITERAL_LESS = 306,             // <
    LITERAL_GREATER = 307,          // >
    
    // Compound operators (400-499)
    LITERAL_DOUBLE_COLON = 400,     // ::
    LITERAL_ARROW = 401,            // ->
    LITERAL_PLUS_ASSIGN = 402,      // +=
    LITERAL_MINUS_ASSIGN = 403,     // -=
    
    // Whitespace patterns (500-599)
    OPTIONAL_WHITESPACE = 500,      // Optional space/newline tokens (0 or more)
    REQUIRED_WHITESPACE = 501,      // Required space/newline tokens (1 or more)
    SINGLE_WHITESPACE = 502,        // Exactly one whitespace token
    MERGED_WHITESPACE = 503,        // Any sequence of whitespace merged into single unit
    
    // Comment patterns (520-529)
    OPTIONAL_COMMENT = 520,         // Optional comment token (0 or 1)
    REQUIRED_COMMENT = 521,         // Required comment token (exactly 1)
    OPTIONAL_COMMENT_AND_WHITESPACE = 522, // Optional comment + whitespace sequence
    
    // Complex patterns (600-699)
    EXPRESSION_TOKENS = 600,        // Variable-length expression
    TYPE_TOKEN_LIST = 601,          // Variable-length type parameter list
    PARAMETER_LIST = 602,           // Function parameter list
    ARGUMENT_LIST = 603,            // Function argument list
    
    // Range reservations for contexts:
    // Header patterns: 1000-1999
    // Footer patterns: 2000-2999  
    // Instruction patterns: 3000-3999
    // Advanced patterns: 4000-4999
};

// Template for generating contextual tokens from matched patterns
struct ContextualTokenTemplate {
    EContextualToken contextual_type;           // Type of contextual token to create
    std::vector<uint32_t> source_token_indices; // Which input tokens this references (relative to pattern start)
    std::string description;                     // Human-readable description for debugging
    
    ContextualTokenTemplate(EContextualToken type, 
                           const std::vector<uint32_t>& indices,
                           const std::string& desc = "")
        : contextual_type(type), source_token_indices(indices), description(desc) {}
};

// Base pattern definition - contexts extend this with their specific pattern elements
template<typename PatternElementType>
struct BaseContextualizationPattern {
    std::string pattern_name;                           // Unique pattern identifier
    std::vector<PatternElementType> token_pattern;      // Sequence of tokens/elements to match
    std::vector<ContextualTokenTemplate> output_templates; // Contextual tokens to generate on match
    int priority;                                       // Higher priority patterns tried first
    
    BaseContextualizationPattern(const std::string& name,
                                const std::vector<PatternElementType>& pattern,
                                const std::vector<ContextualTokenTemplate>& templates,
                                int prio = 0)
        : pattern_name(name), token_pattern(pattern), output_templates(templates), priority(prio) {}
};

// Result of attempting to match a pattern
struct PatternMatchResult {
    bool matched;                    // Whether pattern matched successfully
    size_t tokens_consumed;          // How many input tokens were consumed
    std::vector<ContextualToken> contextual_tokens; // Generated contextual tokens
    std::string error_message;       // Error description if match failed
    
    PatternMatchResult() : matched(false), tokens_consumed(0) {}
    
    static PatternMatchResult success(size_t consumed, const std::vector<ContextualToken>& tokens) {
        PatternMatchResult result;
        result.matched = true;
        result.tokens_consumed = consumed;
        result.contextual_tokens = tokens;
        return result;
    }
    
    static PatternMatchResult failure(const std::string& error = "") {
        PatternMatchResult result;
        result.matched = false;
        result.tokens_consumed = 0;
        result.error_message = error;
        return result;
    }
};

// Result of whitespace analysis during pattern matching
struct WhitespaceMatchResult {
    bool matched;                    // Whether whitespace pattern matched
    size_t tokens_consumed;          // How many whitespace tokens were consumed
    std::vector<uint32_t> token_indices; // Indices of all whitespace tokens matched
    std::string whitespace_type;     // Type of whitespace pattern matched
    
    WhitespaceMatchResult() : matched(false), tokens_consumed(0) {}
    
    static WhitespaceMatchResult success(size_t consumed, const std::vector<uint32_t>& indices, const std::string& type) {
        WhitespaceMatchResult result;
        result.matched = true;
        result.tokens_consumed = consumed;
        result.token_indices = indices;
        result.whitespace_type = type;
        return result;
    }
    
    static WhitespaceMatchResult failure() {
        WhitespaceMatchResult result;
        return result;
    }
};

// Base contextualizer class with shared N:M functionality
template<typename PatternElementType>
class BaseContextualizer {
protected:
    std::vector<BaseContextualizationPattern<PatternElementType>> patterns_;
    
public:
    // Register a new pattern for matching
    void register_pattern(const BaseContextualizationPattern<PatternElementType>& pattern);
    
    // Contextualize an entire instruction using registered patterns
    virtual std::vector<ContextualToken> contextualize(const std::vector<Token>& tokens);
    
    // Try to match a specific pattern at a given position
    virtual PatternMatchResult try_match_pattern(const std::vector<Token>& tokens, 
                                                size_t start_pos, 
                                                const BaseContextualizationPattern<PatternElementType>& pattern);
    
    // Try to match a pattern using preprocessed clean token indices
    virtual PatternMatchResult try_match_pattern_clean(const std::vector<Token>& tokens,
                                                      const std::vector<size_t>& clean_indices,
                                                      size_t clean_start_pos,
                                                      const BaseContextualizationPattern<PatternElementType>& pattern);
    
    // Preprocess token indices to skip comments and consolidate whitespace
    std::vector<size_t> preprocess_token_indices(const std::vector<Token>& tokens);
    
    // Check if a token matches a pattern element (context-specific implementation)
    virtual bool token_matches_element(const Token& token, PatternElementType element) = 0;
    
    // Check if a token is whitespace (space, newline, tab, etc.)
    bool is_whitespace_token(const Token& token);
    
    // Try to match whitespace patterns at current position
    WhitespaceMatchResult try_match_whitespace_pattern(const std::vector<Token>& tokens,
                                                      size_t start_pos,
                                                      PatternElementType whitespace_element);
    
    // Create a contextual token from template and source tokens
    ContextualToken create_contextual_token(const ContextualTokenTemplate& token_template,
                                           const std::vector<Token>& source_tokens,
                                           size_t pattern_start_pos);
    
    // Get number of registered patterns
    size_t pattern_count() const { return patterns_.size(); }
    
    // Clear all registered patterns
    void clear_patterns() { patterns_.clear(); }
    
protected:
    // Check if pattern element is a whitespace pattern
    virtual bool is_whitespace_pattern_element(PatternElementType element);
};

} // namespace cprime::layer2_contextualization