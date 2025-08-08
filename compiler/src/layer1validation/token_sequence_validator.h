#pragma once

#include "../validation_common.h"
#include "../layer1/raw_token.h"
#include <vector>
#include <unordered_set>

namespace cprime::layer1validation {

struct TokenSpan {
    std::vector<RawToken>::const_iterator begin;
    std::vector<RawToken>::const_iterator end;
    
    TokenSpan(std::vector<RawToken>::const_iterator b, std::vector<RawToken>::const_iterator e) 
        : begin(b), end(e) {}
};

/**
 * Token sequence validator for Layer 1.
 * Validates raw token sequences follow CPrime syntax rules.
 * 
 * Responsibilities:
 * - Basic syntax validation (brackets, semicolons, etc.)
 * - Token sequence rules (type specifier ordering, etc.)
 * - Keyword usage in token context
 * - Bracket/parentheses matching
 */
class TokenSequenceValidator : public validation::BaseValidator {
public:
    explicit TokenSequenceValidator(const std::vector<RawToken>& tokens);
    
    friend class SyntaxRuleChecker;
    
    // BaseValidator interface
    validation::ValidationResult validate() override;
    std::string get_validator_name() const override { return "TokenSequenceValidator"; }
    
    // Specific validation methods
    validation::ValidationResult validate_bracket_matching();
    validation::ValidationResult validate_class_declaration_syntax();
    validation::ValidationResult validate_function_declaration_syntax();
    validation::ValidationResult validate_type_specifier_sequences();
    validation::ValidationResult validate_statement_termination();
    
private:
    const std::vector<RawToken>& tokens_;
    
    // Bracket matching helpers
    struct BracketPair {
        size_t open_index;
        size_t close_index;
        char bracket_type;  // '(', '[', '{'
        
        BracketPair(size_t open, size_t close, char type) 
            : open_index(open), close_index(close), bracket_type(type) {}
    };
    
    std::vector<BracketPair> find_bracket_pairs();
    bool is_opening_bracket(const std::string& token) const;
    bool is_closing_bracket(const std::string& token) const;
    char get_bracket_type(const std::string& token) const;
    char get_matching_bracket(char opening) const;
    
    // Token sequence analysis helpers
    std::vector<TokenSpan> find_class_declarations() const;
    std::vector<TokenSpan> find_function_declarations() const;
    std::vector<TokenSpan> find_type_specifier_sequences() const;
    
    // Validation rule helpers
    bool is_valid_identifier_token(const RawToken& token) const;
    bool is_type_specifier_keyword(const std::string& keyword) const;
    bool is_access_modifier(const std::string& keyword) const;
    bool is_statement_terminator(const RawToken& token) const;
    
    // Error creation helpers
    validation::SourceLocation token_to_location(size_t token_index) const;
    validation::SourceLocation span_to_location(const TokenSpan& span) const;
    
    // Token sequence pattern validation
    validation::ValidationResult validate_class_keyword_sequence(const TokenSpan& tokens);
    validation::ValidationResult validate_function_keyword_sequence(const TokenSpan& tokens);
    validation::ValidationResult validate_parameter_list_syntax(const TokenSpan& tokens);
    validation::ValidationResult validate_field_declaration_syntax(const TokenSpan& tokens);
    
    // Constants for validation
    static const std::unordered_set<std::string> TYPE_KEYWORDS;
    static const std::unordered_set<std::string> ACCESS_MODIFIERS;
    static const std::unordered_set<std::string> CLASS_KEYWORDS;
    static const std::unordered_set<std::string> FUNCTION_KEYWORDS;
};

/**
 * Syntax rule checker for specific language constructs.
 * Separated for maintainability and testing.
 */
class SyntaxRuleChecker {
public:
    // Class declaration syntax rules
    static validation::ValidationResult validate_class_syntax(const TokenSpan& tokens);
    static validation::ValidationResult validate_struct_syntax(const TokenSpan& tokens);
    static validation::ValidationResult validate_union_syntax(const TokenSpan& tokens);
    static validation::ValidationResult validate_interface_syntax(const TokenSpan& tokens);
    
    // Function declaration syntax rules
    static validation::ValidationResult validate_function_syntax(const TokenSpan& tokens);
    static validation::ValidationResult validate_constructor_syntax(const TokenSpan& tokens);
    static validation::ValidationResult validate_destructor_syntax(const TokenSpan& tokens);
    
    // Variable declaration syntax rules
    static validation::ValidationResult validate_variable_syntax(const TokenSpan& tokens);
    static validation::ValidationResult validate_field_syntax(const TokenSpan& tokens);
    static validation::ValidationResult validate_parameter_syntax(const TokenSpan& tokens);
    
    // Expression syntax rules  
    static validation::ValidationResult validate_expression_syntax(const TokenSpan& tokens);
    static validation::ValidationResult validate_type_expression_syntax(const TokenSpan& tokens);
    
private:
    // Helper methods
    static bool is_valid_identifier(const RawToken& token);
    static bool is_valid_type_specifier(const RawToken& token);
    static bool follows_naming_convention(const std::string& name, const std::string& context);
    
    // Pattern matching helpers
    static bool matches_pattern(const TokenSpan& tokens, const std::vector<std::string>& pattern);
    static std::optional<size_t> find_token(const TokenSpan& tokens, const std::string& value);
};

/**
 * Bracket and punctuation matcher.
 * Ensures all brackets, parentheses, and braces are properly matched.
 */
class BracketMatcher {
public:
    explicit BracketMatcher(const std::vector<RawToken>& tokens);
    
    validation::ValidationResult validate_matching();
    
    // Query matched pairs
    const std::vector<std::pair<size_t, size_t>>& get_parentheses_pairs() const { return paren_pairs_; }
    const std::vector<std::pair<size_t, size_t>>& get_bracket_pairs() const { return bracket_pairs_; }
    const std::vector<std::pair<size_t, size_t>>& get_brace_pairs() const { return brace_pairs_; }
    
private:
    const std::vector<RawToken>& tokens_;
    std::vector<std::pair<size_t, size_t>> paren_pairs_;      // ( )
    std::vector<std::pair<size_t, size_t>> bracket_pairs_;    // [ ]
    std::vector<std::pair<size_t, size_t>> brace_pairs_;      // { }
    
    validation::ValidationResult match_brackets_of_type(char open_char, char close_char, 
                                           std::vector<std::pair<size_t, size_t>>& pairs);
    
    validation::SourceLocation token_location(size_t index) const;
};

} // namespace cprime::layer1validation