#pragma once

#include "../validation_common.h"
#include "../common/structural_types.h"
#include <memory>

namespace cprime::layer4validation {

/**
 * RAIIFlowValidator - Validates the correctness of RAII flow analysis from Layer 4.
 * 
 * Ensures that:
 * 1. All defer statements have been properly processed
 * 2. Cleanup sequences are correctly placed before return statements
 * 3. Destruction order follows LIFO semantics with proper defer reordering
 * 4. No unresolved conditional defer patterns remain
 */
class RAIIFlowValidator : public validation::BaseValidator {
public:
    explicit RAIIFlowValidator(const StructuredTokens& structured_tokens);
    
    // BaseValidator interface
    validation::ValidationResult validate() override;
    std::string get_validator_name() const override { return "RAIIFlowValidator"; }
    
private:
    const StructuredTokens& structured_tokens_;
    
    /**
     * Validate that all defer statements have been processed.
     */
    validation::ValidationResult validate_defer_processing();
    
    /**
     * Validate cleanup sequence placement and ordering.
     */
    validation::ValidationResult validate_cleanup_sequences();
    
    /**
     * Validate that conditional defer patterns are correctly handled.
     */
    validation::ValidationResult validate_conditional_defer_handling();
    
    /**
     * Check for remaining unprocessed DEFER_RAII tokens.
     */
    bool has_unprocessed_defer_tokens(const std::vector<uint32_t>& tokens) const;
    
    /**
     * Validate cleanup token sequence structure.
     */
    bool validate_cleanup_token_structure(const std::vector<uint32_t>& tokens, size_t start_pos) const;
    
    /**
     * Count cleanup sequences in scope content.
     */
    size_t count_cleanup_sequences(const std::vector<uint32_t>& tokens) const;
    
    /**
     * Find return statements in scope content.
     */
    std::vector<size_t> find_return_statements(const std::vector<uint32_t>& tokens) const;
};

} // namespace cprime::layer4validation