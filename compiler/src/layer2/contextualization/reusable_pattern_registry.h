#pragma once

#include "pattern_core_structures.h"
#include "../../commons/logger.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

namespace cprime::layer2_contextualization {

// Registry for reusable patterns (optional and repeatable)
// These patterns don't have END_OF_PATTERN requirements and can be shared across all contexts
class ReusablePatternRegistry {
private:
    // Optional patterns: can occur 0 times
    std::unordered_map<PatternKey, std::unique_ptr<Pattern>> optional_patterns_;
    
    // Repeatable patterns: can occur 1+ times
    std::unordered_map<PatternKey, std::unique_ptr<Pattern>> repeatable_patterns_;
    
    // Pattern metadata for debugging and validation
    struct PatternMetadata {
        std::string description;
        std::vector<PatternKey> dependencies;  // Other patterns this one might reference
        bool is_terminal;                      // Can this pattern end without END_OF_PATTERN?
    };
    std::unordered_map<PatternKey, PatternMetadata> metadata_;

public:
    ReusablePatternRegistry() = default;
    ~ReusablePatternRegistry() = default;
    
    // Pattern registration interface
    void register_optional_pattern(PatternKey key, const Pattern& pattern, const std::string& description = "");
    void register_repeatable_pattern(PatternKey key, const Pattern& pattern, const std::string& description = "");
    
    // Pattern retrieval interface
    const Pattern* get_optional_pattern(PatternKey key) const;
    const Pattern* get_repeatable_pattern(PatternKey key) const;
    const Pattern* get_pattern(PatternKey key) const;  // Unified lookup
    
    // Pattern type checking
    bool is_optional_pattern(PatternKey key) const;
    bool is_repeatable_pattern(PatternKey key) const;
    bool is_reusable_pattern(PatternKey key) const;
    
    // Registry information
    std::vector<PatternKey> get_all_optional_keys() const;
    std::vector<PatternKey> get_all_repeatable_keys() const;
    std::vector<PatternKey> get_all_reusable_keys() const;
    
    // Validation and debugging
    bool validate_pattern_dependencies() const;
    void log_registry_state() const;
    std::string get_pattern_description(PatternKey key) const;
    
    // Built-in pattern initialization
    void initialize_builtin_reusable_patterns();
    
private:
    // Helper methods
    void validate_pattern_key_range(PatternKey key, bool is_optional) const;
    std::string pattern_key_to_string(PatternKey key) const;
};

} // namespace cprime::layer2_contextualization