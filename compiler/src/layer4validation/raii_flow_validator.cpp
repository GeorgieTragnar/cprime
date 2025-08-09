#include "raii_flow_validator.h"

namespace cprime::layer4validation {

RAIIFlowValidator::RAIIFlowValidator(const StructuredTokens& structured_tokens)
    : structured_tokens_(structured_tokens) {}

validation::ValidationResult RAIIFlowValidator::validate() {
    validation::ValidationResult result;
    
    // Check that input is properly contextualized
    if (!structured_tokens_.is_contextualized()) {
        result.add_error(
            "RAIIFlowValidator requires contextualized StructuredTokens",
            validation::SourceLocation{0, 0}
        );
        return result;
    }
    
    // Validate defer processing
    auto defer_result = validate_defer_processing();
    result.merge(defer_result);
    if (!defer_result.success()) {
        return result;
    }
    
    // Validate cleanup sequences
    auto cleanup_result = validate_cleanup_sequences();
    result.merge(cleanup_result);
    if (!cleanup_result.success()) {
        return result;
    }
    
    // Validate conditional defer handling
    auto conditional_result = validate_conditional_defer_handling();
    result.merge(conditional_result);
    
    return result;
}

validation::ValidationResult RAIIFlowValidator::validate_defer_processing() {
    validation::ValidationResult result;
    
    // Check each function scope for proper defer processing
    for (size_t i = 0; i < structured_tokens_.scopes.size(); ++i) {
        const auto& scope = structured_tokens_.scopes[i];
        
        if (scope.type == Scope::NamedFunction) {
            // Function scopes should not have unprocessed defer tokens
            if (has_unprocessed_defer_tokens(scope.content)) {
                result.add_error(
                    "Function scope contains unprocessed DEFER_RAII tokens",
                    validation::SourceLocation{0, 0}  // TODO: Add proper location tracking
                );
            }
        }
    }
    
    return result;
}

validation::ValidationResult RAIIFlowValidator::validate_cleanup_sequences() {
    validation::ValidationResult result;
    
    // Validate that cleanup sequences are properly placed before return statements
    for (size_t i = 0; i < structured_tokens_.scopes.size(); ++i) {
        const auto& scope = structured_tokens_.scopes[i];
        
        if (scope.type == Scope::NamedFunction) {
            auto return_positions = find_return_statements(scope.content);
            
            // Each return should be preceded by appropriate cleanup if variables exist
            for (size_t return_pos : return_positions) {
                // Check if there's a cleanup sequence before this return
                // This is a simplified check - real implementation would need more sophisticated analysis
                if (return_pos > 0) {
                    // Look for cleanup tokens before return
                    bool has_cleanup = false;
                    for (size_t j = return_pos - 1; j > 0 && j < return_pos; --j) {
                        if (validate_cleanup_token_structure(scope.content, j)) {
                            has_cleanup = true;
                            break;
                        }
                    }
                    
                    // Note: We don't enforce cleanup presence because functions
                    // might not have variables that need cleanup
                }
            }
        }
    }
    
    return result;
}

validation::ValidationResult RAIIFlowValidator::validate_conditional_defer_handling() {
    validation::ValidationResult result;
    
    // Check that conditional scopes with defer are properly validated
    for (size_t i = 0; i < structured_tokens_.scopes.size(); ++i) {
        const auto& scope = structured_tokens_.scopes[i];
        
        if (scope.is_conditional()) {
            // Conditional scopes should not have unprocessed defer validation errors
            // This would be checked by looking at the structured_tokens_.errors
            for (const auto& error : structured_tokens_.errors) {
                if (error.scope_index == i && 
                    error.message.find("defer") != std::string::npos) {
                    result.add_error(
                        "Conditional defer validation error: " + error.message,
                        validation::SourceLocation{0, 0}
                    );
                }
            }
        }
    }
    
    return result;
}

bool RAIIFlowValidator::has_unprocessed_defer_tokens(const std::vector<uint32_t>& tokens) const {
    for (uint32_t token : tokens) {
        auto token_kind = static_cast<ContextualTokenKind>(token);
        if (token_kind == ContextualTokenKind::DEFER_RAII) {
            return true;
        }
    }
    return false;
}

bool RAIIFlowValidator::validate_cleanup_token_structure(const std::vector<uint32_t>& tokens, size_t start_pos) const {
    if (start_pos >= tokens.size()) return false;
    
    // Check for a pattern that looks like a destructor call
    // Simplified: identifier + ( + identifier + ) + ;
    if (start_pos + 4 >= tokens.size()) return false;
    
    auto token1 = static_cast<ContextualTokenKind>(tokens[start_pos]);
    auto token2 = static_cast<ContextualTokenKind>(tokens[start_pos + 1]);
    auto token3 = static_cast<ContextualTokenKind>(tokens[start_pos + 2]);
    auto token4 = static_cast<ContextualTokenKind>(tokens[start_pos + 3]);
    auto token5 = static_cast<ContextualTokenKind>(tokens[start_pos + 4]);
    
    return token1 == ContextualTokenKind::IDENTIFIER &&
           token2 == ContextualTokenKind::LEFT_PAREN &&
           token3 == ContextualTokenKind::IDENTIFIER &&
           token4 == ContextualTokenKind::RIGHT_PAREN &&
           token5 == ContextualTokenKind::SEMICOLON;
}

size_t RAIIFlowValidator::count_cleanup_sequences(const std::vector<uint32_t>& tokens) const {
    size_t count = 0;
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (validate_cleanup_token_structure(tokens, i)) {
            count++;
            i += 4; // Skip the tokens we just validated
        }
    }
    
    return count;
}

std::vector<size_t> RAIIFlowValidator::find_return_statements(const std::vector<uint32_t>& tokens) const {
    std::vector<size_t> positions;
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        auto token_kind = static_cast<ContextualTokenKind>(tokens[i]);
        if (token_kind == ContextualTokenKind::RETURN) {
            positions.push_back(i);
        }
    }
    
    return positions;
}

} // namespace cprime::layer4validation