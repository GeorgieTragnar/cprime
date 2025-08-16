#include "lua_exec_runtime.h"
#include <stdexcept>
#include <iostream>

namespace cprime {

LuaExecRuntime::LuaExecRuntime() : L_(nullptr) {
    initialize_lua();
}

LuaExecRuntime::~LuaExecRuntime() {
    if (L_) {
        lua_close(L_);
    }
}

void LuaExecRuntime::initialize_lua() {
    // Create new Lua state
    L_ = luaL_newstate();
    if (!L_) {
        throw std::runtime_error("Failed to create Lua state");
    }
    
    // Load standard Lua libraries
    luaL_openlibs(L_);
    
    // Store 'this' pointer in Lua registry for API functions
    lua_pushlightuserdata(L_, this);
    lua_setfield(L_, LUA_REGISTRYINDEX, "cprime_runtime");
    
    // Setup CPrime API
    setup_cprime_api();
}

void LuaExecRuntime::setup_cprime_api() {
    // Create cprime table
    lua_newtable(L_);
    
    // Register API functions
    lua_pushcfunction(L_, lua_get_param);
    lua_setfield(L_, -2, "get_param");
    
    lua_pushcfunction(L_, lua_emit);
    lua_setfield(L_, -2, "emit");
    
    lua_pushcfunction(L_, lua_emit_line);
    lua_setfield(L_, -2, "emit_line");
    
    lua_pushcfunction(L_, lua_param_count);
    lua_setfield(L_, -2, "param_count");
    
    // Set cprime table as global
    lua_setglobal(L_, "cprime");
}

std::string LuaExecRuntime::execute_script(const std::string& lua_script, 
                                          const std::vector<std::string>& parameters) {
    last_error_.clear();
    clear_output();
    
    try {
        // Set parameters for the script
        set_parameters(parameters);
        
        // Load and execute the script
        int result = luaL_loadstring(L_, lua_script.c_str());
        if (result != LUA_OK) {
            last_error_ = lua_tostring(L_, -1);
            lua_pop(L_, 1);
            throw std::runtime_error("Lua script compilation failed: " + last_error_);
        }
        
        // Execute the loaded script
        result = lua_pcall(L_, 0, 0, 0);
        if (result != LUA_OK) {
            last_error_ = lua_tostring(L_, -1);
            lua_pop(L_, 1);
            throw std::runtime_error("Lua script execution failed: " + last_error_);
        }
        
        return get_output();
        
    } catch (const std::exception& e) {
        last_error_ = e.what();
        throw;
    }
}

bool LuaExecRuntime::validate_script(const std::string& script) {
    last_error_.clear();
    
    int result = luaL_loadstring(L_, script.c_str());
    if (result != LUA_OK) {
        last_error_ = lua_tostring(L_, -1);
        lua_pop(L_, 1);
        return false;
    }
    
    // Remove the loaded function from the stack without executing
    lua_pop(L_, 1);
    return true;
}

void LuaExecRuntime::set_parameters(const std::vector<std::string>& parameters) {
    // Create parameters table
    lua_newtable(L_);
    
    // Add each parameter to the table (1-indexed for Lua)
    for (size_t i = 0; i < parameters.size(); ++i) {
        lua_pushinteger(L_, static_cast<lua_Integer>(i));
        lua_pushstring(L_, parameters[i].c_str());
        lua_settable(L_, -3);
    }
    
    // Store parameters table in registry
    lua_setfield(L_, LUA_REGISTRYINDEX, "cprime_parameters");
}

void LuaExecRuntime::clear_output() {
    output_buffer_.clear();
}

// Static API functions for Lua

LuaExecRuntime* LuaExecRuntime::get_runtime(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "cprime_runtime");
    LuaExecRuntime* runtime = static_cast<LuaExecRuntime*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return runtime;
}

int LuaExecRuntime::lua_get_param(lua_State* L) {
    // Get parameter index from argument
    if (!lua_isinteger(L, 1)) {
        return luaL_error(L, "cprime.get_param expects integer parameter index");
    }
    
    lua_Integer index = lua_tointeger(L, 1);
    
    // Get parameters table from registry
    lua_getfield(L, LUA_REGISTRYINDEX, "cprime_parameters");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return luaL_error(L, "No parameters available");
    }
    
    // Get parameter at index
    lua_pushinteger(L, index);
    lua_gettable(L, -2);
    
    if (lua_isnil(L, -1)) {
        lua_pop(L, 2);
        return luaL_error(L, "Parameter index %d out of range", (int)index);
    }
    
    // Remove parameters table, leave parameter value on stack
    lua_remove(L, -2);
    return 1;
}

int LuaExecRuntime::lua_emit(lua_State* L) {
    LuaExecRuntime* runtime = get_runtime(L);
    if (!runtime) {
        return luaL_error(L, "Failed to get runtime instance");
    }
    
    // Get string argument
    if (!lua_isstring(L, 1)) {
        return luaL_error(L, "cprime.emit expects string argument");
    }
    
    const char* str = lua_tostring(L, 1);
    runtime->output_buffer_ += str;
    
    return 0;
}

int LuaExecRuntime::lua_emit_line(lua_State* L) {
    LuaExecRuntime* runtime = get_runtime(L);
    if (!runtime) {
        return luaL_error(L, "Failed to get runtime instance");
    }
    
    // Get string argument
    if (!lua_isstring(L, 1)) {
        return luaL_error(L, "cprime.emit_line expects string argument");
    }
    
    const char* str = lua_tostring(L, 1);
    runtime->output_buffer_ += str;
    runtime->output_buffer_ += "\n";
    
    return 0;
}

int LuaExecRuntime::lua_param_count(lua_State* L) {
    // Get parameters table from registry
    lua_getfield(L, LUA_REGISTRYINDEX, "cprime_parameters");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushinteger(L, 0);
        return 1;
    }
    
    // Get table length
    lua_Integer count = lua_rawlen(L, -1);
    lua_pop(L, 1);
    
    lua_pushinteger(L, count);
    return 1;
}

} // namespace cprime