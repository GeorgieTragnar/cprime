#include "symbol_table.h"
#include <stdexcept>

namespace cprime {

SymbolTable::SymbolTable() {
    // Start with global scope
    enter_scope();
}

void SymbolTable::enter_scope() {
    scopes.emplace_back();
}

void SymbolTable::exit_scope() {
    if (scopes.empty()) {
        throw std::runtime_error("Cannot exit scope: no scopes active");
    }
    scopes.pop_back();
}

bool SymbolTable::declare_variable(const std::string& name, Type type) {
    if (scopes.empty()) {
        throw std::runtime_error("No active scope for variable declaration");
    }
    
    auto& current_scope = scopes.back();
    
    // Check if variable already exists in current scope
    if (current_scope.find(name) != current_scope.end()) {
        return false; // Variable already declared in this scope
    }
    
    // Add to current scope
    current_scope.emplace(name, Symbol(name, type, true));
    return true;
}

bool SymbolTable::assign_variable(const std::string& name) {
    Symbol* symbol = find_symbol(name);
    if (!symbol) {
        return false; // Variable not found
    }
    
    symbol->is_initialized = true;
    return true;
}

Symbol* SymbolTable::lookup_variable(const std::string& name) {
    return find_symbol(name);
}

Symbol* SymbolTable::find_symbol(const std::string& name) {
    // Search from innermost to outermost scope
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return &found->second;
        }
    }
    return nullptr;
}

Type SymbolTable::deduce_type(const Expression& expr) {
    if (dynamic_cast<const BooleanLiteral*>(&expr)) {
        return Type::BOOL;
    } else if (dynamic_cast<const NumberLiteral*>(&expr)) {
        return Type::INT;
    } else if (auto var_ref = dynamic_cast<const VariableReference*>(&expr)) {
        Symbol* symbol = lookup_variable(var_ref->name);
        return symbol ? symbol->type : Type::VOID;
    } else if (auto binary = dynamic_cast<const BinaryExpression*>(&expr)) {
        // For arithmetic operators, result is int
        // For comparison operators, result is bool
        if (binary->operator_token == "+" || binary->operator_token == "-" ||
            binary->operator_token == "*" || binary->operator_token == "/" ||
            binary->operator_token == "%") {
            return Type::INT;
        } else {
            return Type::BOOL;
        }
    }
    return Type::VOID;
}

bool SymbolTable::declare_variable(const std::string& name, const std::string& custom_type_name) {
    if (scopes.empty()) {
        throw std::runtime_error("No active scope for variable declaration");
    }
    
    auto& current_scope = scopes.back();
    
    // Check if variable already exists in current scope
    if (current_scope.find(name) != current_scope.end()) {
        return false; // Variable already declared in this scope
    }
    
    // Check if custom type exists
    if (classes.find(custom_type_name) == classes.end()) {
        throw std::runtime_error("Unknown custom type: " + custom_type_name);
    }
    
    // Add to current scope
    current_scope.emplace(name, Symbol(name, custom_type_name, true));
    return true;
}

void SymbolTable::register_class(const ClassDefinition& class_def) {
    ClassInfo class_info(class_def.name);
    
    // Register fields
    for (const auto& field : class_def.fields) {
        if (field->type == Type::CUSTOM && field->custom_type) {
            class_info.custom_fields.emplace_back(field->name, field->custom_type->name);
        } else {
            class_info.fields.emplace_back(field->name, field->type);
        }
    }
    
    classes[class_def.name] = std::move(class_info);
}

ClassInfo* SymbolTable::lookup_class(const std::string& name) {
    auto it = classes.find(name);
    return (it != classes.end()) ? &it->second : nullptr;
}

bool SymbolTable::has_field(const std::string& class_name, const std::string& field_name) {
    ClassInfo* class_info = lookup_class(class_name);
    if (!class_info) return false;
    
    // Check built-in type fields
    for (const auto& field : class_info->fields) {
        if (field.first == field_name) {
            return true;
        }
    }
    
    // Check custom type fields
    for (const auto& field : class_info->custom_fields) {
        if (field.first == field_name) {
            return true;
        }
    }
    
    return false;
}

Type SymbolTable::get_field_type(const std::string& class_name, const std::string& field_name) {
    ClassInfo* class_info = lookup_class(class_name);
    if (!class_info) return Type::VOID;
    
    // Check built-in type fields
    for (const auto& field : class_info->fields) {
        if (field.first == field_name) {
            return field.second;
        }
    }
    
    // Check custom type fields
    for (const auto& field : class_info->custom_fields) {
        if (field.first == field_name) {
            return Type::CUSTOM;
        }
    }
    
    return Type::VOID;
}

std::string SymbolTable::get_field_custom_type(const std::string& class_name, const std::string& field_name) {
    ClassInfo* class_info = lookup_class(class_name);
    if (!class_info) return "";
    
    // Check custom type fields
    for (const auto& field : class_info->custom_fields) {
        if (field.first == field_name) {
            return field.second;
        }
    }
    
    return "";
}

bool SymbolTable::is_compatible(Type from, Type to) {
    return from == to || to == Type::AUTO;
}

} // namespace cprime