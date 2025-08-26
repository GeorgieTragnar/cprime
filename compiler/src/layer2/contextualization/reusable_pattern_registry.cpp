#include "reusable_pattern_registry.h"

namespace cprime::layer2_contextualization {

// Pattern registration interface
void ReusablePatternRegistry::register_optional_pattern(PatternKey key, const Pattern& pattern, const std::string& description) {
    auto logger = cprime::LoggerFactory::get_logger("reusable_pattern_registry");
    
    validate_pattern_key_range(key, true);
    
    // Create a copy of the pattern
    optional_patterns_[key] = std::make_unique<Pattern>(pattern);
    
    // Store metadata
    PatternMetadata meta;
    meta.description = description.empty() ? pattern_key_to_string(key) : description;
    meta.dependencies = {};  // TODO: analyze pattern for dependencies
    meta.is_terminal = true;  // Optional patterns are always terminal (can end at 0 occurrences)
    metadata_[key] = meta;
    
    LOG_DEBUG("Registered optional pattern: {} ({})", pattern_key_to_string(key), metadata_[key].description);
}

void ReusablePatternRegistry::register_repeatable_pattern(PatternKey key, const Pattern& pattern, const std::string& description) {
    auto logger = cprime::LoggerFactory::get_logger("reusable_pattern_registry");
    
    validate_pattern_key_range(key, false);
    
    // Create a copy of the pattern
    repeatable_patterns_[key] = std::make_unique<Pattern>(pattern);
    
    // Store metadata
    PatternMetadata meta;
    meta.description = description.empty() ? pattern_key_to_string(key) : description;
    meta.dependencies = {};  // TODO: analyze pattern for dependencies
    meta.is_terminal = false;  // Repeatable patterns require at least 1 occurrence
    metadata_[key] = meta;
    
    LOG_DEBUG("Registered repeatable pattern: {} ({})", pattern_key_to_string(key), metadata_[key].description);
}

// Pattern retrieval interface
const Pattern* ReusablePatternRegistry::get_optional_pattern(PatternKey key) const {
    auto it = optional_patterns_.find(key);
    return (it != optional_patterns_.end()) ? it->second.get() : nullptr;
}

const Pattern* ReusablePatternRegistry::get_repeatable_pattern(PatternKey key) const {
    auto it = repeatable_patterns_.find(key);
    return (it != repeatable_patterns_.end()) ? it->second.get() : nullptr;
}

const Pattern* ReusablePatternRegistry::get_pattern(PatternKey key) const {
    // Try optional patterns first
    const Pattern* pattern = get_optional_pattern(key);
    if (pattern) return pattern;
    
    // Try repeatable patterns
    return get_repeatable_pattern(key);
}

// Pattern type checking
bool ReusablePatternRegistry::is_optional_pattern(PatternKey key) const {
    return optional_patterns_.find(key) != optional_patterns_.end();
}

bool ReusablePatternRegistry::is_repeatable_pattern(PatternKey key) const {
    return repeatable_patterns_.find(key) != repeatable_patterns_.end();
}

bool ReusablePatternRegistry::is_reusable_pattern(PatternKey key) const {
    return is_optional_pattern(key) || is_repeatable_pattern(key);
}

// Registry information
std::vector<PatternKey> ReusablePatternRegistry::get_all_optional_keys() const {
    std::vector<PatternKey> keys;
    keys.reserve(optional_patterns_.size());
    for (const auto& pair : optional_patterns_) {
        keys.push_back(pair.first);
    }
    return keys;
}

std::vector<PatternKey> ReusablePatternRegistry::get_all_repeatable_keys() const {
    std::vector<PatternKey> keys;
    keys.reserve(repeatable_patterns_.size());
    for (const auto& pair : repeatable_patterns_) {
        keys.push_back(pair.first);
    }
    return keys;
}

std::vector<PatternKey> ReusablePatternRegistry::get_all_reusable_keys() const {
    auto optional_keys = get_all_optional_keys();
    auto repeatable_keys = get_all_repeatable_keys();
    
    std::vector<PatternKey> all_keys;
    all_keys.reserve(optional_keys.size() + repeatable_keys.size());
    all_keys.insert(all_keys.end(), optional_keys.begin(), optional_keys.end());
    all_keys.insert(all_keys.end(), repeatable_keys.begin(), repeatable_keys.end());
    
    return all_keys;
}

// Validation and debugging
bool ReusablePatternRegistry::validate_pattern_dependencies() const {
    auto logger = cprime::LoggerFactory::get_logger("reusable_pattern_registry");
    
    bool all_valid = true;
    
    // Check that all pattern dependencies exist
    for (const auto& [key, meta] : metadata_) {
        for (PatternKey dependency : meta.dependencies) {
            if (!is_reusable_pattern(dependency)) {
                LOG_ERROR("Pattern {} depends on non-existent pattern {}", 
                         pattern_key_to_string(key), pattern_key_to_string(dependency));
                all_valid = false;
            }
        }
    }
    
    return all_valid;
}

void ReusablePatternRegistry::log_registry_state() const {
    auto logger = cprime::LoggerFactory::get_logger("reusable_pattern_registry");
    
    LOG_INFO("🏗️ Reusable Pattern Registry State:");
    LOG_INFO("  Optional patterns: {}", optional_patterns_.size());
    for (const auto& [key, pattern] : optional_patterns_) {
        LOG_INFO("    {}: {} ({} elements)", 
                pattern_key_to_string(key), 
                get_pattern_description(key),
                pattern->elements.size());
    }
    
    LOG_INFO("  Repeatable patterns: {}", repeatable_patterns_.size());
    for (const auto& [key, pattern] : repeatable_patterns_) {
        LOG_INFO("    {}: {} ({} elements)", 
                pattern_key_to_string(key), 
                get_pattern_description(key),
                pattern->elements.size());
    }
}

std::string ReusablePatternRegistry::get_pattern_description(PatternKey key) const {
    auto it = metadata_.find(key);
    return (it != metadata_.end()) ? it->second.description : "Unknown pattern";
}

// Built-in pattern initialization
void ReusablePatternRegistry::initialize_builtin_reusable_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("reusable_pattern_registry");
    LOG_INFO("🏗️ Initializing builtin reusable patterns");
    
