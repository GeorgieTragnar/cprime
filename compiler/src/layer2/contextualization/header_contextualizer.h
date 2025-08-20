#pragma once

#include "base_contextualizer.h"
#include "context_pattern_elements.h"

namespace cprime::layer2_contextualization {

// Header contextualizer for function and type definitions
class HeaderContextualizer : public BaseContextualizer<HeaderPatternElement> {
public:
    HeaderContextualizer();
    
    // Check if a token matches a header-specific pattern element
    bool token_matches_element(const Token& token, HeaderPatternElement element) override;
    
    // Setup header-specific patterns
    void setup_header_patterns();
    
    // Override pattern matching to support N:M complex patterns
    PatternMatchResult try_match_pattern(const std::vector<Token>& tokens, 
                                        size_t start_pos, 
                                        const BaseContextualizationPattern<HeaderPatternElement>& pattern) override;
    
    // Override clean pattern matching to support N:M complex patterns with preprocessing
    PatternMatchResult try_match_pattern_clean(const std::vector<Token>& tokens,
                                              const std::vector<size_t>& clean_indices,
                                              size_t clean_start_pos,
                                              const BaseContextualizationPattern<HeaderPatternElement>& pattern) override;
    
protected:
    // Check if pattern element is a whitespace pattern (includes base + header-specific)
    bool is_whitespace_pattern_element(HeaderPatternElement element) override;
    
    // Helper to match base pattern elements
    bool token_matches_base_element(const Token& token, BasePatternElement element);
    
private:
    // Helper methods for pattern setup
    void setup_function_declaration_patterns();
    void setup_type_declaration_patterns();
    void setup_template_patterns();
    void setup_namespace_patterns();
    void setup_visibility_patterns();
    void setup_inheritance_patterns();
    void setup_import_export_patterns();
    
    // N:M pattern matching helpers for complex pattern elements
    PatternMatchResult try_match_function_parameters(const std::vector<Token>& tokens, size_t start_pos);
    PatternMatchResult try_match_type_body(const std::vector<Token>& tokens, size_t start_pos);
    PatternMatchResult try_match_template_parameters(const std::vector<Token>& tokens, size_t start_pos);
    PatternMatchResult try_match_namespace_path(const std::vector<Token>& tokens, size_t start_pos);
    PatternMatchResult try_match_inheritance_list(const std::vector<Token>& tokens, size_t start_pos);
    PatternMatchResult try_match_function_signature(const std::vector<Token>& tokens, size_t start_pos);
};

} // namespace cprime::layer2_contextualization