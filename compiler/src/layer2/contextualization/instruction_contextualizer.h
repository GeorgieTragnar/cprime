#pragma once

#include "../../commons/token.h"
#include "../../commons/contextualToken.h"
#include "../../commons/contextualizationError.h"
#include <vector>
#include <string>
#include <functional>

namespace cprime::layer2_contextualization {

// Pattern elements for matching token sequences
enum class PatternElement : uint32_t {
    // Specific keywords
    KEYWORD_INT = 1,
    KEYWORD_FUNC = 2,
    KEYWORD_AUTO = 3,
    KEYWORD_IF = 4,
    KEYWORD_WHILE = 5,
    KEYWORD_FOR = 6,
    KEYWORD_RETURN = 7,
    KEYWORD_EXEC = 8,
    
    // Generic token types  
    ANY_IDENTIFIER = 100,
    ANY_LITERAL = 101,
    ANY_STRING_LITERAL = 102,
    ANY_INT_LITERAL = 103,
    
    // Specific operators and punctuation
    LITERAL_ASSIGN = 200,           // =
    LITERAL_PLUS = 201,             // +
    LITERAL_MINUS = 202,            // -
    LITERAL_MULTIPLY = 203,         // *
    LITERAL_DIVIDE = 204,           // /
    LITERAL_SEMICOLON = 205,        // ;
    LITERAL_COLON = 206,            // :
    LITERAL_COMMA = 207,            // ,
    LITERAL_DOT = 208,              // .
    
    // Brackets and delimiters
    LITERAL_PAREN_L = 300,          // (
    LITERAL_PAREN_R = 301,          // )
    LITERAL_BRACE_L = 302,          // {
    LITERAL_BRACE_R = 303,          // }
    LITERAL_BRACKET_L = 304,        // [
    LITERAL_BRACKET_R = 305,        // ]
    LITERAL_LESS = 306,             // <
    LITERAL_GREATER = 307,          // >
    
    // Compound operators
    LITERAL_DOUBLE_COLON = 400,     // ::
    LITERAL_ARROW = 401,            // ->
    LITERAL_PLUS_ASSIGN = 402,      // +=
    LITERAL_MINUS_ASSIGN = 403,     // -=
    
    // Special pattern elements
    OPTIONAL_WHITESPACE = 500,      // Optional space/newline tokens (0 or more)
    REQUIRED_WHITESPACE = 501,      // Required space/newline tokens (1 or more)
    SINGLE_WHITESPACE = 502,        // Exactly one whitespace token
    MERGED_WHITESPACE = 503,        // Any sequence of whitespace merged into single unit
    EXPRESSION_TOKENS = 510,        // Variable-length expression
    TYPE_TOKEN_LIST = 511,          // Variable-length type parameter list
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

// Pattern definition for matching and contextualizing token sequences
struct ContextualizationPattern {
    std::string pattern_name;                           // Unique pattern identifier
    std::vector<PatternElement> token_pattern;          // Sequence of tokens/elements to match
    std::vector<ContextualTokenTemplate> output_templates; // Contextual tokens to generate on match
    int priority;                                       // Higher priority patterns tried first
    
    ContextualizationPattern(const std::string& name,
                           const std::vector<PatternElement>& pattern,
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

// Main class for instruction-level contextualization using pattern matching
class InstructionContextualizer {
private:
    std::vector<ContextualizationPattern> patterns_;
    
public:
    // Register a new pattern for matching
    void register_pattern(const ContextualizationPattern& pattern);
    
    // Contextualize an entire instruction using registered patterns
    std::vector<ContextualToken> contextualize_instruction(const std::vector<Token>& tokens);
    
    // Try to match a specific pattern at a given position
    PatternMatchResult try_match_pattern(const std::vector<Token>& tokens, 
                                       size_t start_pos, 
                                       const ContextualizationPattern& pattern);
    
    // Check if a token matches a pattern element
    bool token_matches_element(const Token& token, PatternElement element);
    
    // Check if a token is whitespace (space, newline, tab, etc.)
    bool is_whitespace_token(const Token& token);
    
    // Try to match whitespace patterns at current position
    WhitespaceMatchResult try_match_whitespace_pattern(const std::vector<Token>& tokens,
                                                      size_t start_pos,
                                                      PatternElement whitespace_element);
    
    // Create a contextual token from template and source tokens
    ContextualToken create_contextual_token(const ContextualTokenTemplate& token_template,
                                           const std::vector<Token>& source_tokens,
                                           size_t pattern_start_pos);
    
    // Get number of registered patterns
    size_t pattern_count() const { return patterns_.size(); }
    
    // Clear all registered patterns
    void clear_patterns() { patterns_.clear(); }
};

// Pattern setup functions for incremental development
void setup_basic_patterns(InstructionContextualizer& contextualizer);
void setup_declaration_patterns(InstructionContextualizer& contextualizer);
void setup_assignment_patterns(InstructionContextualizer& contextualizer);
void setup_function_call_patterns(InstructionContextualizer& contextualizer);
void setup_operator_patterns(InstructionContextualizer& contextualizer);
void setup_whitespace_patterns(InstructionContextualizer& contextualizer);
void setup_advanced_patterns(InstructionContextualizer& contextualizer);

} // namespace cprime::layer2_contextualization