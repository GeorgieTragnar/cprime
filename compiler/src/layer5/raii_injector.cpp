#include "raii_injector.h"
#include <algorithm>
#include <iostream>

namespace cprime {

// ============================================================================
// RAIIInjector Implementation
// ============================================================================

RAIIInjector::RAIIInjector(SymbolTable& symbol_table)
    : symbol_table(symbol_table), current_scope(nullptr) {
}

std::shared_ptr<ast::CompilationUnit> RAIIInjector::process(std::shared_ptr<ast::CompilationUnit> unit) {
    // Process the compilation unit and inject RAII cleanup
    unit->accept(*this);
    
    // Return the original unit - we modify statements in place
    return unit;
}

// ============================================================================
// Scope Management
// ============================================================================

void RAIIInjector::enter_scope() {
    scope_stack.push(std::vector<ScopedObject>());
    current_scope = &scope_stack.top();
}

void RAIIInjector::exit_scope() {
    if (!scope_stack.empty()) {
        scope_stack.pop();
    }
    current_scope = scope_stack.empty() ? nullptr : &scope_stack.top();
}

// ============================================================================
// Object Tracking
// ============================================================================

void RAIIInjector::track_stack_object(const std::string& name, ast::TypePtr type, const ast::SourceLocation& location) {
    if (current_scope && is_stack_allocated(type)) {
        current_scope->emplace_back(name, type, location);
    }
}

bool RAIIInjector::is_stack_allocated(ast::TypePtr type) const {
    if (!type) return false;
    
    // Simple heuristic: assume non-pointer, non-reference types are stack allocated
    return type->get_kind() != ast::Type::Kind::Pointer && 
           type->get_kind() != ast::Type::Kind::Reference;
}

bool RAIIInjector::has_destructor(const std::string& type_name) const {
    // For now, assume all user-defined types have destructors
    // TODO: Actually check the symbol table for destructor existence
    return !type_name.empty() && 
           type_name != "int" && type_name != "bool" && type_name != "float" && 
           type_name != "double" && type_name != "char" && type_name != "void";
}

// ============================================================================
// Destructor Injection
// ============================================================================

ast::StmtList RAIIInjector::inject_destructors_for_scope() {
    ast::StmtList destructors;
    
    if (!current_scope) return destructors;
    
    // Create destructor calls in reverse order (LIFO)
    for (auto it = current_scope->rbegin(); it != current_scope->rend(); ++it) {
        const auto& obj = *it;
        
        if (has_destructor(obj.type->get_name())) {
            auto destructor_call = create_destructor_call(obj);
            if (destructor_call) {
                destructors.push_back(destructor_call);
            }
        }
    }
    
    return destructors;
}

ast::StmtPtr RAIIInjector::create_destructor_call(const ScopedObject& obj) {
    // Create a destructor call: obj.~TypeName();
    
    // Create identifier for the object
    auto obj_expr = std::make_shared<ast::IdentifierExpr>(obj.name, obj.location);
    
    // Create destructor name (~TypeName)
    std::string destructor_name = "~" + obj.type->get_name();
    
    // Create member access: obj.~TypeName
    auto member_expr = std::make_shared<ast::MemberExpr>(
        obj_expr, destructor_name, false, obj.location
    );
    
    // Create function call: obj.~TypeName()
    ast::ExprList empty_args;
    auto call_expr = std::make_shared<ast::CallExpr>(
        member_expr, empty_args, obj.location
    );
    
    // Wrap in expression statement
    return std::make_shared<ast::ExprStatement>(call_expr, obj.location);
}

// ============================================================================
// AST Transformation Helpers
// ============================================================================

ast::StmtPtr RAIIInjector::transform_statement(ast::StmtPtr stmt) {
    if (!stmt) return stmt;
    
    // Transform different statement types
    if (auto block = std::dynamic_pointer_cast<ast::BlockStatement>(stmt)) {
        return transform_block_statement(*block);
    } else if (auto if_stmt = std::dynamic_pointer_cast<ast::IfStatement>(stmt)) {
        return transform_if_statement(*if_stmt);
    } else if (auto while_stmt = std::dynamic_pointer_cast<ast::WhileStatement>(stmt)) {
        return transform_while_statement(*while_stmt);
    } else if (auto for_stmt = std::dynamic_pointer_cast<ast::ForStatement>(stmt)) {
        return transform_for_statement(*for_stmt);
    }
    
    return stmt;
}

ast::StmtPtr RAIIInjector::transform_block_statement(const ast::BlockStatement& block) {
    enter_scope();
    
    ast::StmtList new_statements;
    
    // Process each statement in the block
    for (const auto& stmt : block.get_statements()) {
        // Track variable declarations
        if (auto var_decl = std::dynamic_pointer_cast<ast::VarDecl>(stmt)) {
            track_stack_object(var_decl->get_name(), var_decl->get_type(), var_decl->get_location());
        }
        
        // Transform and add the statement
        auto transformed = transform_statement(stmt);
        new_statements.push_back(transformed);
    }
    
    // Inject destructors at the end of the block
    auto destructors = inject_destructors_for_scope();
    for (const auto& destructor : destructors) {
        new_statements.push_back(destructor);
    }
    
    exit_scope();
    
    return std::make_shared<ast::BlockStatement>(new_statements, block.get_location());
}

ast::StmtPtr RAIIInjector::transform_if_statement(const ast::IfStatement& if_stmt) {
    // Transform then and else branches
    auto then_stmt = transform_statement(if_stmt.get_then_statement());
    auto else_stmt = if_stmt.get_else_statement() ? 
                     transform_statement(if_stmt.get_else_statement()) : nullptr;
    
    return std::make_shared<ast::IfStatement>(
        if_stmt.get_condition(), then_stmt, else_stmt, if_stmt.get_location()
    );
}

ast::StmtPtr RAIIInjector::transform_while_statement(const ast::WhileStatement& while_stmt) {
    auto body = transform_statement(while_stmt.get_body());
    
    return std::make_shared<ast::WhileStatement>(
        while_stmt.get_condition(), body, while_stmt.get_location()
    );
}

ast::StmtPtr RAIIInjector::transform_for_statement(const ast::ForStatement& for_stmt) {
    // For loops create their own scope
    enter_scope();
    
    // Track init statement if it's a variable declaration
    if (auto var_decl = std::dynamic_pointer_cast<ast::VarDecl>(for_stmt.get_init())) {
        track_stack_object(var_decl->get_name(), var_decl->get_type(), var_decl->get_location());
    }
    
    // Transform the body
    auto body = transform_statement(for_stmt.get_body());
    
    // Inject destructors for the for loop scope
    auto destructors = inject_destructors_for_scope();
    if (!destructors.empty()) {
        // Wrap body and destructors in a new block
        ast::StmtList body_with_cleanup;
        body_with_cleanup.push_back(body);
        for (const auto& destructor : destructors) {
            body_with_cleanup.push_back(destructor);
        }
        body = std::make_shared<ast::BlockStatement>(
            body_with_cleanup, 
            combine_locations(body->get_location(), destructors.back()->get_location())
        );
    }
    
    exit_scope();
    
    return std::make_shared<ast::ForStatement>(
        for_stmt.get_init(), for_stmt.get_condition(), 
        for_stmt.get_update(), body, for_stmt.get_location()
    );
}

ast::StmtPtr RAIIInjector::transform_function_body(ast::StmtPtr body) {
    // Function bodies create their own scope
    return transform_statement(body);
}

// ============================================================================
// Visitor Implementation (mostly pass-through)
// ============================================================================

void RAIIInjector::visit(const ast::IdentifierExpr& node) {
    // Pass-through - no transformation needed for expressions
}

void RAIIInjector::visit(const ast::LiteralExpr& node) {
    // Pass-through
}

void RAIIInjector::visit(const ast::BinaryExpr& node) {
    // Pass-through
}

void RAIIInjector::visit(const ast::UnaryExpr& node) {
    // Pass-through
}

void RAIIInjector::visit(const ast::CallExpr& node) {
    // Pass-through
}

void RAIIInjector::visit(const ast::MemberExpr& node) {
    // Pass-through
}

void RAIIInjector::visit(const ast::ExprStatement& node) {
    // Pass-through for now - expressions don't introduce new scopes
}

void RAIIInjector::visit(const ast::BlockStatement& node) {
    // This should be handled by transform_block_statement
    // Called from the transformation process
}

void RAIIInjector::visit(const ast::IfStatement& node) {
    // This should be handled by transform_if_statement
}

void RAIIInjector::visit(const ast::WhileStatement& node) {
    // This should be handled by transform_while_statement  
}

void RAIIInjector::visit(const ast::ForStatement& node) {
    // This should be handled by transform_for_statement
}

void RAIIInjector::visit(const ast::ReturnStatement& node) {
    // TODO: Inject destructors before return
    // For now, pass-through
}

void RAIIInjector::visit(const ast::DeferStatement& node) {
    // Pass-through - defer is handled elsewhere
}

void RAIIInjector::visit(const ast::VarDecl& node) {
    // Variable declarations are tracked during block processing
}

void RAIIInjector::visit(const ast::FunctionDecl& node) {
    // Transform function body if it exists
    if (node.get_body()) {
        auto transformed_body = transform_function_body(node.get_body());
        // Note: We can't modify the const node here
        // In practice, we'd need a mutable AST or return transformed nodes
    }
}

void RAIIInjector::visit(const ast::ClassDecl& node) {
    // Process class members - method bodies need RAII injection
    for (const auto& member : node.get_members()) {
        if (auto func = std::dynamic_pointer_cast<ast::FunctionDecl>(member)) {
            func->accept(*this);
        }
    }
}

void RAIIInjector::visit(const ast::StructDecl& node) {
    // Pass-through for now
}

void RAIIInjector::visit(const ast::UnionDecl& node) {
    // Pass-through for now
}

void RAIIInjector::visit(const ast::InterfaceDecl& node) {
    // Pass-through for now
}

void RAIIInjector::visit(const ast::CompilationUnit& node) {
    // Process all top-level declarations
    for (const auto& decl : node.get_declarations()) {
        decl->accept(*this);
    }
}

void RAIIInjector::visit(const ast::Type& node) {
    // Pass-through
}

// ============================================================================
// Helper Methods
// ============================================================================

ast::SourceLocation RAIIInjector::combine_locations(const ast::SourceLocation& start, const ast::SourceLocation& end) const {
    return ast::SourceLocation(start.line, start.column, start.start_pos, end.end_pos);
}

} // namespace cprime