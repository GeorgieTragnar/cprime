#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <string>
#include <vector>
#include <memory>

namespace cprime {

/**
 * Lua runtime for executing exec block scripts.
 * Provides basic functionality to run Lua scripts with parameter passing
 * and string result extraction.
 */
class LuaExecRuntime {
public:
    LuaExecRuntime();
    ~LuaExecRuntime();
    
    // Non-copyable but movable
    LuaExecRuntime(const LuaExecRuntime&) = delete;
    LuaExecRuntime& operator=(const LuaExecRuntime&) = delete;
    LuaExecRuntime(LuaExecRuntime&&) = default;
    LuaExecRuntime& operator=(LuaExecRuntime&&) = default;
    
    /**
     * Execute a Lua script with string parameters.
     * @param lua_script The Lua script to execute
     * @param parameters Vector of string parameters to pass to the script
     * @return Generated string result from the script
     * @throws std::runtime_error if script execution fails
     */
    std::string execute_script(const std::string& lua_script, 
                              const std::vector<std::string>& parameters);
    
    /**
     * Validate Lua script syntax without executing.
     * @param script The Lua script to validate
     * @return true if syntax is valid, false otherwise
     */
    bool validate_script(const std::string& script);
    
    /**
     * Get last error message from Lua execution.
     * @return Error message string, empty if no error
     */
    std::string get_last_error() const { return last_error_; }

private:
    lua_State* L_;
    std::string last_error_;
    std::string output_buffer_;  // Accumulates output from cprime.emit()
    
    /**
     * Initialize Lua state and register CPrime API.
     */
    void initialize_lua();
    
    /**
     * Setup CPrime-specific API functions for Lua scripts.
     */
    void setup_cprime_api();
    
    /**
     * Set parameters that can be accessed by Lua scripts.
     */
    void set_parameters(const std::vector<std::string>& parameters);
    
    /**
     * Clear output buffer for new execution.
     */
    void clear_output();
    
    /**
     * Get accumulated output from script execution.
     */
    std::string get_output() const { return output_buffer_; }
    
    // Static C functions for Lua API
    static int lua_get_param(lua_State* L);
    static int lua_emit(lua_State* L);
    static int lua_emit_line(lua_State* L);
    static int lua_param_count(lua_State* L);
    
    // Helper to get runtime instance from Lua state
    static LuaExecRuntime* get_runtime(lua_State* L);
};

} // namespace cprime