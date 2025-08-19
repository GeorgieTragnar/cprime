#include "footer_contextualizer.h"
#include "../../commons/logger.h"
#include "../../commons/enum/token.h"

namespace cprime::layer2_contextualization {

FooterContextualizer::FooterContextualizer() {
    setup_footer_patterns();
}

bool FooterContextualizer::token_matches_element(const Token& token, FooterPatternElement element) {
    // First check if it's a base pattern element
    if (is_base_pattern_element(element)) {
        BasePatternElement base_element = to_base_pattern_element(element);
        return token_matches_base_element(token, base_element);
    }
    
    // Handle footer-specific pattern elements
    switch (element) {
        // Return patterns
        case FooterPatternElement::KEYWORD_RETURN:
            return token._token == EToken::RETURN;
        case FooterPatternElement::RETURN_EXPRESSION:
            return false; // Handled by specialized N:M matching logic
        case FooterPatternElement::RETURN_VOID:
            return token._token == EToken::RETURN; // return without expression
            
        // Control flow patterns
        case FooterPatternElement::KEYWORD_BREAK:
            return token._token == EToken::BREAK;
        case FooterPatternElement::KEYWORD_CONTINUE:
            return token._token == EToken::CONTINUE;
        case FooterPatternElement::KEYWORD_GOTO:
            return false; // GOTO not in current EToken enum
        case FooterPatternElement::LABEL_REFERENCE:
            return token._token == EToken::IDENTIFIER; // Label names are identifiers
            
        // Exception patterns
        case FooterPatternElement::KEYWORD_THROW:
            return token._token == EToken::RAISE; // CPrime uses RAISE instead of throw
        case FooterPatternElement::KEYWORD_RETHROW:
            return false; // RETHROW not in current EToken enum
        case FooterPatternElement::EXCEPTION_EXPRESSION:
            return false; // Handled by specialized N:M matching logic
            
        // Cleanup patterns
        case FooterPatternElement::KEYWORD_DEFER:
            return token._token == EToken::DEFER;
        case FooterPatternElement::CLEANUP_STATEMENT:
            return false; // Handled by specialized N:M matching logic
        case FooterPatternElement::RESOURCE_RELEASE:
            return false; // Handled by specialized N:M matching logic
        case FooterPatternElement::DESTRUCTOR_CALL:
            return false; // Handled by specialized N:M matching logic
            
        // Scope finalization
        case FooterPatternElement::SCOPE_CLEANUP:
            return false; // Handled by specialized N:M matching logic
        case FooterPatternElement::SCOPE_VALIDATION:
            return false; // Handled by specialized N:M matching logic
        case FooterPatternElement::SCOPE_SUMMARY:
            return false; // Handled by specialized N:M matching logic
            
        default:
            return false;
    }
}

bool FooterContextualizer::is_whitespace_pattern_element(FooterPatternElement element) {
    // Check base whitespace patterns first
    if (is_base_pattern_element(element)) {
        uint32_t element_value = static_cast<uint32_t>(element);
        
        return element_value == static_cast<uint32_t>(BasePatternElement::OPTIONAL_WHITESPACE) ||
               element_value == static_cast<uint32_t>(BasePatternElement::REQUIRED_WHITESPACE) ||
               element_value == static_cast<uint32_t>(BasePatternElement::SINGLE_WHITESPACE) ||
               element_value == static_cast<uint32_t>(BasePatternElement::MERGED_WHITESPACE);
    }
    
    // Footer-specific whitespace patterns (none for now)
    return false;
}

bool FooterContextualizer::token_matches_base_element(const Token& token, BasePatternElement element) {
    switch (element) {
        // Generic token types
        case BasePatternElement::ANY_IDENTIFIER:
            return token._token == EToken::IDENTIFIER;
        case BasePatternElement::ANY_LITERAL:
            return token._token == EToken::STRING_LITERAL || 
                   token._token == EToken::INT_LITERAL ||
                   token._token == EToken::FLOAT_LITERAL;
        case BasePatternElement::ANY_STRING_LITERAL:
            return token._token == EToken::STRING_LITERAL;
        case BasePatternElement::ANY_INT_LITERAL:
            return token._token == EToken::INT_LITERAL;
            
        // Specific operators and punctuation
        case BasePatternElement::LITERAL_ASSIGN:
            return token._token == EToken::ASSIGN;
        case BasePatternElement::LITERAL_PLUS:
            return token._token == EToken::PLUS;
        case BasePatternElement::LITERAL_MINUS:
            return token._token == EToken::MINUS;
        case BasePatternElement::LITERAL_MULTIPLY:
            return token._token == EToken::MULTIPLY;
        case BasePatternElement::LITERAL_DIVIDE:
            return token._token == EToken::DIVIDE;
        case BasePatternElement::LITERAL_SEMICOLON:
            return token._token == EToken::SEMICOLON;
        case BasePatternElement::LITERAL_COLON:
            return token._token == EToken::COLON;
        case BasePatternElement::LITERAL_COMMA:
            return token._token == EToken::COMMA;
        case BasePatternElement::LITERAL_DOT:
            return token._token == EToken::DOT;
            
        // Brackets and delimiters
        case BasePatternElement::LITERAL_PAREN_L:
            return token._token == EToken::LEFT_PAREN;
        case BasePatternElement::LITERAL_PAREN_R:
            return token._token == EToken::RIGHT_PAREN;
        case BasePatternElement::LITERAL_BRACE_L:
            return token._token == EToken::LEFT_BRACE;
        case BasePatternElement::LITERAL_BRACE_R:
            return token._token == EToken::RIGHT_BRACE;
        case BasePatternElement::LITERAL_BRACKET_L:
            return token._token == EToken::LEFT_BRACKET;
        case BasePatternElement::LITERAL_BRACKET_R:
            return token._token == EToken::RIGHT_BRACKET;
        case BasePatternElement::LITERAL_LESS:
            return token._token == EToken::LESS_THAN;
        case BasePatternElement::LITERAL_GREATER:
            return token._token == EToken::GREATER_THAN;
            
        // Compound operators
        case BasePatternElement::LITERAL_DOUBLE_COLON:
            return token._token == EToken::SCOPE_RESOLUTION;
        case BasePatternElement::LITERAL_ARROW:
            return token._token == EToken::ARROW;
        case BasePatternElement::LITERAL_PLUS_ASSIGN:
        case BasePatternElement::LITERAL_MINUS_ASSIGN:
            return false; // Not implemented yet
            
        // Complex patterns
        case BasePatternElement::EXPRESSION_TOKENS:
        case BasePatternElement::TYPE_TOKEN_LIST:
        case BasePatternElement::PARAMETER_LIST:
        case BasePatternElement::ARGUMENT_LIST:
            return false; // Not implemented yet
            
        default:
            return false;
    }
}

void FooterContextualizer::setup_footer_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("footer_contextualizer");
    LOG_INFO("Setting up footer contextualization patterns");
    
