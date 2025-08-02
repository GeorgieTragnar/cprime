#pragma once

#include "ast.h"
#include <unordered_map>
#include <vector>
#include <string>

namespace cprime {

// Symbol information
struct Symbol {
    std::string name;
    Type type;
    bool is_initialized;
    
    Symbol(const std::string& name, Type type, bool is_initialized = false)
        : name(name), type(type), is_initialized(is_initialized) {}
};

// Scope management for variables
class SymbolTable {
public:
    SymbolTable();
    
    // Scope management
    void enter_scope();
    void exit_scope();
    
    // Symbol operations
    bool declare_variable(const std::string& name, Type type);
    bool assign_variable(const std::string& name);
    Symbol* lookup_variable(const std::string& name);
    
    // Type operations
    Type deduce_type(const Expression& expr);
    bool is_compatible(Type from, Type to);
    
private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
    
    // Helper to find symbol in scope chain
    Symbol* find_symbol(const std::string& name);
};

} // namespace cprime