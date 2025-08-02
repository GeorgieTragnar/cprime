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

bool SymbolTable::is_compatible(Type from, Type to) {
    return from == to || to == Type::AUTO;
}

} // namespace cprime