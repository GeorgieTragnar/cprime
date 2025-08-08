#pragma once

#include "../validation_common.h"
#include "../layer2/semantic_token.h"
#include <vector>
#include <unordered_set>

namespace cprime::layer2validation {

/**
 * Context completeness validator for Layer 2.
 * Validates that contextually-resolved semantic tokens are complete and consistent.
 * 
 * Responsibilities:
 * - Ensure all context-sensitive keywords are fully resolved
 * - Validate access rights declarations are complete
 * - Check that runtime/comptime context is consistent
 * - Verify defer statements have proper context
 * - Validate union declarations are complete
 */
class ContextValidator : public validation::BaseValidator {
public:
    explicit ContextValidator(const std::vector<SemanticToken>& tokens);
    
    // BaseValidator interface
    validation::ValidationResult validate() override;
    std::string get_validator_name() const override { return "ContextValidator"; }
    
    // Specific validation methods
    validation::ValidationResult validate_access_rights_completeness();
    validation::ValidationResult validate_runtime_comptime_consistency();
    validation::ValidationResult validate_defer_statement_context();
    validation::ValidationResult validate_union_declaration_completeness();
    validation::ValidationResult validate_keyword_context_resolution();
    
private:
    const std::vector<SemanticToken>& tokens_;
};

} // namespace cprime::layer2validation