#pragma once

#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <optional>
#include <unordered_map>

namespace cprime::v2::ast {

// Forward declarations
class ASTVisitor;
struct SourceLocation;
class Type;
class Symbol;

/**
 * Source location information for AST nodes.
 * Preserves exact mapping to original tokens.
 */
struct SourceLocation {
    size_t line;
    size_t column;
    size_t start_pos;
    size_t end_pos;
    
    SourceLocation(size_t line = 0, size_t column = 0, size_t start = 0, size_t end = 0)
        : line(line), column(column), start_pos(start), end_pos(end) {}
};

/**
 * Base class for all AST nodes.
 * Immutable design - nodes are constructed once and never modified.
 */
class ASTNode {
public:
    explicit ASTNode(const SourceLocation& loc) : location(loc) {}
    virtual ~ASTNode() = default;
    
    // Visitor pattern for traversal
    virtual void accept(ASTVisitor& visitor) const = 0;
    
    // Source location for error reporting
    const SourceLocation& get_location() const { return location; }
    
    // Debug representation
    virtual std::string to_string() const = 0;
    
protected:
    SourceLocation location;
};

// Node pointer types
using ASTNodePtr = std::shared_ptr<ASTNode>;
using ASTNodeList = std::vector<ASTNodePtr>;

/**
 * Type representation in the AST.
 */
class Type : public ASTNode {
public:
    enum class Kind {
        Builtin,      // int, bool, float, etc.
        Pointer,      // *T
        Reference,    // &T
        Array,        // [T; N]
        Class,        // User-defined class
        Union,        // Union type
        Function,     // Function type
        Generic,      // Generic/template type
        Runtime,      // Runtime-parameterized type
        Unknown       // Unresolved type
    };
    
    Type(Kind kind, const std::string& name, const SourceLocation& loc)
        : ASTNode(loc), kind(kind), name(name) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    Kind get_kind() const { return kind; }
    const std::string& get_name() const { return name; }
    
private:
    Kind kind;
    std::string name;
};

using TypePtr = std::shared_ptr<Type>;

/**
 * Expression base class.
 */
class Expression : public ASTNode {
public:
    explicit Expression(const SourceLocation& loc) : ASTNode(loc) {}
    
    // Optional type information (filled during semantic analysis)
    void set_type(TypePtr type) { expr_type = type; }
    TypePtr get_type() const { return expr_type; }
    
protected:
    TypePtr expr_type;
};

using ExprPtr = std::shared_ptr<Expression>;
using ExprList = std::vector<ExprPtr>;

/**
 * Statement base class.
 */
class Statement : public ASTNode {
public:
    explicit Statement(const SourceLocation& loc) : ASTNode(loc) {}
};

using StmtPtr = std::shared_ptr<Statement>;
using StmtList = std::vector<StmtPtr>;

/**
 * Declaration base class.
 */
class Declaration : public Statement {
public:
    Declaration(const std::string& name, const SourceLocation& loc)
        : Statement(loc), name(name) {}
    
    const std::string& get_name() const { return name; }
    
protected:
    std::string name;
};

using DeclPtr = std::shared_ptr<Declaration>;
using DeclList = std::vector<DeclPtr>;

// ============================================================================
// Specific AST Node Types
// ============================================================================

/**
 * Identifier expression.
 */
class IdentifierExpr : public Expression {
public:
    IdentifierExpr(const std::string& name, const SourceLocation& loc)
        : Expression(loc), name(name) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    const std::string& get_name() const { return name; }
    
private:
    std::string name;
};

/**
 * Literal expression (numbers, strings, booleans).
 */
class LiteralExpr : public Expression {
public:
    using Value = std::variant<int64_t, double, bool, std::string>;
    
    LiteralExpr(const Value& value, const SourceLocation& loc)
        : Expression(loc), value(value) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    const Value& get_value() const { return value; }
    
private:
    Value value;
};

/**
 * Binary expression (a + b, a == b, etc.).
 */
class BinaryExpr : public Expression {
public:
    enum class Operator {
        Add, Sub, Mul, Div, Mod,
        Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual,
        LogicalAnd, LogicalOr,
        BitwiseAnd, BitwiseOr, BitwiseXor,
        LeftShift, RightShift,
        Assign, AddAssign, SubAssign, MulAssign, DivAssign,
        Arrow, Dot, Scope
    };
    
    BinaryExpr(ExprPtr left, Operator op, ExprPtr right, const SourceLocation& loc)
        : Expression(loc), left(left), op(op), right(right) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    ExprPtr get_left() const { return left; }
    Operator get_operator() const { return op; }
    ExprPtr get_right() const { return right; }
    
private:
    ExprPtr left;
    Operator op;
    ExprPtr right;
};

/**
 * Unary expression (!a, -a, &a, *a, etc.).
 */
class UnaryExpr : public Expression {
public:
    enum class Operator {
        LogicalNot, BitwiseNot, Negate,
        AddressOf, Dereference,
        PreIncrement, PreDecrement,
        PostIncrement, PostDecrement
    };
    
