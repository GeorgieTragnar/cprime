#include "exec_alias_registry.h"
#include <stdexcept>
#include <algorithm>
#include <sstream>

// Forward declaration to avoid circular dependency
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

namespace cprime {

// Enhanced Lua execution that returns structured results from 2-3 string returns
static ExecResult execute_lua_with_result(const std::string& script, const std::vector<std::string>& parameters) {
    lua_State* L = luaL_newstate();
    if (!L) {
        ExecResult error_result;
        error_result.generated_code = "// Error: Failed to create Lua state\n";
        error_result.integration_type = "token";  // Default to token for error cases
        error_result.is_valid = false;
        return error_result;
    }
    
    try {
        // Load standard libraries
        luaL_openlibs(L);
        
        // Set up parameters table
        lua_newtable(L);
        for (size_t i = 0; i < parameters.size(); ++i) {
            lua_pushinteger(L, static_cast<lua_Integer>(i));
            lua_pushstring(L, parameters[i].c_str());
            lua_settable(L, -3);
        }
        lua_setglobal(L, "params");
        
        // Execute the script
        int result = luaL_loadstring(L, script.c_str());
        if (result != LUA_OK) {
            std::string error = "// Lua compilation error: ";
            if (lua_isstring(L, -1)) {
                error += lua_tostring(L, -1);
            }
            lua_close(L);
            return ExecResult(error + "\n", "token");  // Error defaults to token
        }
        
        result = lua_pcall(L, 0, LUA_MULTRET, 0);  // Accept multiple return values
        if (result != LUA_OK) {
            std::string error = "// Lua execution error: ";
            if (lua_isstring(L, -1)) {
                error += lua_tostring(L, -1);
            }
            lua_close(L);
            return ExecResult(error + "\n", "token");  // Error defaults to token
        }
        
        // Check the number of return values
        int return_count = lua_gettop(L);
        ExecResult exec_result;
        
        if (return_count == 1) {
            // Single string return - backward compatibility (assume "token")
            if (lua_isstring(L, -1)) {
                exec_result.generated_code = lua_tostring(L, -1);
                exec_result.integration_type = "token";  // Default for backward compatibility
                exec_result.is_valid = true;
            } else {
                exec_result.generated_code = "// Error: Single return value must be string";
                exec_result.integration_type = "token";
                exec_result.is_valid = false;
            }
        } else if (return_count == 2) {
            // Two string returns: code, integration_type
            if (lua_isstring(L, -2) && lua_isstring(L, -1)) {
                exec_result.generated_code = lua_tostring(L, -2);
                exec_result.integration_type = lua_tostring(L, -1);
                
                // Validate integration type
                if (exec_result.integration_type == "token" || 
                    exec_result.integration_type == "scope_insert" || 
                    exec_result.integration_type == "scope_create") {
                    exec_result.is_valid = true;
                } else {
                    exec_result.generated_code = "// Error: Invalid integration type '" + exec_result.integration_type + "'. Must be: token, scope_insert, or scope_create";
                    exec_result.integration_type = "token";
                    exec_result.is_valid = false;
                }
            } else {
                exec_result.generated_code = "// Error: Two return values must both be strings";
                exec_result.integration_type = "token";
                exec_result.is_valid = false;
            }
        } else if (return_count == 3) {
            // Three string returns: code, integration_type, identifier
            if (lua_isstring(L, -3) && lua_isstring(L, -2) && lua_isstring(L, -1)) {
                exec_result.generated_code = lua_tostring(L, -3);
                exec_result.integration_type = lua_tostring(L, -2);
                exec_result.identifier = lua_tostring(L, -1);
                
                // Validate integration type and identifier requirements
                if (exec_result.integration_type == "scope_create") {
                    if (!exec_result.identifier.empty()) {
                        exec_result.is_valid = true;
                    } else {
                        exec_result.generated_code = "// Error: scope_create requires non-empty identifier (third parameter)";
                        exec_result.integration_type = "token";
                        exec_result.identifier = "";
                        exec_result.is_valid = false;
                    }
                } else {
                    exec_result.generated_code = "// Error: Third parameter (identifier) only valid for scope_create integration type";
                    exec_result.integration_type = "token";
                    exec_result.identifier = "";
                    exec_result.is_valid = false;
                }
            } else {
                exec_result.generated_code = "// Error: Three return values must all be strings";
                exec_result.integration_type = "token";
                exec_result.is_valid = false;
            }
        } else {
            // Invalid number of return values
            exec_result.generated_code = "// Error: Lua script must return 1, 2, or 3 strings, got " + std::to_string(return_count) + " values";
            exec_result.integration_type = "token";
            exec_result.is_valid = false;
        }
        
        lua_close(L);
        return exec_result;
        
    } catch (const std::exception& e) {
        lua_close(L);
        return ExecResult("// Exception during Lua execution: " + std::string(e.what()) + "\n", "token");
    }
}


ExecResult ExecutableLambda::execute(const std::vector<std::string>& parameters) {
    if (lua_script.empty()) {
        return ExecResult("", "token");  // Empty script returns empty token
    }
    
    // Check if this is a specialization (not a direct Lua script)
    if (lua_script.substr(0, 15) == "SPECIALIZATION:") {
        // Extract CPrime content from specialization
        std::string cprime_content = lua_script.substr(15); // Remove "SPECIALIZATION:" prefix
        
        // For the basic execute method, just return the content directly as token
        return ExecResult("SPECIALIZATION_EXECUTED: " + cprime_content, "token");
    }
    
    // Execute the Lua script using enhanced execution
    return execute_lua_with_result(lua_script, parameters);
}

ExecResult ExecutableLambda::execute(const std::vector<std::string>& parameters, ExecAliasRegistry* registry, uint32_t scope_index) {
    if (lua_script.empty()) {
        return ExecResult("", "token");  // Empty script returns empty token
    }
    
    // Check if this is a specialization (not a direct Lua script)
    if (lua_script.substr(0, 15) == "SPECIALIZATION:") {
        // Extract CPrime content from specialization
        std::string cprime_content = lua_script.substr(15); // Remove "SPECIALIZATION:" prefix
        
        // Get the parent alias name for this specialization
        std::string parent_alias_name = registry->get_parent_alias_name(scope_index);
        if (parent_alias_name.empty()) {
            return ExecResult("// Error: No parent alias found for specialization", "token");
        }
        
        // Find the parent's executable lambda
        ExecAliasIndex parent_alias_index = registry->get_alias_index(parent_alias_name);
        if (parent_alias_index.value == UINT32_MAX) {
            return ExecResult("// Error: Parent alias '" + parent_alias_name + "' not found in registry", "token");
        }
        
        try {
            const ExecutableLambda& parent_lambda = registry->get_executable_lambda_by_alias(parent_alias_index);
            
            // Create parameters vector with CPrime content as first parameter
            std::vector<std::string> parent_parameters;
            parent_parameters.push_back(cprime_content);
            
            // Add any additional parameters passed to the specialization
            for (const auto& param : parameters) {
                parent_parameters.push_back(param);
            }
            
            // Execute the parent's Lua script with enhanced result structure
            return execute_lua_with_result(parent_lambda.lua_script, parent_parameters);
            
        } catch (const std::exception& e) {
            return ExecResult("// Error executing parent '" + parent_alias_name + "': " + std::string(e.what()), "token");
        }
    }
    
    // Regular Lua script execution with enhanced result structure
    return execute_lua_with_result(lua_script, parameters);
}


ExecAliasIndex ExecAliasRegistry::register_alias(const std::string& alias_name) {
    // Check for duplicate registration - assert for now with TODO for proper error handling
    auto existing = alias_to_index_.find(alias_name);
    if (existing != alias_to_index_.end()) {
        // TODO: Replace with proper error handling/reporting system
        assert(false && "Duplicate exec alias registration detected! Each exec template name must be unique.");
        return existing->second; // This line won't execute due to assertion
    }
    
    // Add new alias
    ExecAliasIndex new_index;
    new_index.value = static_cast<uint32_t>(aliases_.size());
    
    aliases_.push_back(alias_name);
    alias_to_index_[alias_name] = new_index;
    
    return new_index;
}

bool ExecAliasRegistry::contains_alias(const std::string& alias_name) const {
    return alias_to_index_.find(alias_name) != alias_to_index_.end();
}

ExecAliasIndex ExecAliasRegistry::get_alias_index(const std::string& alias_name) const {
    auto it = alias_to_index_.find(alias_name);
    if (it != alias_to_index_.end()) {
        return it->second;
    }
    // Return invalid index
    ExecAliasIndex invalid_index;
    invalid_index.value = UINT32_MAX;
    return invalid_index;
}

const std::string& ExecAliasRegistry::get_alias(ExecAliasIndex index) const {
    if (!is_valid_index(index)) {
        throw std::out_of_range("ExecAliasRegistry: Invalid alias index");
    }
    return aliases_[index.value];
}

ExecAliasRegistry::Statistics ExecAliasRegistry::get_statistics() const {
    Statistics stats;
    stats.registered_aliases = aliases_.size();
    stats.total_characters = 0;
    stats.longest_alias_length = 0;
    
    for (const auto& alias : aliases_) {
        stats.total_characters += alias.length();
        stats.longest_alias_length = std::max(stats.longest_alias_length, alias.length());
    }
    
    stats.average_alias_length = aliases_.empty() ? 0 : stats.total_characters / aliases_.size();
    
    return stats;
}

void ExecAliasRegistry::clear() {
    aliases_.clear();
    alias_to_index_.clear();
    namespace_paths_.clear();
    alias_reverse_map_.clear();
    scope_to_lambda_.clear();
    alias_to_scope_.clear();
    specialization_to_parent_.clear();
}

void ExecAliasRegistry::reserve(size_t expected_aliases) {
    aliases_.reserve(expected_aliases);
    alias_to_index_.reserve(expected_aliases);
}

std::unordered_map<std::string, ExecAliasIndex> ExecAliasRegistry::get_all_aliases() const {
    return alias_to_index_;
}

void ExecAliasRegistry::register_scope_index(uint32_t scope_index) {
    // Create empty ExecutableLambda stub using [] access
    scope_to_lambda_[scope_index] = ExecutableLambda{};
}

void ExecAliasRegistry::register_scope_index_to_exec_alias(ExecAliasIndex alias_idx, uint32_t scope_index) {
    // Register alias index value to scope index mapping
    alias_to_scope_[alias_idx.value] = scope_index;
}

const ExecutableLambda& ExecAliasRegistry::get_executable_lambda(uint32_t scope_index) const {
    auto it = scope_to_lambda_.find(scope_index);
    if (it == scope_to_lambda_.end()) {
        throw std::out_of_range("ExecAliasRegistry: Scope index not registered as exec scope");
    }
    return it->second;
}

const ExecutableLambda& ExecAliasRegistry::get_executable_lambda_by_alias(ExecAliasIndex alias_idx) const {
    // Two-step lookup: alias index -> scope index -> executable lambda
    auto alias_it = alias_to_scope_.find(alias_idx.value);
    if (alias_it == alias_to_scope_.end()) {
        throw std::out_of_range("ExecAliasRegistry: Exec alias index not mapped to any scope");
    }
    
    uint32_t scope_index = alias_it->second;
    return get_executable_lambda(scope_index);
}

uint32_t ExecAliasRegistry::get_scope_index_for_alias(ExecAliasIndex alias_idx) const {
    auto alias_it = alias_to_scope_.find(alias_idx.value);
    if (alias_it == alias_to_scope_.end()) {
        return UINT32_MAX; // Alias not mapped to any scope
    }
    return alias_it->second;
}

void ExecAliasRegistry::update_executable_lambda(uint32_t scope_index, const ExecutableLambda& lambda) {
    auto it = scope_to_lambda_.find(scope_index);
    if (it == scope_to_lambda_.end()) {
        throw std::out_of_range("ExecAliasRegistry: Scope index not registered as exec scope");
    }
    
    it->second = lambda;
}

// Namespace path system with anti-shadowing

ExecAliasIndex ExecAliasRegistry::register_namespaced_alias(const std::vector<std::string>& namespace_path) {
    if (namespace_path.empty()) {
        throw std::invalid_argument("Namespace path cannot be empty");
    }
    
    std::string alias_name = extract_alias_name(namespace_path);
    
    // Anti-shadowing protection: Check if alias exists in global scope
    if (!is_global_namespace(namespace_path)) {
        // Check if global alias already exists
        for (ExecAliasIndex global_idx : alias_reverse_map_[alias_name]) {
            if (is_global_namespace(namespace_paths_[global_idx.value])) {
                throw std::runtime_error("Cannot register namespaced alias '" + alias_name + 
                                       "' - global alias with same name already exists (anti-shadowing protection)");
            }
        }
    }
    
    // Check for exact duplicate path
    for (ExecAliasIndex existing_idx : alias_reverse_map_[alias_name]) {
        if (namespace_paths_[existing_idx.value] == namespace_path) {
            throw std::runtime_error("Duplicate namespace path registration: alias '" + alias_name + "'");
        }
    }
    
    // Create new index
    ExecAliasIndex new_index;
    new_index.value = static_cast<uint32_t>(namespace_paths_.size());
    
    // Store namespace path
    namespace_paths_.push_back(namespace_path);
    
    // Update reverse map
    alias_reverse_map_[alias_name].push_back(new_index);
    
    // Backward compatibility: also register simple alias if global scope
    if (is_global_namespace(namespace_path)) {
        if (alias_to_index_.find(alias_name) == alias_to_index_.end()) {
            aliases_.push_back(alias_name);
            alias_to_index_[alias_name] = new_index;
        }
    }
    
    return new_index;
}

bool ExecAliasRegistry::lookup_alias_with_context(const std::string& alias_name, 
                                                 const std::vector<std::string>& current_namespace_context,
                                                 std::vector<std::string>& found_namespace_path) const {
    
    // Get all namespace paths containing this alias name using reverse map
    auto reverse_it = alias_reverse_map_.find(alias_name);
    if (reverse_it == alias_reverse_map_.end()) {
        // Fallback to backward compatibility
        if (contains_alias(alias_name)) {
            found_namespace_path = {alias_name};
            return true;
        }
        return false;
    }
    
    const std::vector<ExecAliasIndex>& candidate_indices = reverse_it->second;
    
    // Anti-shadowing rule: If global alias exists, always prefer it
    for (ExecAliasIndex idx : candidate_indices) {
        const std::vector<std::string>& candidate_path = namespace_paths_[idx.value];
        if (is_global_namespace(candidate_path)) {
            found_namespace_path = candidate_path;
            return true;
        }
    }
    
    // Upward traversal: Try current namespace context from most specific to least specific
    for (int i = static_cast<int>(current_namespace_context.size()); i >= 0; --i) {
        std::vector<std::string> target_context(current_namespace_context.begin(), current_namespace_context.begin() + i);
        
        // Check if any candidate path matches this namespace level
        for (ExecAliasIndex idx : candidate_indices) {
            const std::vector<std::string>& candidate_path = namespace_paths_[idx.value];
            if (namespace_path_matches(candidate_path, target_context)) {
                found_namespace_path = candidate_path;
                return true;
            }
        }
    }
    
    return false;
}

ExecAliasIndex ExecAliasRegistry::get_alias_index_with_context(const std::string& alias_name,
                                                              const std::vector<std::string>& current_namespace_context) const {
    std::vector<std::string> found_namespace_path;
    if (lookup_alias_with_context(alias_name, current_namespace_context, found_namespace_path)) {
        // Find the ExecAliasIndex for this exact namespace path
        std::string found_alias_name = extract_alias_name(found_namespace_path);
        auto reverse_it = alias_reverse_map_.find(found_alias_name);
        if (reverse_it != alias_reverse_map_.end()) {
            for (ExecAliasIndex idx : reverse_it->second) {
                if (namespace_paths_[idx.value] == found_namespace_path) {
                    return idx;
                }
            }
        }
        // Fallback to backward compatibility
        return get_alias_index(alias_name);
    }
    
    // Return invalid index
    ExecAliasIndex invalid_index;
    invalid_index.value = UINT32_MAX;
    return invalid_index;
}

// Anti-shadowing lookup helper methods

bool ExecAliasRegistry::is_global_namespace(const std::vector<std::string>& namespace_path) const {
    // Global namespace has only one element (the alias name)
    return namespace_path.size() == 1;
}

std::string ExecAliasRegistry::extract_alias_name(const std::vector<std::string>& namespace_path) const {
    if (namespace_path.empty()) {
        throw std::invalid_argument("Cannot extract alias name from empty namespace path");
    }
    // Alias name is always the last element
    return namespace_path.back();
}

bool ExecAliasRegistry::namespace_path_matches(const std::vector<std::string>& candidate_path, 
                                              const std::vector<std::string>& current_context) const {
    // candidate_path = [namespace_parts..., alias_name]
    // current_context = [namespace_parts...]
    
    if (candidate_path.empty()) return false;
    
    // Extract namespace part (all but last element)
    std::vector<std::string> candidate_namespace(candidate_path.begin(), candidate_path.end() - 1);
    
    // Check if candidate namespace matches current context exactly
    return candidate_namespace == current_context;
}

void ExecAliasRegistry::register_specialization_to_parent(uint32_t specialization_scope_index, const std::string& parent_alias_name) {
    specialization_to_parent_[specialization_scope_index] = parent_alias_name;
}

std::string ExecAliasRegistry::get_parent_alias_name(uint32_t specialization_scope_index) const {
    auto it = specialization_to_parent_.find(specialization_scope_index);
    if (it != specialization_to_parent_.end()) {
        return it->second;
    }
    return ""; // Not a specialization or parent not found
}

} // namespace cprime