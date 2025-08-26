#pragma once

#include "pattern_core_structures.h"
#include "reusable_pattern_registry.h"
#include "pattern_definitions_optional.h"
#include "pattern_definitions_header.h"
#include "pattern_definitions_footer.h"
#include "pattern_definitions_body.h"
#include "../layer2.h"
#include <map>
#include <memory>
#include <unordered_set>

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
    
    // Reusable pattern registry access
    ReusablePatternRegistry& get_reusable_registry();
    const ReusablePatternRegistry& get_reusable_registry() const;
    
    // Pattern count queries (for logging/debugging)
    size_t get_header_pattern_count() const { return header_patterns_.size(); }
    size_t get_footer_pattern_count() const { return footer_patterns_.size(); }
    size_t get_body_pattern_count() const { return body_patterns_.size(); }
    
    // Convenience methods for reusable pattern registration
    void register_optional_pattern(PatternKey key, const Pattern& pattern, const std::string& description = "");
    void register_repeatable_pattern(PatternKey key, const Pattern& pattern, const std::string& description = "");
    
    // Debug and testing interface
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
    
    // Reusable pattern registry for optional/repeatable patterns
    ReusablePatternRegistry reusable_registry_;
    
    // Enhanced pattern trees for nested map-based matching (future migration)
    std::unique_ptr<KeyedPatternNode> keyed_header_tree_;
    std::unique_ptr<KeyedPatternNode> keyed_footer_tree_;
    std::unique_ptr<KeyedPatternNode> keyed_body_tree_;
    
    // Core pattern matching algorithm
    PatternMatchResult match_instruction_against_tree(
        const Instruction& instruction, 
        PatternNode* tree_root
    );
    
    // Enhanced pattern matching with keyed trees
    PatternMatchResult match_instruction_against_keyed_tree(
        const Instruction& instruction, 
        KeyedPatternNode* tree_root
    );
    
    // Token preprocessing - creates clean index vector skipping whitespace/comments
    std::vector<size_t> preprocess_instruction_tokens(const Instruction& instruction);
    
    // Tree building helpers
    void build_pattern_tree(std::unique_ptr<PatternNode>& tree_root, const std::vector<std::unique_ptr<Pattern>>& patterns);
    void insert_pattern_into_tree(PatternNode* root, const Pattern* pattern);
    
    // Enhanced tree building with reusable pattern support
    void build_keyed_pattern_tree(std::unique_ptr<KeyedPatternNode>& tree_root, const std::vector<std::unique_ptr<Pattern>>& patterns, PatternKey base_key);
    void insert_keyed_pattern_into_tree(KeyedPatternNode* root, const Pattern* pattern, PatternKey pattern_key);
    KeyedPatternNode* inline_reusable_pattern(PatternKey reusable_key, KeyedPatternNode* continuation_node);
    void build_nested_map_transitions(KeyedPatternNode* from_node, KeyedPatternNode* to_node, PatternKey pattern_key, const PatternElement& element);
    
    // Pattern matching traversal
    PatternMatchResult traverse_pattern_tree(
        PatternNode* current_node,
        const Instruction& instruction,
        const std::vector<size_t>& clean_indices,
        size_t current_index,
        std::vector<ContextualTokenResult>& accumulated_results
    );
    
    // Enhanced keyed pattern tree traversal with reusable pattern support
    PatternMatchResult traverse_keyed_pattern_tree(
        KeyedPatternNode* current_node,
        const Instruction& instruction,
        const std::vector<size_t>& clean_indices,
        size_t current_index,
        std::vector<ContextualTokenResult>& accumulated_results,
        PatternKey active_pattern_key,
        std::unordered_set<PatternKey>& used_optional_patterns
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
    
    // Reusable pattern helpers
    bool is_reusable_optional_element(const PatternElement& element) const;
    PatternKey get_reusable_pattern_key(const PatternElement& element) const;
    
    // Pattern uniqueness validation
    bool validate_pattern_uniqueness() const;
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
    void register_complex_test_pattern(); // For testing multiple reusable patterns
    bool patterns_initialized_ = false;
};

} // namespace cprime::layer2_contextualization