#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <cassert>

namespace cprime {

/**
 * Exec alias index wrapper - distinct type for variant compatibility.
 */
struct ExecAliasIndex {
    uint32_t value = UINT32_MAX;
};

/**
 * Registry for exec template aliases discovered during tokenization.
 * Stores exec alias names to enable forward declaration and dynamic keyword recognition.
 * Thread-safe after construction is complete (immutable access).
 */
class ExecAliasRegistry {
public:
    
    ExecAliasRegistry() = default;
    
    /**
     * Register an exec alias name, returning its index.
     * Asserts if the alias already exists (duplicate detection).
     * TODO: Replace assertion with proper error handling for production.
     */
    ExecAliasIndex register_alias(const std::string& alias_name);
    
    /**
     * Check if an alias name is already registered.
     */
    bool contains_alias(const std::string& alias_name) const;
    
    /**
     * Get the index of a registered alias name.
     * Returns invalid index if alias is not registered.
     */
    ExecAliasIndex get_alias_index(const std::string& alias_name) const;
    
    /**
     * Get the alias name associated with the given index.
     * Throws std::out_of_range if index is invalid.
     */
    const std::string& get_alias(ExecAliasIndex index) const;
    
    /**
     * Check if an index is valid (within bounds).
     */
    bool is_valid_index(ExecAliasIndex index) const {
        return index.value < aliases_.size();
    }
    
    /**
     * Get the number of registered aliases.
     */
    size_t size() const { return aliases_.size(); }
    
    /**
     * Check if the registry is empty.
     */
    bool empty() const { return aliases_.empty(); }
    
    /**
     * Get statistics about the alias registry.
     */
    struct Statistics {
        size_t registered_aliases;
        size_t total_characters;
        size_t average_alias_length;
        size_t longest_alias_length;
    };
    
    Statistics get_statistics() const;
    
    /**
     * Clear the alias registry (useful for testing).
     */
    void clear();
    
    /**
     * Reserve space for expected number of aliases (optimization).
     */
    void reserve(size_t expected_aliases);
    
    /**
     * Get all registered aliases as a map (for debugging).
     */
    std::unordered_map<std::string, ExecAliasIndex> get_all_aliases() const;

private:
    std::vector<std::string> aliases_;                                    // Indexed alias storage
    std::unordered_map<std::string, ExecAliasIndex> alias_to_index_;      // Fast lookup for registration
};

} // namespace cprime