#pragma once

#include "../layer3/ast.h"
#include "../layer3/symbol_table.h"
#include <memory>
#include <vector>
#include <stack>

namespace cprime {

/**
 * RAII Injector - Layer 4 of the compiler pipeline.
 * Injects automatic destructor calls at scope boundaries for RAII cleanup.
 * 
 * This class traverses the AST and automatically inserts destructor calls
 * for stack-allocated objects when they go out of scope, following LIFO
 * (Last In, First Out) destruction order.
 */
class RAIIInjector : public ast::ASTVisitor {
public:
    explicit RAIIInjector(SymbolTable& symbol_table);
    
    /**
     * Process the AST and inject RAII cleanup code.
     * Returns a new AST with destructor calls inserted.
     */
    std::shared_ptr<ast::CompilationUnit> process(std::shared_ptr<ast::CompilationUnit> unit);
    
    // Visitor pattern implementation
    void visit(const ast::IdentifierExpr& node) override;
    void visit(const ast::LiteralExpr& node) override;
    void visit(const ast::BinaryExpr& node) override;
    void visit(const ast::UnaryExpr& node) override;
    void visit(const ast::CallExpr& node) override;
    void visit(const ast::MemberExpr& node) override;
    
    void visit(const ast::ExprStatement& node) override;
    void visit(const ast::BlockStatement& node) override;
    void visit(const ast::IfStatement& node) override;
    void visit(const ast::WhileStatement& node) override;
    void visit(const ast::ForStatement& node) override;
    void visit(const ast::ReturnStatement& node) override;
    void visit(const ast::DeferStatement& node) override;
    
    void visit(const ast::VarDecl& node) override;
    void visit(const ast::FunctionDecl& node) override;
    void visit(const ast::ClassDecl& node) override;
    void visit(const ast::StructDecl& node) override;
    void visit(const ast::UnionDecl& node) override;
    void visit(const ast::InterfaceDecl& node) override;
    
    void visit(const ast::CompilationUnit& node) override;
    void visit(const ast::Type& node) override;

private:
    SymbolTable& symbol_table;
    
    /**
     * Tracks objects that need RAII cleanup in the current scope.
     */
    struct ScopedObject {
        std::string name;
        ast::TypePtr type;
        ast::SourceLocation location;
        
        ScopedObject(const std::string& name, ast::TypePtr type, const ast::SourceLocation& loc)
            : name(name), type(type), location(loc) {}
    };
    
    /**
     * Stack of scopes, each containing objects that need cleanup.
     */
    std::stack<std::vector<ScopedObject>> scope_stack;
    
    /**
     * Current scope being processed.
     */
    std::vector<ScopedObject>* current_scope;
    
    /**
     * Collected statements for the current processing.
     */
    ast::StmtList current_statements;
    
    // Scope management
    void enter_scope();
    void exit_scope();
    
    // Object tracking
    void track_stack_object(const std::string& name, ast::TypePtr type, const ast::SourceLocation& location);
    bool is_stack_allocated(ast::TypePtr type) const;
    bool has_destructor(const std::string& type_name) const;
    
    // Destructor injection
    ast::StmtList inject_destructors_for_scope();
    ast::StmtPtr create_destructor_call(const ScopedObject& obj);
    
    // AST transformation helpers
    ast::StmtPtr transform_statement(ast::StmtPtr stmt);
    ast::StmtPtr transform_block_statement(const ast::BlockStatement& block);
    ast::StmtPtr transform_if_statement(const ast::IfStatement& if_stmt);
    ast::StmtPtr transform_while_statement(const ast::WhileStatement& while_stmt);
    ast::StmtPtr transform_for_statement(const ast::ForStatement& for_stmt);
    ast::StmtPtr transform_function_body(ast::StmtPtr body);
    
    // Helper methods
    ast::SourceLocation combine_locations(const ast::SourceLocation& start, const ast::SourceLocation& end) const;
};

} // namespace cprime