    // Optional Assignment Pattern: [= expression]
    // This can be reused in variable declarations, parameter defaults, etc.
    {
        std::vector<PatternElement> elements = {
            PatternElement(EToken::ASSIGN, EContextualToken::OPERATOR),
            PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::EXPRESSION)
        };
        Pattern assignment_pattern("optional_assignment", elements);
        register_optional_pattern(PatternKey::OPTIONAL_ASSIGNMENT, assignment_pattern, 
                                "Optional assignment: = expression");
    }
    
    // Optional Type Modifier Pattern: [const|volatile|static]
    // Reusable for variable declarations, function parameters, etc.
    {
        std::vector<PatternElement> elements = {
            PatternElement({EToken::CONST, EToken::VOLATILE, EToken::STATIC}, EContextualToken::TYPE_REFERENCE)
        };
        Pattern modifier_pattern("optional_type_modifier", elements);
        register_optional_pattern(PatternKey::OPTIONAL_TYPE_MODIFIER, modifier_pattern,
                                "Optional type modifier: const|volatile|static");
    }
    
    // Repeatable Namespace Pattern: (::identifier)+
    // For handling namespace::nested::identifier chains
    {
        std::vector<PatternElement> elements = {
            PatternElement(EToken::COLON, EContextualToken::SCOPE_REFERENCE),
            PatternElement(EToken::COLON, EContextualToken::SCOPE_REFERENCE),
            PatternElement(EToken::IDENTIFIER, EContextualToken::SCOPE_REFERENCE)
        };
        Pattern namespace_pattern("repeatable_namespace", elements);
        register_repeatable_pattern(PatternKey::REPEATABLE_NAMESPACE, namespace_pattern,
                                  "Repeatable namespace resolution: ::identifier");
    }
    
    // Advanced Optional Pattern: Multiple Type Modifiers [const|static|volatile]*
    {
        std::vector<PatternElement> elements = {
            PatternElement({EToken::CONST, EToken::STATIC, EToken::VOLATILE}, EContextualToken::TYPE_REFERENCE)
        };
        Pattern multi_modifier_pattern("optional_multi_modifier", elements);
        register_optional_pattern(PatternKey::OPTIONAL_ACCESS_MODIFIER, multi_modifier_pattern,
                                "Optional multiple type modifiers");
    }
    
    // Debug: Log all registered patterns
    log_registry_state();
    
    LOG_INFO("✅ Builtin reusable patterns initialized: {} optional, {} repeatable",
             optional_patterns_.size(), repeatable_patterns_.size());
}

// Helper methods
void ReusablePatternRegistry::validate_pattern_key_range(PatternKey key, bool is_optional) const {
    uint16_t key_value = static_cast<uint16_t>(key);
    
    if (is_optional) {
        // Optional patterns: 0x1000-0x1FFF
        if (key_value < 0x1000 || key_value >= 0x2000) {
            throw std::runtime_error("Optional pattern key out of range: " + pattern_key_to_string(key));
        }
    } else {
        // Repeatable patterns: 0x2000-0x2FFF
        if (key_value < 0x2000 || key_value >= 0x3000) {
            throw std::runtime_error("Repeatable pattern key out of range: " + pattern_key_to_string(key));
        }
    }
}

std::string ReusablePatternRegistry::pattern_key_to_string(PatternKey key) const {
    switch (key) {
        case PatternKey::OPTIONAL_ASSIGNMENT: return "OPTIONAL_ASSIGNMENT";
        case PatternKey::OPTIONAL_TYPE_MODIFIER: return "OPTIONAL_TYPE_MODIFIER";
        case PatternKey::OPTIONAL_ACCESS_MODIFIER: return "OPTIONAL_ACCESS_MODIFIER";
        case PatternKey::OPTIONAL_WHITESPACE_PATTERN: return "OPTIONAL_WHITESPACE_PATTERN";
        case PatternKey::REPEATABLE_NAMESPACE: return "REPEATABLE_NAMESPACE";
        case PatternKey::REPEATABLE_PARAMETER_LIST: return "REPEATABLE_PARAMETER_LIST";
        case PatternKey::REPEATABLE_TEMPLATE_ARGS: return "REPEATABLE_TEMPLATE_ARGS";
        default: return "UNKNOWN_PATTERN_" + std::to_string(static_cast<uint16_t>(key));
    }
}

} // namespace cprime::layer2_contextualization