    setup_return_patterns();
    setup_control_flow_patterns();
    setup_exception_patterns();
    setup_cleanup_patterns();
    setup_scope_finalization_patterns();
    
    LOG_INFO("Footer pattern setup complete - {} patterns registered", pattern_count());
}

void FooterContextualizer::setup_return_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("footer_contextualizer");
    LOG_DEBUG("Setting up return patterns");
    
    // Pattern: return;
    // Example: return;
    FooterContextualizationPattern void_return(
        "void_return",
        {FooterPatternElement::KEYWORD_RETURN,
         static_cast<FooterPatternElement>(BasePatternElement::OPTIONAL_WHITESPACE),
         static_cast<FooterPatternElement>(BasePatternElement::LITERAL_SEMICOLON)},
        {
            ContextualTokenTemplate(EContextualToken::CONTROL_FLOW, {0}, "void return statement"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {1}, "return spacing"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {2}, "statement terminator")
        },
        100
    );
    register_pattern(void_return);
    
    // Pattern: return expression;
    // Example: return 42; return value + 1;
    FooterContextualizationPattern expression_return(
        "expression_return",
        {FooterPatternElement::KEYWORD_RETURN,
         static_cast<FooterPatternElement>(BasePatternElement::REQUIRED_WHITESPACE),
         FooterPatternElement::RETURN_EXPRESSION,
         static_cast<FooterPatternElement>(BasePatternElement::OPTIONAL_WHITESPACE),
         static_cast<FooterPatternElement>(BasePatternElement::LITERAL_SEMICOLON)},
        {
            ContextualTokenTemplate(EContextualToken::CONTROL_FLOW, {0, 2}, "return with expression"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {1}, "return spacing"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {3}, "pre-semicolon spacing"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {4}, "statement terminator")
        },
        120
    );
    register_pattern(expression_return);
    
    LOG_DEBUG("Return patterns registered");
}

