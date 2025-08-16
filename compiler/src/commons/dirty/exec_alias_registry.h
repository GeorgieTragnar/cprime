#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <cassert>

namespace cprime {

// Stub structure for executable lambdas created from exec blocks
// Will be expanded in Sublayer 2B implementation
struct ExecutableLambda {
    // Empty stub for now - will contain compiled exec block logic
};

}

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
    
    /**
     * Register a scope index as an exec scope with empty ExecutableLambda.
     */
    void register_scope_index(uint32_t scope_index);
    
    /**
     * Register a scope index to an exec alias using alias index value.
     */
    void register_scope_index_to_exec_alias(ExecAliasIndex alias_idx, uint32_t scope_index);
    
    /**
     * Get executable lambda by scope index.
     */
    const ExecutableLambda& get_executable_lambda(uint32_t scope_index) const;
    
    /**
     * Get executable lambda by exec alias index (two-step lookup).
     */
    const ExecutableLambda& get_executable_lambda_by_alias(ExecAliasIndex alias_idx) const;
    
    /**
     * Get the number of registered exec scopes.
     */
    size_t get_exec_scope_count() const { return scope_to_lambda_.size(); }
    
    /**
     * Get the number of registered alias-to-scope mappings.
     */
    size_t get_alias_to_scope_count() const { return alias_to_scope_.size(); }

private:
    std::vector<std::string> aliases_;                                    // Indexed alias storage
    std::unordered_map<std::string, ExecAliasIndex> alias_to_index_;      // Fast lookup for registration
    
    // Exec scope registration maps
    std::unordered_map<uint32_t, ExecutableLambda> scope_to_lambda_;      // Scope indices → executable lambdas
    std::unordered_map<uint32_t, uint32_t> alias_to_scope_;              // Exec alias indices → scope indices
};

} // namespace cprime