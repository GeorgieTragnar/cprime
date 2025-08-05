#include "symbol_table.h"
#include <iostream>
#include <iomanip>

namespace cprime::v2 {

// ============================================================================
// Symbol implementation
// ============================================================================

std::string Symbol::to_string() const {
    std::string result = name + " (" + std::to_string(static_cast<int>(kind)) + ")";
    if (type) {
        result += " : " + type->to_string();
    }
    return result;
}

// ============================================================================
// Scope implementation
// ============================================================================

bool Scope::add_symbol(SymbolPtr symbol) {
    const std::string& name = symbol->get_name();
    if (symbols.find(name) != symbols.end()) {
        return false; // Symbol already exists
    }
    symbols[name] = symbol;
    return true;
}

SymbolPtr Scope::lookup_local(const std::string& name) const {
    auto it = symbols.find(name);
    return it != symbols.end() ? it->second : nullptr;
}

SymbolPtr Scope::lookup(const std::string& name) const {
    // Search current scope first
    auto symbol = lookup_local(name);
    if (symbol) {
        return symbol;
    }
    
    // Search parent scopes
    if (parent) {
        return parent->lookup(name);
    }
    
    return nullptr;
}

Scope* Scope::create_child_scope(Kind kind, const std::string& name) {
    auto child = std::make_unique<Scope>(kind, name, this);
    Scope* child_ptr = child.get();
    children.push_back(std::move(child));
    return child_ptr;
}

std::string Scope::get_qualified_name() const {
    if (parent && parent->kind != Kind::Global) {
        return parent->get_qualified_name() + "::" + name;
    }
    return name;
}

void Scope::dump(int indent) const {
    std::string prefix(indent * 2, ' ');
    std::cout << prefix << "Scope: " << name << " (kind=" << static_cast<int>(kind) << ")\n";
    
    for (const auto& [sym_name, symbol] : symbols) {
        std::cout << prefix << "  - " << symbol->to_string() << "\n";
    }
    
    for (const auto& child : children) {
        child->dump(indent + 1);
    }
}

// ============================================================================
// SymbolTable implementation
// ============================================================================

SymbolTable::SymbolTable() {
    global_scope = std::make_unique<Scope>(Scope::Kind::Global, "global");
    current_scope = global_scope.get();
}

void SymbolTable::enter_scope(Scope::Kind kind, const std::string& name) {
    current_scope = current_scope->create_child_scope(kind, name);
}

void SymbolTable::exit_scope() {
    if (current_scope->get_parent()) {
        current_scope = current_scope->get_parent();
    }
}

bool SymbolTable::add_symbol(const std::string& name, SymbolKind kind, 
                            ast::TypePtr type, ast::DeclPtr declaration) {
    auto symbol = std::make_shared<Symbol>(name, kind, type, declaration);
    return current_scope->add_symbol(symbol);
}

SymbolPtr SymbolTable::lookup(const std::string& name) const {
    return current_scope->lookup(name);
}

SymbolPtr SymbolTable::lookup_in_scope(const std::string& name, const Scope* scope) const {
    if (!scope) scope = current_scope;
    return scope->lookup(name);
}

void SymbolTable::register_type(const std::string& name, ast::TypePtr type) {
    type_registry[name] = type;
}

ast::TypePtr SymbolTable::lookup_type(const std::string& name) const {
    auto it = type_registry.find(name);
    return it != type_registry.end() ? it->second : nullptr;
}

void SymbolTable::register_access_right(const std::string& class_name, const ast::AccessRight& access_right) {
    access_rights[class_name][access_right.name] = access_right;
}

std::optional<ast::AccessRight> SymbolTable::lookup_access_right(const std::string& class_name, 
                                                                const std::string& right_name) const {
    auto class_it = access_rights.find(class_name);
    if (class_it == access_rights.end()) {
        return std::nullopt;
    }
    
    auto right_it = class_it->second.find(right_name);
    if (right_it == class_it->second.end()) {
        return std::nullopt;
    }
    
    return right_it->second;
}

size_t SymbolTable::total_symbols() const {
    std::vector<SymbolPtr> symbols;
    collect_symbols_recursive(global_scope.get(), symbols);
    return symbols.size();
}

void SymbolTable::dump() const {
    std::cout << "=== Symbol Table ===\n";
    std::cout << "Total symbols: " << total_symbols() << "\n\n";
    global_scope->dump(0);
    
    if (!type_registry.empty()) {
        std::cout << "\n=== Type Registry ===\n";
        for (const auto& [name, type] : type_registry) {
            std::cout << "  " << name << " -> " << type->to_string() << "\n";
        }
    }
    
    if (!access_rights.empty()) {
        std::cout << "\n=== Access Rights ===\n";
        for (const auto& [class_name, rights] : access_rights) {
            std::cout << "  " << class_name << ":\n";
            for (const auto& [right_name, right] : rights) {
                std::cout << "    " << (right.is_runtime ? "runtime " : "") 
                         << "exposes " << right_name << " { ";
                for (const auto& field : right.granted_fields) {
                    std::cout << field << " ";
                }
                std::cout << "}\n";
            }
        }
    }
}

std::vector<SymbolPtr> SymbolTable::find_symbols_by_kind(SymbolKind kind) const {
    std::vector<SymbolPtr> result;
    collect_symbols_recursive(global_scope.get(), result, kind);
    return result;
}

std::vector<SymbolPtr> SymbolTable::find_symbols_in_scope(const Scope* scope, SymbolKind kind) const {
    std::vector<SymbolPtr> result;
    collect_symbols_recursive(scope, result, kind);
    return result;
}

void SymbolTable::collect_symbols_recursive(const Scope* scope, std::vector<SymbolPtr>& result,
                                           std::optional<SymbolKind> kind) const {
    for (const auto& [name, symbol] : scope->get_symbols()) {
        if (!kind || symbol->get_kind() == *kind) {
            result.push_back(symbol);
        }
    }
    
    for (const auto& child : scope->get_children()) {
        collect_symbols_recursive(child.get(), result, kind);
    }
}

// ============================================================================
// SymbolTableBuilder implementation
// ============================================================================

void SymbolTableBuilder::process_variable_declaration(const ast::VarDecl& decl) {
    symbol_table.add_symbol(decl.get_name(), SymbolKind::Variable, 
                           decl.get_type(), nullptr);
}

void SymbolTableBuilder::process_function_declaration(const ast::FunctionDecl& decl) {
    symbol_table.add_symbol(decl.get_name(), SymbolKind::Function, 
                           decl.get_return_type(), nullptr);
}

void SymbolTableBuilder::process_class_declaration(const ast::ClassDecl& decl) {
    // Add class symbol
    symbol_table.add_symbol(decl.get_name(), SymbolKind::Class, nullptr, nullptr);
    
    // Register access rights
    for (const auto& access_right : decl.get_access_rights()) {
        symbol_table.register_access_right(decl.get_name(), access_right);
    }
}

void SymbolTableBuilder::process_struct_declaration(const ast::StructDecl& decl) {
    symbol_table.add_symbol(decl.get_name(), SymbolKind::Struct, nullptr, nullptr);
}

void SymbolTableBuilder::process_union_declaration(const ast::UnionDecl& decl) {
    symbol_table.add_symbol(decl.get_name(), SymbolKind::Union, nullptr, nullptr);
}

void SymbolTableBuilder::process_interface_declaration(const ast::InterfaceDecl& decl) {
    symbol_table.add_symbol(decl.get_name(), SymbolKind::Interface, nullptr, nullptr);
}

} // namespace cprime::v2