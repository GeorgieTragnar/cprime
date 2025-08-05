#pragma once

#include "ast.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <optional>

namespace cprime {

/**
 * Symbol kinds in the symbol table.
 */
enum class SymbolKind {
    Variable,
    Function,
    Class,
    Struct,
    Union,
    Interface,
    Type,
    Namespace,
    Parameter,
    Field,
    Method,
    AccessRight,
    Unknown
};

/**
 * Symbol visibility.
 */
enum class Visibility {
    Public,
    Private,
    Protected,
    Internal
};

/**
 * Symbol information stored in the symbol table.
 */
class Symbol {
public:
    Symbol(const std::string& name, SymbolKind kind, ast::TypePtr type, ast::DeclPtr declaration)
        : name(name), kind(kind), type(type), declaration(declaration),
          visibility(Visibility::Private), defined(false) {}
    
    // Basic properties
    const std::string& get_name() const { return name; }
    SymbolKind get_kind() const { return kind; }
    ast::TypePtr get_type() const { return type; }
    ast::DeclPtr get_declaration() const { return declaration; }
    
    // Visibility and definition status
    Visibility get_visibility() const { return visibility; }
    void set_visibility(Visibility vis) { visibility = vis; }
    bool is_symbol_defined() const { return defined; }
    void set_defined(bool def) { defined = def; }
    
    // Additional metadata
    void set_attribute(const std::string& key, const std::string& value) {
        attributes[key] = value;
    }
    
    std::optional<std::string> get_attribute(const std::string& key) const {
        auto it = attributes.find(key);
        return it != attributes.end() ? std::optional(it->second) : std::nullopt;
    }
    
    // Debug representation
    std::string to_string() const;
    
private:
    std::string name;
    SymbolKind kind;
    ast::TypePtr type;
    ast::DeclPtr declaration;
    Visibility visibility;
    bool defined;
    std::unordered_map<std::string, std::string> attributes;
};

using SymbolPtr = std::shared_ptr<Symbol>;

/**
 * Scope in the symbol table hierarchy.
 */
class Scope {
public:
    enum class Kind {
        Global,
        Namespace,
        Class,
        Function,
        Block,
        Interface,
        Union
    };
    
    Scope(Kind kind, const std::string& name = "", Scope* parent = nullptr)
        : kind(kind), name(name), parent(parent) {}
    
    // Symbol management
    bool add_symbol(SymbolPtr symbol);
    SymbolPtr lookup_local(const std::string& name) const;
    SymbolPtr lookup(const std::string& name) const; // Searches parent scopes too
    
    // Scope hierarchy
    Scope* get_parent() const { return parent; }
    const std::vector<std::unique_ptr<Scope>>& get_children() const { return children; }
    Scope* create_child_scope(Kind kind, const std::string& name = "");
    
    // Scope properties
    Kind get_kind() const { return kind; }
    const std::string& get_name() const { return name; }
    std::string get_qualified_name() const;
    
    // Symbol iteration
    const std::unordered_map<std::string, SymbolPtr>& get_symbols() const { return symbols; }
    
    // Debug
    void dump(int indent = 0) const;
    
private:
    Kind kind;
    std::string name;
    Scope* parent;
    std::vector<std::unique_ptr<Scope>> children;
    std::unordered_map<std::string, SymbolPtr> symbols;
};

/**
 * Symbol table for the entire compilation unit.
 */
class SymbolTable {
public:
    SymbolTable();
    
    // Scope management
    Scope* get_global_scope() { return global_scope.get(); }
    const Scope* get_global_scope() const { return global_scope.get(); }
    
    Scope* get_current_scope() { return current_scope; }
    const Scope* get_current_scope() const { return current_scope; }
    
    void enter_scope(Scope::Kind kind, const std::string& name = "");
    void exit_scope();
    
