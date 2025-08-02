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
    std::string custom_type_name;  // For CUSTOM types
    bool is_initialized;
    
    Symbol(const std::string& name, Type type, bool is_initialized = false)
        : name(name), type(type), is_initialized(is_initialized) {}
    
    Symbol(const std::string& name, const std::string& custom_type_name, bool is_initialized = false)
        : name(name), type(Type::CUSTOM), custom_type_name(custom_type_name), is_initialized(is_initialized) {}
};

// Class information for user-defined classes
struct ClassInfo {
    std::string name;
    std::vector<std::pair<std::string, Type>> fields;           // Built-in type fields
    std::vector<std::pair<std::string, std::string>> custom_fields;  // Custom type fields
    
    ClassInfo() = default;  // Default constructor for unordered_map
    ClassInfo(const std::string& name) : name(name) {}
    
    // Make it movable for unordered_map
    ClassInfo(const ClassInfo&) = default;
    ClassInfo(ClassInfo&&) = default;
    ClassInfo& operator=(const ClassInfo&) = default;
    ClassInfo& operator=(ClassInfo&&) = default;
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
    bool declare_variable(const std::string& name, const std::string& custom_type_name);
    bool assign_variable(const std::string& name);
    Symbol* lookup_variable(const std::string& name);
    
    // Class operations
    void register_class(const ClassDefinition& class_def);
    ClassInfo* lookup_class(const std::string& name);
    bool has_field(const std::string& class_name, const std::string& field_name);
    Type get_field_type(const std::string& class_name, const std::string& field_name);
    std::string get_field_custom_type(const std::string& class_name, const std::string& field_name);
    
    // Type operations
    Type deduce_type(const Expression& expr);
    bool is_compatible(Type from, Type to);
    
private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
    std::unordered_map<std::string, ClassInfo> classes;  // Registered classes
    
    // Helper to find symbol in scope chain
    Symbol* find_symbol(const std::string& name);
};

} // namespace cprime