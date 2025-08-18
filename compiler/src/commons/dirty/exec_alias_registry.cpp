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

// Simple Lua execution without full LuaExecRuntime to avoid circular deps
static std::string execute_lua_inline(const std::string& script, const std::vector<std::string>& parameters) {
    lua_State* L = luaL_newstate();
    if (!L) {
        return "// Error: Failed to create Lua state\n";
    }
    
    std::string output_buffer;
    
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
        
        // Set up simple cprime API
        lua_newtable(L);
        
        // cprime.emit function
        lua_pushstring(L, "emit");
        lua_pushlightuserdata(L, &output_buffer);
        lua_pushcclosure(L, [](lua_State* L) -> int {
            std::string* buffer = static_cast<std::string*>(lua_touserdata(L, lua_upvalueindex(1)));
            if (lua_isstring(L, 1)) {
                *buffer += lua_tostring(L, 1);
            }
            return 0;
        }, 1);
        lua_settable(L, -3);
        
        // cprime.emit_line function
        lua_pushstring(L, "emit_line");
        lua_pushlightuserdata(L, &output_buffer);
        lua_pushcclosure(L, [](lua_State* L) -> int {
            std::string* buffer = static_cast<std::string*>(lua_touserdata(L, lua_upvalueindex(1)));
            if (lua_isstring(L, 1)) {
                *buffer += lua_tostring(L, 1);
                *buffer += "\n";
            }
            return 0;
        }, 1);
        lua_settable(L, -3);
        
        lua_setglobal(L, "cprime");
        
        // Execute the script and capture return value
        int result = luaL_loadstring(L, script.c_str());
        if (result != LUA_OK) {
            std::string error = "// Lua compilation error: ";
            error += lua_tostring(L, -1);
            lua_close(L);
            return error + "\n";
        }
        
        result = lua_pcall(L, 0, 1, 0);  // Expect 1 return value
        if (result != LUA_OK) {
            std::string error = "// Lua execution error: ";
            error += lua_tostring(L, -1);
            lua_close(L);
            return error + "\n";
        }
        
        // Get return value from Lua script
        std::string return_value;
        if (lua_isstring(L, -1)) {
            return_value = lua_tostring(L, -1);
        } else {
            return_value = "No return value";
        }
        
        lua_close(L);
        
        // Combine generated output and return value
        std::string full_result = output_buffer;
        if (!return_value.empty()) {
            full_result += "\n=== LUA RETURN VALUE ===\n";
            full_result += return_value + "\n";
        }
        
        return full_result;
        
    } catch (const std::exception& e) {
        lua_close(L);
        return "// Exception during Lua execution: " + std::string(e.what()) + "\n";
    }
}

std::string ExecutableLambda::execute(const std::vector<std::string>& parameters) {
    if (lua_script.empty()) {
        return "";  // Empty script returns empty string
    }
    
    // Check if this is a specialization (not a direct Lua script)
    if (lua_script.substr(0, 15) == "SPECIALIZATION:") {
        // Extract CPrime content from specialization
        std::string cprime_content = lua_script.substr(15); // Remove "SPECIALIZATION:" prefix
        
        // For the basic execute method, just return the content directly
        // The registry-aware execute method will handle parent delegation
        return "SPECIALIZATION_EXECUTED: " + cprime_content;
    }
    
    // Execute the Lua script and get the return value
    lua_State* L = luaL_newstate();
    if (!L) {
        throw std::runtime_error("Failed to create Lua state");
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
        
        // Load and execute the Lua script
        int load_result = luaL_loadstring(L, lua_script.c_str());
        if (load_result != LUA_OK) {
            std::string error = "Lua syntax error: ";
            if (lua_isstring(L, -1)) {
                error += lua_tostring(L, -1);
            }
            lua_close(L);
            throw std::runtime_error(error);
        }
        
        // Execute the script - expect exactly 1 return value
        int call_result = lua_pcall(L, 0, 1, 0);  // 0 args, 1 return value, 0 error handler
        if (call_result != LUA_OK) {
            std::string error = "Lua execution error: ";
            if (lua_isstring(L, -1)) {
                error += lua_tostring(L, -1);
            }
            lua_close(L);
            throw std::runtime_error(error);
        }
        
        // Check that we got exactly 1 return value and it's a string
        int return_count = lua_gettop(L);
        if (return_count != 1) {
            lua_close(L);
            throw std::runtime_error("Lua script must return exactly 1 value, got " + std::to_string(return_count));
        }
        
        if (!lua_isstring(L, -1)) {
            lua_close(L);
            throw std::runtime_error("Lua script must return a string value");
        }
        
        // Extract the single string return value
        std::string result = lua_tostring(L, -1);
        lua_close(L);
        
        return result;
        
    } catch (...) {
        lua_close(L);
        throw;  // Re-throw the exception
    }
}

std::string ExecutableLambda::execute(const std::vector<std::string>& parameters, ExecAliasRegistry* registry, uint32_t scope_index) {
    if (lua_script.empty()) {
        return "";  // Empty script returns empty string
    }
    
    // Check if this is a specialization (not a direct Lua script)
    if (lua_script.substr(0, 15) == "SPECIALIZATION:") {
        // Extract CPrime content from specialization
        std::string cprime_content = lua_script.substr(15); // Remove "SPECIALIZATION:" prefix
        
        // Get the parent alias name for this specialization
        std::string parent_alias_name = registry->get_parent_alias_name(scope_index);
        if (parent_alias_name.empty()) {
            return "// Error: No parent alias found for specialization";
        }
        
        // Find the parent's executable lambda
        ExecAliasIndex parent_alias_index = registry->get_alias_index(parent_alias_name);
        if (parent_alias_index.value == UINT32_MAX) {
            return "// Error: Parent alias '" + parent_alias_name + "' not found in registry";
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
            
            // Execute the parent's Lua script with specialization content as first parameter
            return execute_lua_inline(parent_lambda.lua_script, parent_parameters);
            
        } catch (const std::exception& e) {
            return "// Error executing parent '" + parent_alias_name + "': " + std::string(e.what());
        }
    }
    
    // Regular Lua script execution
    return execute_lua_inline(lua_script, parameters);
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