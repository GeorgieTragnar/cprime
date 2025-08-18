#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <cstdint>
#include <cassert>

namespace cprime {

// Forward declaration
class ExecAliasRegistry;

// Structure for executable lambdas created from exec blocks
// Contains Lua script and execution interface for exec blocks
struct ExecutableLambda {
    std::string lua_script;                    // Converted exec block as Lua script
    
    // Execution interface
    std::string execute(const std::vector<std::string>& parameters = {});
    std::string execute(const std::vector<std::string>& parameters, ExecAliasRegistry* registry, uint32_t scope_index);
    
    // Utilities
    bool is_empty() const { return lua_script.empty(); }
    bool has_script() const { return !lua_script.empty(); }
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
     * Register a namespace-qualified exec alias using namespace path.
     * Examples: {"ns1", "ns2", "foo"} registers foo in ns1::ns2 namespace
     * Single item {"foo"} registers in global scope
     * Anti-shadowing: Global aliases are forever unique, namespace aliases can coexist
     */
    ExecAliasIndex register_namespaced_alias(const std::vector<std::string>& namespace_path);
    
    /**
     * Check if an alias name is already registered.
     */
    bool contains_alias(const std::string& alias_name) const;
    
    /**
     * Smart namespace-aware alias lookup with anti-shadowing.
     * Searches for alias_name with upward traversal:
     * 1. Walk up current namespace hierarchy (most specific first)
     * 2. Prefer global scope if exists (anti-shadowing protection)
     * 3. Return most specific match found
     * Returns true if found, sets found_namespace_path to the matching entry.
     */
    bool lookup_alias_with_context(const std::string& alias_name, 
                                  const std::vector<std::string>& current_namespace_context,
                                  std::vector<std::string>& found_namespace_path) const;
    
    /**
     * Get the index of a registered alias name.
     * Returns invalid index if alias is not registered.
     */
    ExecAliasIndex get_alias_index(const std::string& alias_name) const;
    
    /**
     * Get the index of a namespace-qualified alias with anti-shadowing lookup.
     * Returns invalid index if alias is not found in the context.
     */
    ExecAliasIndex get_alias_index_with_context(const std::string& alias_name,
                                               const std::vector<std::string>& current_namespace_context) const;
    
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
    
    /**
     * Update an existing executable lambda with compiled content.
     */
    void update_executable_lambda(uint32_t scope_index, const ExecutableLambda& lambda);
    
    /**
     * Get read-only access to scope-to-lambda map for iteration.
     */
    const std::unordered_map<uint32_t, ExecutableLambda>& get_scope_to_lambda_map() const { 
        return scope_to_lambda_; 
    }
    
    /**
     * Get mutable access to scope-to-lambda map for execution.
     */
    std::unordered_map<uint32_t, ExecutableLambda>& get_scope_to_lambda_map() { 
        return scope_to_lambda_; 
    }
    
    /**
     * Register a parent-specialization relationship.
     * Links a specialization scope to its parent exec alias.
     */
    void register_specialization_to_parent(uint32_t specialization_scope_index, const std::string& parent_alias_name);
    
    /**
     * Get the parent alias name for a specialization scope.
     * Returns empty string if not a specialization or parent not found.
     */
    std::string get_parent_alias_name(uint32_t specialization_scope_index) const;

private:
    std::vector<std::string> aliases_;                                    // Indexed alias storage (for backward compatibility)
    std::unordered_map<std::string, ExecAliasIndex> alias_to_index_;      // Fast lookup for registration (for backward compatibility)
    
    // Namespace path storage with reverse map for efficient lookup
    std::vector<std::vector<std::string>> namespace_paths_;              // namespace paths, indexed by ExecAliasIndex
    std::unordered_map<std::string, std::vector<ExecAliasIndex>> alias_reverse_map_;  // alias_name → indices of namespace paths containing it
    
    // Exec scope registration maps
    std::unordered_map<uint32_t, ExecutableLambda> scope_to_lambda_;      // Scope indices → executable lambdas
    std::unordered_map<uint32_t, uint32_t> alias_to_scope_;              // Exec alias indices → scope indices
    
    // Parent-specialization relationship tracking
    std::unordered_map<uint32_t, std::string> specialization_to_parent_; // Specialization scope indices → parent alias names
    
    // Anti-shadowing lookup helpers
    bool is_global_namespace(const std::vector<std::string>& namespace_path) const;
    std::string extract_alias_name(const std::vector<std::string>& namespace_path) const;
    bool namespace_path_matches(const std::vector<std::string>& candidate_path, 
                               const std::vector<std::string>& current_context) const;
};

} // namespace cprime