    UnaryExpr(Operator op, ExprPtr operand, const SourceLocation& loc)
        : Expression(loc), op(op), operand(operand) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    Operator get_operator() const { return op; }
    ExprPtr get_operand() const { return operand; }
    
private:
    Operator op;
    ExprPtr operand;
};

/**
 * Function call expression.
 */
class CallExpr : public Expression {
public:
    CallExpr(ExprPtr callee, ExprList arguments, const SourceLocation& loc)
        : Expression(loc), callee(callee), arguments(arguments) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    ExprPtr get_callee() const { return callee; }
    const ExprList& get_arguments() const { return arguments; }
    
private:
    ExprPtr callee;
    ExprList arguments;
};

/**
 * Member access expression (object.field or object->field).
 */
class MemberExpr : public Expression {
public:
    MemberExpr(ExprPtr object, const std::string& member, bool is_arrow, const SourceLocation& loc)
        : Expression(loc), object(object), member(member), is_arrow(is_arrow) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    ExprPtr get_object() const { return object; }
    const std::string& get_member() const { return member; }
    bool is_arrow_access() const { return is_arrow; }
    
private:
    ExprPtr object;
    std::string member;
    bool is_arrow;
};

// ============================================================================
// Statement Nodes
// ============================================================================

/**
 * Expression statement.
 */
class ExprStatement : public Statement {
public:
    ExprStatement(ExprPtr expr, const SourceLocation& loc)
        : Statement(loc), expr(expr) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    ExprPtr get_expression() const { return expr; }
    
private:
    ExprPtr expr;
};

/**
 * Block statement { ... }.
 */
class BlockStatement : public Statement {
public:
    BlockStatement(StmtList statements, const SourceLocation& loc)
        : Statement(loc), statements(statements) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    const StmtList& get_statements() const { return statements; }
    
private:
    StmtList statements;
};

/**
 * If statement.
 */
class IfStatement : public Statement {
public:
    IfStatement(ExprPtr condition, StmtPtr then_stmt, StmtPtr else_stmt, const SourceLocation& loc)
        : Statement(loc), condition(condition), then_stmt(then_stmt), else_stmt(else_stmt) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    ExprPtr get_condition() const { return condition; }
    StmtPtr get_then_statement() const { return then_stmt; }
    StmtPtr get_else_statement() const { return else_stmt; }
    
private:
    ExprPtr condition;
    StmtPtr then_stmt;
    StmtPtr else_stmt;
};

/**
 * While loop.
 */
class WhileStatement : public Statement {
public:
    WhileStatement(ExprPtr condition, StmtPtr body, const SourceLocation& loc)
        : Statement(loc), condition(condition), body(body) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    ExprPtr get_condition() const { return condition; }
    StmtPtr get_body() const { return body; }
    
private:
    ExprPtr condition;
    StmtPtr body;
};

/**
 * For loop.
 */
class ForStatement : public Statement {
public:
    ForStatement(StmtPtr init, ExprPtr condition, ExprPtr update, StmtPtr body, const SourceLocation& loc)
        : Statement(loc), init(init), condition(condition), update(update), body(body) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    StmtPtr get_init() const { return init; }
    ExprPtr get_condition() const { return condition; }
    ExprPtr get_update() const { return update; }
    StmtPtr get_body() const { return body; }
    
private:
    StmtPtr init;
    ExprPtr condition;
    ExprPtr update;
    StmtPtr body;
};

/**
 * Return statement.
 */
class ReturnStatement : public Statement {
public:
    ReturnStatement(ExprPtr value, const SourceLocation& loc)
        : Statement(loc), value(value) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    ExprPtr get_value() const { return value; }
    
private:
    ExprPtr value;
};

/**
 * Defer statement (RAII cleanup).
 */
class DeferStatement : public Statement {
public:
    DeferStatement(ExprPtr cleanup_expr, const SourceLocation& loc)
        : Statement(loc), cleanup_expr(cleanup_expr) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    ExprPtr get_cleanup_expression() const { return cleanup_expr; }
    
private:
    ExprPtr cleanup_expr;
};

// ============================================================================
// Declaration Nodes
// ============================================================================

/**
 * Variable declaration.
 */
class VarDecl : public Declaration {
public:
    VarDecl(const std::string& name, TypePtr type, ExprPtr init, bool is_const, const SourceLocation& loc)
        : Declaration(name, loc), type(type), init(init), is_const(is_const) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    TypePtr get_type() const { return type; }
    ExprPtr get_initializer() const { return init; }
    bool is_constant() const { return is_const; }
    
private:
    TypePtr type;
    ExprPtr init;
    bool is_const;
};

/**
 * Function parameter.
 */
struct Parameter {
    std::string name;
    TypePtr type;
    ExprPtr default_value;
    
