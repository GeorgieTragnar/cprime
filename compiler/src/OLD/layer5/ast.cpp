#include "ast.h"
#include <sstream>

namespace cprime::ast {

// ============================================================================
// Base AST Node implementations
// ============================================================================

std::string ASTNode::to_string() const {
    return "ASTNode(line=" + std::to_string(location.line) + ", col=" + std::to_string(location.column) + ")";
}

// ============================================================================
// Type implementations
// ============================================================================

void Type::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string Type::to_string() const {
    return "Type(" + name + ")";
}

// ============================================================================
// Expression implementations
// ============================================================================

void IdentifierExpr::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string IdentifierExpr::to_string() const {
    return "IdentifierExpr(" + name + ")";
}

void LiteralExpr::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string LiteralExpr::to_string() const {
    return "LiteralExpr(value)";
}

void BinaryExpr::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string BinaryExpr::to_string() const {
    return "BinaryExpr(op)";
}

void UnaryExpr::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string UnaryExpr::to_string() const {
    return "UnaryExpr(op)";
}

void CallExpr::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string CallExpr::to_string() const {
    return "CallExpr()";
}

void MemberExpr::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string MemberExpr::to_string() const {
    return "MemberExpr(" + member + ")";
}

// ============================================================================
// Statement implementations
// ============================================================================

void ExprStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string ExprStatement::to_string() const {
    return "ExprStatement()";
}

void BlockStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string BlockStatement::to_string() const {
    return "BlockStatement(" + std::to_string(statements.size()) + " stmts)";
}

void IfStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string IfStatement::to_string() const {
    return "IfStatement()";
}

void WhileStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string WhileStatement::to_string() const {
    return "WhileStatement()";
}

void ForStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string ForStatement::to_string() const {
    return "ForStatement()";
}

void ReturnStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string ReturnStatement::to_string() const {
    return "ReturnStatement()";
}

void DeferStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string DeferStatement::to_string() const {
    return "DeferStatement()";
}

// ============================================================================
// Declaration implementations
// ============================================================================

void VarDecl::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string VarDecl::to_string() const {
    return "VarDecl(" + name + ")";
}

void FunctionDecl::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string FunctionDecl::to_string() const {
    return "FunctionDecl(" + name + ")";
}

void ClassDecl::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string ClassDecl::to_string() const {
    std::stringstream ss;
    ss << "ClassDecl(" << name << ", " << members.size() << " members, " << access_rights.size() << " access rights)";
    return ss.str();
}

void StructDecl::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string StructDecl::to_string() const {
    return "StructDecl(" + name + ")";
}

void UnionDecl::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string UnionDecl::to_string() const {
    return "UnionDecl(" + name + ")";
}

void InterfaceDecl::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string InterfaceDecl::to_string() const {
    return "InterfaceDecl(" + name + ")";
}

// ============================================================================
// CompilationUnit implementation
// ============================================================================

void CompilationUnit::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

std::string CompilationUnit::to_string() const {
    std::stringstream ss;
    ss << "CompilationUnit(" << declarations.size() << " declarations)\n";
    for (const auto& decl : declarations) {
        ss << "  - " << decl->to_string() << "\n";
    }
    return ss.str();
}

} // namespace cprime::ast