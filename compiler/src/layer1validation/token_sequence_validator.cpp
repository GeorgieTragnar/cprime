#include "token_sequence_validator.h"
#include <algorithm>
#include <stack>

namespace cprime::layer1validation {

// Static constants for validation rules
const std::unordered_set<std::string> TokenSequenceValidator::TYPE_KEYWORDS = {
    "int", "bool", "float", "double", "char", "void", "string", "u8", "u16", "u32", "u64", 
    "i8", "i16", "i32", "i64", "f32", "f64", "usize", "isize"
};

const std::unordered_set<std::string> TokenSequenceValidator::ACCESS_MODIFIERS = {
    "public", "private", "protected", "internal"
};

const std::unordered_set<std::string> TokenSequenceValidator::CLASS_KEYWORDS = {
    "class", "struct", "union", "interface"
};

const std::unordered_set<std::string> TokenSequenceValidator::FUNCTION_KEYWORDS = {
    "fn", "async", "constexpr", "inline", "volatile"
};

// ============================================================================
// TokenSequenceValidator Implementation
// ============================================================================

TokenSequenceValidator::TokenSequenceValidator(const std::vector<RawToken>& tokens)
    : tokens_(tokens) {
}

validation::ValidationResult TokenSequenceValidator::validate() {
    validation::ValidationResult result;
    
    // Run all token sequence validations
    result.merge(validate_bracket_matching());
    result.merge(validate_class_declaration_syntax());
    result.merge(validate_function_declaration_syntax());
    result.merge(validate_type_specifier_sequences());
    result.merge(validate_statement_termination());
    
    return result;
}

validation::ValidationResult TokenSequenceValidator::validate_bracket_matching() {
    BracketMatcher matcher(tokens_);
    return matcher.validate_matching();
}

validation::ValidationResult TokenSequenceValidator::validate_class_declaration_syntax() {
    validation::ValidationResult result;
    
    auto class_declarations = find_class_declarations();
    for (const auto& decl : class_declarations) {
        result.merge(SyntaxRuleChecker::validate_class_syntax(decl));
    }
    
    return result;
}

validation::ValidationResult TokenSequenceValidator::validate_function_declaration_syntax() {
    validation::ValidationResult result;
    
    auto function_declarations = find_function_declarations();
    for (const auto& decl : function_declarations) {
        result.merge(SyntaxRuleChecker::validate_function_syntax(decl));
    }
    
    return result;
}

validation::ValidationResult TokenSequenceValidator::validate_type_specifier_sequences() {
    validation::ValidationResult result;
    
    auto type_sequences = find_type_specifier_sequences();
    for (const auto& seq : type_sequences) {
        result.merge(SyntaxRuleChecker::validate_type_expression_syntax(seq));
    }
    
    return result;
}

validation::ValidationResult TokenSequenceValidator::validate_statement_termination() {
    validation::ValidationResult result;
    
    // Simple implementation: check for statements that should end with semicolons
    for (size_t i = 0; i < tokens_.size(); ++i) {
        const auto& token = tokens_[i];
        
        // Look for statements that need semicolon termination
        if (token.value == "return" || token.value == "break" || token.value == "continue") {
            // Find the end of this statement
            size_t end = i + 1;
            while (end < tokens_.size() && tokens_[end].value != ";" && tokens_[end].value != "}") {
                end++;
            }
            
            if (end >= tokens_.size() || tokens_[end].value != ";") {
                result.add_error(
                    "Statement must be terminated with semicolon",
                    token_to_location(i),
                    "Add ';' at the end of this statement"
                );
            }
        }
    }
    
    return result;
}

// ============================================================================
// Helper Methods
// ============================================================================

std::vector<TokenSpan> TokenSequenceValidator::find_class_declarations() const {
    std::vector<TokenSpan> declarations;
    
    for (size_t i = 0; i < tokens_.size(); ++i) {
        if (CLASS_KEYWORDS.count(tokens_[i].value)) {
            // Simple implementation: class keyword to end of tokens
            size_t start = i;
            size_t end = std::min(start + 10, tokens_.size()); // Simple bound
            
            declarations.emplace_back(tokens_.begin() + start, tokens_.begin() + end);
        }
    }
    
    return declarations;
}

std::vector<TokenSpan> TokenSequenceValidator::find_function_declarations() const {
    std::vector<TokenSpan> declarations;
    
    for (size_t i = 0; i < tokens_.size(); ++i) {
        if (tokens_[i].value == "fn") {
            // Simple implementation: fn keyword to end of tokens
            size_t start = i;
            size_t end = std::min(start + 10, tokens_.size()); // Simple bound
            
            declarations.emplace_back(tokens_.begin() + start, tokens_.begin() + end);
        }
    }
    
    return declarations;
}

std::vector<TokenSpan> TokenSequenceValidator::find_type_specifier_sequences() const {
    std::vector<TokenSpan> sequences;
    
    for (size_t i = 0; i < tokens_.size(); ++i) {
        if (TYPE_KEYWORDS.count(tokens_[i].value)) {
            // Simple implementation: type keyword to next token
            size_t start = i;
            size_t end = std::min(start + 2, tokens_.size());
            
            sequences.emplace_back(tokens_.begin() + start, tokens_.begin() + end);
        }
    }
    
    return sequences;
}

bool TokenSequenceValidator::is_valid_identifier_token(const RawToken& token) const {
    return token.type == RawTokenType::IDENTIFIER;
}

bool TokenSequenceValidator::is_type_specifier_keyword(const std::string& keyword) const {
    return TYPE_KEYWORDS.count(keyword) > 0;
}

bool TokenSequenceValidator::is_access_modifier(const std::string& keyword) const {
    return ACCESS_MODIFIERS.count(keyword) > 0;
}

bool TokenSequenceValidator::is_statement_terminator(const RawToken& token) const {
    return token.value == ";";
}

validation::SourceLocation TokenSequenceValidator::token_to_location(size_t token_index) const {
    if (token_index >= tokens_.size()) {
        return validation::SourceLocation();
    }
    
    const auto& token = tokens_[token_index];
    return validation::SourceLocation(token.line, token.column, token.position, 
                                    token.position + token.value.length());
}

validation::SourceLocation TokenSequenceValidator::span_to_location(const TokenSpan& span) const {
    if (span.begin == span.end) {
        return validation::SourceLocation();
    }
    
    const auto& first = *span.begin;
    const auto& last = *(span.end - 1);
    
    return validation::SourceLocation(
        first.line, first.column, first.position, 
        last.position + last.value.length()
    );
}

// ============================================================================
// SyntaxRuleChecker Implementation
// ============================================================================

validation::ValidationResult SyntaxRuleChecker::validate_class_syntax(const TokenSpan& tokens) {
    validation::ValidationResult result;
    
    if (tokens.begin == tokens.end) {
        return result;
    }
    
    // Simple validation: check if first token is a class keyword
    if (!TokenSequenceValidator::CLASS_KEYWORDS.count(tokens.begin->value)) {
        result.add_error(
            "Expected class keyword",
            validation::SourceLocation(tokens.begin->line, tokens.begin->column, 
                                     tokens.begin->position, tokens.begin->position + tokens.begin->value.length()),
            "Use 'class', 'struct', 'union', or 'interface'"
        );
    }
    
    return result;
}

validation::ValidationResult SyntaxRuleChecker::validate_function_syntax(const TokenSpan& tokens) {
    validation::ValidationResult result;
    
    if (tokens.begin == tokens.end) {
        return result;
    }
    
    // Simple validation: check if first token is 'fn'
    if (tokens.begin->value != "fn") {
        result.add_error(
            "Expected function keyword 'fn'",
            validation::SourceLocation(tokens.begin->line, tokens.begin->column, 
                                     tokens.begin->position, tokens.begin->position + tokens.begin->value.length()),
            "Functions should start with 'fn' keyword"
        );
    }
    
    return result;
}

validation::ValidationResult SyntaxRuleChecker::validate_type_expression_syntax(const TokenSpan& tokens) {
    validation::ValidationResult result;
    
    if (tokens.begin == tokens.end) {
        return result;
    }
    
    // Simple validation: check if first token is a valid type
    if (!TokenSequenceValidator::TYPE_KEYWORDS.count(tokens.begin->value)) {
        result.add_warning(
            "Unknown type specifier: " + tokens.begin->value,
            validation::SourceLocation(tokens.begin->line, tokens.begin->column, 
                                     tokens.begin->position, tokens.begin->position + tokens.begin->value.length()),
            "Use a known type or define a custom type"
        );
    }
    
    return result;
}

bool SyntaxRuleChecker::is_valid_identifier(const RawToken& token) {
    return token.type == RawTokenType::IDENTIFIER && !token.value.empty();
}

bool SyntaxRuleChecker::is_valid_type_specifier(const RawToken& token) {
    return TokenSequenceValidator::TYPE_KEYWORDS.count(token.value) > 0;
}

bool SyntaxRuleChecker::follows_naming_convention(const std::string& name, const std::string& /* context */) {
    // Simple check: non-empty name
    return !name.empty();
}

// ============================================================================
// BracketMatcher Implementation
// ============================================================================

BracketMatcher::BracketMatcher(const std::vector<RawToken>& tokens)
    : tokens_(tokens) {
}

validation::ValidationResult BracketMatcher::validate_matching() {
    validation::ValidationResult result;
    
    // Match each type of bracket
    result.merge(match_brackets_of_type('(', ')', paren_pairs_));
    result.merge(match_brackets_of_type('[', ']', bracket_pairs_));
    result.merge(match_brackets_of_type('{', '}', brace_pairs_));
    
    return result;
}

validation::ValidationResult BracketMatcher::match_brackets_of_type(char open_char, char close_char, 
                                                      std::vector<std::pair<size_t, size_t>>& pairs) {
    validation::ValidationResult result;
    std::stack<size_t> stack;
    
    for (size_t i = 0; i < tokens_.size(); ++i) {
        const auto& token = tokens_[i];
        
        if (token.value.length() == 1) {
            if (token.value[0] == open_char) {
                stack.push(i);
            } else if (token.value[0] == close_char) {
                if (stack.empty()) {
                    result.add_error(
                        std::string("Unmatched closing bracket: ") + close_char,
                        token_location(i),
                        std::string("Add matching opening bracket: ") + open_char
                    );
                } else {
                    size_t open_index = stack.top();
                    stack.pop();
                    pairs.emplace_back(open_index, i);
                }
            }
        }
    }
    
    // Check for unmatched opening brackets
    while (!stack.empty()) {
        size_t open_index = stack.top();
        stack.pop();
        result.add_error(
            std::string("Unmatched opening bracket: ") + open_char,
            token_location(open_index),
            std::string("Add matching closing bracket: ") + close_char
        );
    }
    
    return result;
}

validation::SourceLocation BracketMatcher::token_location(size_t index) const {
    if (index >= tokens_.size()) {
        return validation::SourceLocation();
    }
    
    const auto& token = tokens_[index];
    return validation::SourceLocation(token.line, token.column, token.position, 
                                    token.position + token.value.length());
}

} // namespace cprime::layer1validation