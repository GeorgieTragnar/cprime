#pragma once

#include "../common/structural_types.h"
#include "../common/validation_types.h"
#include <vector>

namespace cprime::layer3validation {

/**
 * Layer 3 Validation - Contextualization Validator
 * Validates that contextualization was performed correctly and completely.
 * 
 * Key validations:
 * - All TokenKind values properly transformed to ContextualTokenKind
 * - No CONTEXTUAL_TODO or CONTEXTUAL_ERROR values remain
 * - Contextual interpretations are consistent with scope structure
 * - Access right and type parameter contexts are properly resolved
 */
class ContextualizationValidator {
public:
    explicit ContextualizationValidator(const StructuredTokens& structured_tokens);
    
    // Main validation method
    validation::ValidationResult validate();
    
    // Specific validation checks
    validation::ValidationResult validate_contextualized_flag_consistency();
    validation::ValidationResult validate_no_unresolved_tokens();
    validation::ValidationResult validate_contextual_consistency();
    validation::ValidationResult validate_scope_type_alignment();
    
    std::string get_validator_name() const { return "ContextualizationValidator"; }
    
private:
    const StructuredTokens& structured_tokens_;
    
    // Helper methods
    bool has_unresolved_contextual_tokens(const std::vector<uint32_t>& token_sequence);
    bool is_contextual_interpretation_valid(ContextualTokenKind kind, Scope::Type scope_type, bool in_signature);
    std::string get_contextual_token_name(ContextualTokenKind kind);
};

} // namespace cprime::layer3validation