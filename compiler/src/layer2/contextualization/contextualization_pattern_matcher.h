#pragma once

#include "pattern_core_structures.h"
#include "../layer2.h"
#include <map>
#include <memory>

namespace cprime::layer2_contextualization {

// Singleton class that manages pattern matching for all contextualization types
class ContextualizationPatternMatcher {
public:
    // Singleton access
    static ContextualizationPatternMatcher& getInstance();
    
    // Delete copy constructor and assignment operator
    ContextualizationPatternMatcher(const ContextualizationPatternMatcher&) = delete;
    ContextualizationPatternMatcher& operator=(const ContextualizationPatternMatcher&) = delete;
    
    // Main pattern matching interface
    PatternMatchResult match_header_pattern(const Instruction& header_instruction);
    PatternMatchResult match_footer_pattern(const Instruction& footer_instruction);
    PatternMatchResult match_body_pattern(const Instruction& body_instruction);
    
    // Pattern registration interface (for building the trees)
    void register_header_pattern(const Pattern& pattern);
    void register_footer_pattern(const Pattern& pattern);
    void register_body_pattern(const Pattern& pattern);
    
    // Debug and testing interface
    size_t get_header_pattern_count() const;
    size_t get_footer_pattern_count() const;  
    size_t get_body_pattern_count() const;
    void clear_all_patterns(); // For testing
    
private:
    // Private constructor for singleton
    ContextualizationPatternMatcher();
    ~ContextualizationPatternMatcher() = default;
    
    // Pattern trees - one for each instruction type
    std::unique_ptr<PatternNode> header_pattern_tree_;
    std::unique_ptr<PatternNode> footer_pattern_tree_;
    std::unique_ptr<PatternNode> body_pattern_tree_;
    
    // Pattern storage for tree building
    std::vector<std::unique_ptr<Pattern>> header_patterns_;
    std::vector<std::unique_ptr<Pattern>> footer_patterns_;
    std::vector<std::unique_ptr<Pattern>> body_patterns_;
    
    // Core pattern matching algorithm
    PatternMatchResult match_instruction_against_tree(
        const Instruction& instruction, 
        PatternNode* tree_root
    );
    
    // Token preprocessing - creates clean index vector skipping whitespace/comments
    std::vector<size_t> preprocess_instruction_tokens(const Instruction& instruction);
    
    // Tree building helpers
    void build_pattern_tree(std::unique_ptr<PatternNode>& tree_root, const std::vector<std::unique_ptr<Pattern>>& patterns);
    void insert_pattern_into_tree(PatternNode* root, const Pattern* pattern);
    
    // Pattern matching traversal
    PatternMatchResult traverse_pattern_tree(
        PatternNode* current_node,
        const Instruction& instruction,
        const std::vector<size_t>& clean_indices,
        size_t current_index,
        std::vector<ContextualTokenResult>& accumulated_results
    );
    
    // Pattern element matching helpers
    bool matches_pattern_element(
        const PatternElement& element,
        const Instruction& instruction,
        const std::vector<size_t>& clean_indices,
        size_t& current_index,
        ContextualTokenResult& result
    );
    
    // Helper for different pattern element types
    bool matches_concrete_token(const PatternElement& element, EToken token);
    bool matches_concrete_token_group(const PatternElement& element, EToken token);
    bool matches_whitespace_pattern(
        const PatternElement& element,
        const Instruction& instruction,
        const std::vector<size_t>& clean_indices,
        size_t& current_index,
        ContextualTokenResult& result
    );
    bool matches_namespaced_identifier(
        const PatternElement& element,
        const Instruction& instruction,
        const std::vector<size_t>& clean_indices,
        size_t& current_index,
        ContextualTokenResult& result
    );
    
    // Tree initialization - called once during first access
    void initialize_builtin_patterns();
    bool patterns_initialized_ = false;
};

} // namespace cprime::layer2_contextualization