    Parameter(const std::string& name, TypePtr type, ExprPtr default_val = nullptr)
        : name(name), type(type), default_value(default_val) {}
};

/**
 * Function declaration.
 */
class FunctionDecl : public Declaration {
public:
    FunctionDecl(const std::string& name, std::vector<Parameter> params, TypePtr return_type,
                 StmtPtr body, bool is_async, const SourceLocation& loc)
        : Declaration(name, loc), parameters(params), return_type(return_type),
          body(body), is_async(is_async) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    const std::vector<Parameter>& get_parameters() const { return parameters; }
    TypePtr get_return_type() const { return return_type; }
    StmtPtr get_body() const { return body; }
    bool is_coroutine() const { return is_async; }
    
private:
    std::vector<Parameter> parameters;
    TypePtr return_type;
    StmtPtr body;
    bool is_async;
};

/**
 * Access rights specification.
 */
struct AccessRight {
    std::string name;
    std::vector<std::string> granted_fields;
    bool is_runtime;
    
    AccessRight() : is_runtime(false) {}
    AccessRight(const std::string& name, const std::vector<std::string>& fields, bool runtime)
        : name(name), granted_fields(fields), is_runtime(runtime) {}
};

/**
 * Class declaration.
 */
class ClassDecl : public Declaration {
public:
    enum class Kind {
        Data,        // Regular data class
        Functional,  // Functional class (static methods only)
        Danger      // Unsafe class
    };
    
    ClassDecl(const std::string& name, Kind kind, DeclList members,
              std::vector<AccessRight> access_rights, const SourceLocation& loc)
        : Declaration(name, loc), kind(kind), members(members), access_rights(access_rights) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    Kind get_kind() const { return kind; }
    const DeclList& get_members() const { return members; }
    const std::vector<AccessRight>& get_access_rights() const { return access_rights; }
    
private:
    Kind kind;
    DeclList members;
    std::vector<AccessRight> access_rights;
};

/**
 * Struct declaration (C++ compatibility).
 */
class StructDecl : public Declaration {
public:
    StructDecl(const std::string& name, DeclList members, const SourceLocation& loc)
        : Declaration(name, loc), members(members) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    const DeclList& get_members() const { return members; }
    
private:
    DeclList members;
};

/**
 * Union variant.
 */
struct UnionVariant {
    std::string name;
    TypePtr type;
    
    UnionVariant(const std::string& name, TypePtr type = nullptr)
        : name(name), type(type) {}
};

/**
 * Union declaration.
 */
class UnionDecl : public Declaration {
public:
    UnionDecl(const std::string& name, std::vector<UnionVariant> variants,
              bool is_runtime, const SourceLocation& loc)
        : Declaration(name, loc), variants(variants), is_runtime(is_runtime) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    const std::vector<UnionVariant>& get_variants() const { return variants; }
    bool is_runtime_union() const { return is_runtime; }
    
private:
    std::vector<UnionVariant> variants;
    bool is_runtime;
};

/**
 * Interface declaration.
 */
class InterfaceDecl : public Declaration {
public:
    InterfaceDecl(const std::string& name, DeclList methods, const SourceLocation& loc)
        : Declaration(name, loc), methods(methods) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    const DeclList& get_methods() const { return methods; }
    
private:
    DeclList methods;
};

// ============================================================================
// Top-level AST
// ============================================================================

/**
 * Compilation unit (source file).
 */
class CompilationUnit : public ASTNode {
public:
    CompilationUnit(DeclList declarations, const SourceLocation& loc)
        : ASTNode(loc), declarations(declarations) {}
    
    void accept(ASTVisitor& visitor) const override;
    std::string to_string() const override;
    
    const DeclList& get_declarations() const { return declarations; }
    
private:
    DeclList declarations;
};

// ============================================================================
// Visitor Interface
// ============================================================================

/**
 * Visitor pattern for AST traversal.
 */
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    // Expression visitors
    virtual void visit(const IdentifierExpr& node) = 0;
    virtual void visit(const LiteralExpr& node) = 0;
    virtual void visit(const BinaryExpr& node) = 0;
    virtual void visit(const UnaryExpr& node) = 0;
    virtual void visit(const CallExpr& node) = 0;
    virtual void visit(const MemberExpr& node) = 0;
    
    // Statement visitors
    virtual void visit(const ExprStatement& node) = 0;
    virtual void visit(const BlockStatement& node) = 0;
    virtual void visit(const IfStatement& node) = 0;
    virtual void visit(const WhileStatement& node) = 0;
    virtual void visit(const ForStatement& node) = 0;
    virtual void visit(const ReturnStatement& node) = 0;
    virtual void visit(const DeferStatement& node) = 0;
    
    // Declaration visitors
    virtual void visit(const VarDecl& node) = 0;
    virtual void visit(const FunctionDecl& node) = 0;
    virtual void visit(const ClassDecl& node) = 0;
    virtual void visit(const StructDecl& node) = 0;
    virtual void visit(const UnionDecl& node) = 0;
    virtual void visit(const InterfaceDecl& node) = 0;
    
    // Top-level visitor
    virtual void visit(const CompilationUnit& node) = 0;
    
    // Type visitor
    virtual void visit(const Type& node) = 0;
};

} // namespace cprime::v2::ast