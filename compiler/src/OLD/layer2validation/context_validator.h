#pragma once

#include "../validation_common.h"
#include "../common/structural_types.h"
#include <vector>
#include <unordered_set>

namespace cprime::layer2validation {

/**
 * Structure completeness validator for Layer 2.
 * Validates that structured tokens are correctly organized with proper scope hierarchy.
 * 
 * Responsibilities:
 * - Ensure proper scope hierarchy and bracket matching
 * - Validate scope types are correctly determined
 * - Check that cache-and-boundary algorithm worked correctly
 * - Verify signature tokens vs content tokens are properly separated
 * - Validate structure integrity before contextualization
 */
class StructureValidator : public validation::BaseValidator {
public:
    explicit StructureValidator(const StructuredTokens& structured_tokens);
    
    // BaseValidator interface
    validation::ValidationResult validate() override;
    std::string get_validator_name() const override { return "StructureValidator"; }
    
    // Specific validation methods
    validation::ValidationResult validate_scope_hierarchy_integrity();
    validation::ValidationResult validate_scope_type_consistency();
    validation::ValidationResult validate_signature_content_separation();
    validation::ValidationResult validate_bracket_balance();
    validation::ValidationResult validate_structure_completeness();
    
private:
    const StructuredTokens& structured_tokens_;
};

} // namespace cprime::layer2validation