#pragma once

#include "base_contextualizer.h"
#include "context_pattern_elements.h"

namespace cprime::layer2_contextualization {

// Footer contextualizer for control flow exits, cleanup, and scope finalization
class FooterContextualizer : public BaseContextualizer<FooterPatternElement> {
public:
    FooterContextualizer();
    
    // Check if a token matches a footer-specific pattern element
    bool token_matches_element(const Token& token, FooterPatternElement element) override;
    
    // Setup footer-specific patterns
    void setup_footer_patterns();
    
    // Override pattern matching to support N:M complex patterns
    PatternMatchResult try_match_pattern(const std::vector<Token>& tokens, 
                                        size_t start_pos, 
                                        const BaseContextualizationPattern<FooterPatternElement>& pattern) override;
    
protected:
    // Check if pattern element is a whitespace pattern (includes base + footer-specific)
    bool is_whitespace_pattern_element(FooterPatternElement element) override;
    
    // Helper to match base pattern elements
    bool token_matches_base_element(const Token& token, BasePatternElement element);
    
private:
    // Helper methods for pattern setup
    void setup_return_patterns();
    void setup_control_flow_patterns();
    void setup_exception_patterns();
    void setup_cleanup_patterns();
    void setup_scope_finalization_patterns();
    
    // N:M pattern matching helpers for complex pattern elements
    PatternMatchResult try_match_return_expression(const std::vector<Token>& tokens, size_t start_pos);
    PatternMatchResult try_match_exception_expression(const std::vector<Token>& tokens, size_t start_pos);
    PatternMatchResult try_match_cleanup_statement(const std::vector<Token>& tokens, size_t start_pos);
    PatternMatchResult try_match_scope_cleanup(const std::vector<Token>& tokens, size_t start_pos);
    
    // Preserve existing exec detection functionality
    bool is_footer_exec_execution_pattern(const std::vector<Token>& tokens);
};

} // namespace cprime::layer2_contextualization