void FooterContextualizer::setup_control_flow_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("footer_contextualizer");
    LOG_DEBUG("Setting up control flow patterns");
    
    // Pattern: break;
    FooterContextualizationPattern break_statement(
        "break_statement",
        {FooterPatternElement::KEYWORD_BREAK,
         static_cast<FooterPatternElement>(BasePatternElement::OPTIONAL_WHITESPACE),
         static_cast<FooterPatternElement>(BasePatternElement::LITERAL_SEMICOLON)},
        {
            ContextualTokenTemplate(EContextualToken::CONTROL_FLOW, {0}, "break statement"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {1}, "break spacing"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {2}, "statement terminator")
        },
        100
    );
    register_pattern(break_statement);
    
    // Pattern: continue;
    FooterContextualizationPattern continue_statement(
        "continue_statement",
        {FooterPatternElement::KEYWORD_CONTINUE,
         static_cast<FooterPatternElement>(BasePatternElement::OPTIONAL_WHITESPACE),
         static_cast<FooterPatternElement>(BasePatternElement::LITERAL_SEMICOLON)},
        {
            ContextualTokenTemplate(EContextualToken::CONTROL_FLOW, {0}, "continue statement"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {1}, "continue spacing"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {2}, "statement terminator")
        },
        100
    );
    register_pattern(continue_statement);
    
    LOG_DEBUG("Control flow patterns registered");
}

void FooterContextualizer::setup_exception_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("footer_contextualizer");
    LOG_DEBUG("Setting up exception patterns");
    
    // Pattern: raise expression;
    // Example: raise error("Something went wrong");
    FooterContextualizationPattern raise_statement(
        "raise_statement",
        {FooterPatternElement::KEYWORD_THROW,
         static_cast<FooterPatternElement>(BasePatternElement::REQUIRED_WHITESPACE),
         FooterPatternElement::EXCEPTION_EXPRESSION,
         static_cast<FooterPatternElement>(BasePatternElement::OPTIONAL_WHITESPACE),
         static_cast<FooterPatternElement>(BasePatternElement::LITERAL_SEMICOLON)},
        {
            ContextualTokenTemplate(EContextualToken::CONTROL_FLOW, {0, 2}, "raise/throw statement"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {1}, "raise spacing"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {3}, "pre-semicolon spacing"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {4}, "statement terminator")
        },
        110
    );
    register_pattern(raise_statement);
    
    LOG_DEBUG("Exception patterns registered");
}

void FooterContextualizer::setup_cleanup_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("footer_contextualizer");
    LOG_DEBUG("Setting up cleanup patterns");
    
    // Pattern: defer statement;
    // Example: defer close_file();
    FooterContextualizationPattern defer_statement(
        "defer_statement",
        {FooterPatternElement::KEYWORD_DEFER,
         static_cast<FooterPatternElement>(BasePatternElement::REQUIRED_WHITESPACE),
         FooterPatternElement::CLEANUP_STATEMENT,
         static_cast<FooterPatternElement>(BasePatternElement::OPTIONAL_WHITESPACE),
         static_cast<FooterPatternElement>(BasePatternElement::LITERAL_SEMICOLON)},
        {
            ContextualTokenTemplate(EContextualToken::RESOURCE_MANAGEMENT, {0, 2}, "defer cleanup statement"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {1}, "defer spacing"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {3}, "pre-semicolon spacing"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {4}, "statement terminator")
        },
        110
    );
    register_pattern(defer_statement);
    
    LOG_DEBUG("Cleanup patterns registered");
}

