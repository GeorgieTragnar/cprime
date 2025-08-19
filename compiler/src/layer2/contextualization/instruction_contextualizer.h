#pragma once

#include "base_contextualizer.h"
#include "context_pattern_elements.h"
#include "../../commons/token.h"
#include "../../commons/contextualToken.h"
#include "../../commons/contextualizationError.h"
#include <vector>
#include <string>
#include <functional>

namespace cprime::layer2_contextualization {

// Type alias for instruction contextualization patterns - using the unified system
using InstructionContextualizationPattern = BaseContextualizationPattern<InstructionPatternElement>;

// Main class for instruction-level contextualization using pattern matching
class InstructionContextualizer : public BaseContextualizer<InstructionPatternElement> {
public:
    InstructionContextualizer();
    
    // Check if a token matches an instruction-specific pattern element
    bool token_matches_element(const Token& token, InstructionPatternElement element) override;
    
    // Setup instruction-specific patterns
    void setup_instruction_patterns();
    
    // Legacy interface wrapper for existing code
    std::vector<ContextualToken> contextualize_instruction(const std::vector<Token>& tokens) {
        return contextualize(tokens);
    }
    
protected:
    // Check if pattern element is a whitespace pattern (includes base + instruction-specific)
    bool is_whitespace_pattern_element(InstructionPatternElement element) override;
    
    // Helper to match base pattern elements
    bool token_matches_base_element(const Token& token, BasePatternElement element);
    
private:
    // Helper methods for pattern setup
    void setup_basic_patterns();
    void setup_declaration_patterns();
    void setup_assignment_patterns();
    void setup_function_call_patterns();
    void setup_operator_patterns();
    void setup_whitespace_patterns();
    void setup_advanced_patterns();
};

// Legacy pattern setup functions - now integrated into class methods

} // namespace cprime::layer2_contextualization