    // Symbol management in current scope
    bool add_symbol(const std::string& name, SymbolKind kind, 
                   ast::TypePtr type, ast::DeclPtr declaration);
    SymbolPtr lookup(const std::string& name) const;
    SymbolPtr lookup_in_scope(const std::string& name, const Scope* scope) const;
    
    // Type management
    void register_type(const std::string& name, ast::TypePtr type);
    ast::TypePtr lookup_type(const std::string& name) const;
    
    // Access rights management
    void register_access_right(const std::string& class_name, const ast::AccessRight& access_right);
    std::optional<ast::AccessRight> lookup_access_right(const std::string& class_name, 
                                                        const std::string& right_name) const;
    
    // Statistics and debugging
    size_t total_symbols() const;
    void dump() const;
    
    // Symbol resolution helpers
    std::vector<SymbolPtr> find_symbols_by_kind(SymbolKind kind) const;
    std::vector<SymbolPtr> find_symbols_in_scope(const Scope* scope, SymbolKind kind) const;
    
private:
    std::unique_ptr<Scope> global_scope;
    Scope* current_scope;
    
    // Type registry
    std::unordered_map<std::string, ast::TypePtr> type_registry;
    
    // Access rights registry (class_name -> access_right_name -> AccessRight)
    std::unordered_map<std::string, std::unordered_map<std::string, ast::AccessRight>> access_rights;
    
    // Helper methods
    void collect_symbols_recursive(const Scope* scope, std::vector<SymbolPtr>& result,
                                  std::optional<SymbolKind> kind = std::nullopt) const;
};

/**
 * Symbol table builder - populates symbol table during AST construction.
 */
class SymbolTableBuilder {
public:
    explicit SymbolTableBuilder(SymbolTable& table) : symbol_table(table) {}
    
    // Declaration processing
    void process_variable_declaration(const ast::VarDecl& decl);
    void process_function_declaration(const ast::FunctionDecl& decl);
    void process_class_declaration(const ast::ClassDecl& decl);
    void process_struct_declaration(const ast::StructDecl& decl);
    void process_union_declaration(const ast::UnionDecl& decl);
    void process_interface_declaration(const ast::InterfaceDecl& decl);
    
    // Scope management helpers
    class ScopeGuard {
    public:
        ScopeGuard(SymbolTable& table, Scope::Kind kind, const std::string& name = "")
            : table(table) {
            table.enter_scope(kind, name);
        }
        ~ScopeGuard() {
            table.exit_scope();
        }
    private:
        SymbolTable& table;
    };
    
private:
    SymbolTable& symbol_table;
    
    // Helper methods
    void process_parameters(const std::vector<ast::Parameter>& params);
    void process_class_members(const ast::DeclList& members);
    void process_access_rights(const std::string& class_name, 
                              const std::vector<ast::AccessRight>& rights);
};

/**
 * Symbol resolver - resolves symbol references after symbol table is built.
 */
class SymbolResolver {
public:
    explicit SymbolResolver(const SymbolTable& table) : symbol_table(table) {}
    
    // Symbol resolution
    SymbolPtr resolve_identifier(const std::string& name, const Scope* scope = nullptr) const;
    SymbolPtr resolve_member_access(const std::string& object_type, const std::string& member) const;
    ast::TypePtr resolve_type(const std::string& type_name) const;
    
    // Access rights validation
    bool validate_access_right(const std::string& class_name, const std::string& right_name,
                              const std::string& field_name) const;
    
    // Type checking helpers
    bool is_compatible_type(ast::TypePtr expected, ast::TypePtr actual) const;
    bool is_numeric_type(ast::TypePtr type) const;
    bool is_pointer_type(ast::TypePtr type) const;
    
private:
    const SymbolTable& symbol_table;
    
    // Helper methods
    SymbolPtr resolve_qualified_name(const std::string& qualified_name) const;
    std::vector<std::string> split_qualified_name(const std::string& name) const;
};

} // namespace cprime