void FooterContextualizer::setup_scope_finalization_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("footer_contextualizer");
    LOG_DEBUG("Setting up scope finalization patterns");
    
    // TODO: Implement scope finalization patterns when needed
    
    LOG_DEBUG("Scope finalization patterns setup (placeholder)");
}

PatternMatchResult FooterContextualizer::try_match_pattern(const std::vector<Token>& tokens, 
                                                          size_t start_pos, 
                                                          const BaseContextualizationPattern<FooterPatternElement>& pattern) {
    // Check if pattern contains complex N:M elements that need specialized handling
    for (const auto& element : pattern.token_pattern) {
        switch (element) {
            case FooterPatternElement::RETURN_EXPRESSION:
                return try_match_return_expression(tokens, start_pos);
            case FooterPatternElement::EXCEPTION_EXPRESSION:
                return try_match_exception_expression(tokens, start_pos);
            case FooterPatternElement::CLEANUP_STATEMENT:
                return try_match_cleanup_statement(tokens, start_pos);
            case FooterPatternElement::SCOPE_CLEANUP:
                return try_match_scope_cleanup(tokens, start_pos);
            default:
                // Continue to base class implementation for simple patterns
                break;
        }
    }
    
    // Use base class implementation for simple patterns
    return BaseContextualizer<FooterPatternElement>::try_match_pattern(tokens, start_pos, pattern);
}

PatternMatchResult FooterContextualizer::try_match_return_expression(const std::vector<Token>& tokens, size_t start_pos) {
    auto logger = cprime::LoggerFactory::get_logger("footer_contextualizer");
    
    if (start_pos >= tokens.size()) {
        return PatternMatchResult::failure("No tokens available for return expression");
    }
    
    size_t pos = start_pos;
    std::vector<uint32_t> expression_token_indices;
    
    // Consume tokens until we hit semicolon, newline, or end of tokens
    while (pos < tokens.size()) {
        if (tokens[pos]._token == EToken::SEMICOLON || 
            tokens[pos]._token == EToken::NEWLINE ||
            tokens[pos]._token == EToken::EOF_TOKEN) {
            break; // End of return expression
        }
        
        expression_token_indices.push_back(tokens[pos]._tokenIndex);
        pos++;
    }
    
    if (expression_token_indices.empty()) {
        return PatternMatchResult::failure("Empty return expression");
    }
    
    // Generate contextual tokens for the return expression
    std::vector<ContextualToken> contextual_tokens;
    
    ContextualToken expression_token;
    expression_token._contextualToken = EContextualToken::EXPRESSION;
    expression_token._parentTokenIndices = expression_token_indices;
    contextual_tokens.push_back(expression_token);
    
    LOG_DEBUG("Matched return expression consuming {} tokens", pos - start_pos);
    return PatternMatchResult::success(pos - start_pos, contextual_tokens);
}

PatternMatchResult FooterContextualizer::try_match_exception_expression(const std::vector<Token>& tokens, size_t start_pos) {
    auto logger = cprime::LoggerFactory::get_logger("footer_contextualizer");
    
    if (start_pos >= tokens.size()) {
        return PatternMatchResult::failure("No tokens available for exception expression");
    }
    
    size_t pos = start_pos;
    std::vector<uint32_t> expression_token_indices;
    
    // Consume tokens until we hit semicolon, newline, or end of tokens
    while (pos < tokens.size()) {
        if (tokens[pos]._token == EToken::SEMICOLON || 
            tokens[pos]._token == EToken::NEWLINE ||
            tokens[pos]._token == EToken::EOF_TOKEN) {
            break; // End of exception expression
        }
        
        expression_token_indices.push_back(tokens[pos]._tokenIndex);
        pos++;
    }
    
    if (expression_token_indices.empty()) {
        return PatternMatchResult::failure("Empty exception expression");
    }
    
    // Generate contextual tokens for the exception expression
    std::vector<ContextualToken> contextual_tokens;
    
    ContextualToken expression_token;
    expression_token._contextualToken = EContextualToken::EXPRESSION;
    expression_token._parentTokenIndices = expression_token_indices;
    contextual_tokens.push_back(expression_token);
    
    LOG_DEBUG("Matched exception expression consuming {} tokens", pos - start_pos);
    return PatternMatchResult::success(pos - start_pos, contextual_tokens);
}

PatternMatchResult FooterContextualizer::try_match_cleanup_statement(const std::vector<Token>& tokens, size_t start_pos) {
    auto logger = cprime::LoggerFactory::get_logger("footer_contextualizer");
    
    if (start_pos >= tokens.size()) {
        return PatternMatchResult::failure("No tokens available for cleanup statement");
    }
    
    size_t pos = start_pos;
    std::vector<uint32_t> statement_token_indices;
    
    // Consume tokens until we hit semicolon, newline, or end of tokens
    while (pos < tokens.size()) {
        if (tokens[pos]._token == EToken::SEMICOLON || 
            tokens[pos]._token == EToken::NEWLINE ||
            tokens[pos]._token == EToken::EOF_TOKEN) {
            break; // End of cleanup statement
        }
        
        statement_token_indices.push_back(tokens[pos]._tokenIndex);
        pos++;
    }
    
    if (statement_token_indices.empty()) {
        return PatternMatchResult::failure("Empty cleanup statement");
    }
    
    // Generate contextual tokens for the cleanup statement
    std::vector<ContextualToken> contextual_tokens;
    
    ContextualToken statement_token;
    statement_token._contextualToken = EContextualToken::RESOURCE_MANAGEMENT;
    statement_token._parentTokenIndices = statement_token_indices;
    contextual_tokens.push_back(statement_token);
    
    LOG_DEBUG("Matched cleanup statement consuming {} tokens", pos - start_pos);
    return PatternMatchResult::success(pos - start_pos, contextual_tokens);
}

PatternMatchResult FooterContextualizer::try_match_scope_cleanup(const std::vector<Token>& tokens, size_t start_pos) {
    auto logger = cprime::LoggerFactory::get_logger("footer_contextualizer");
    
    // For now, scope cleanup is not fully implemented
    return PatternMatchResult::failure("Scope cleanup pattern not yet implemented");
}

bool FooterContextualizer::is_footer_exec_execution_pattern(const std::vector<Token>& tokens) {
    if (tokens.empty()) return false;
    
    // Pattern 1: Noname exec footer execution - "<args>"
    // Look for: LESS_THAN + ... + GREATER_THAN (parameters for noname exec)
    // This is the primary footer execution pattern
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i]._token == EToken::LESS_THAN) {
            // Look for matching GREATER_THAN
            for (size_t j = i + 1; j < tokens.size(); ++j) {
                if (tokens[j]._token == EToken::GREATER_THAN) {
                    // Found <...> pattern - this is a footer exec execution
                    return true;
                }
            }
        }
    }
    
    // Pattern 2: Named exec alias call in footer - "EXEC_ALIAS<params>()"
    // Footer can also contain named exec alias calls
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i]._token == EToken::EXEC_ALIAS) {
            // Found exec alias in footer
            return true;
        }
    }
    
    // Pattern 3: Direct identifier exec call in footer - "identifier<params>()"
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i]._token == EToken::IDENTIFIER && i + 1 < tokens.size() &&
            tokens[i + 1]._token == EToken::LESS_THAN) {
            // Found potential exec call in footer
            return true;
        }
    }
    
    return false;
}

} // namespace cprime::layer2_contextualization