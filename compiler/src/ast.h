#pragma once

#include <string>
#include <vector>
#include <memory>

namespace cprime {

// Type enumeration
enum class Type {
    INT,
    BOOL,
    VOID,
    AUTO,  // For type deduction
    CUSTOM // For user-defined classes
};

// Custom type information for user-defined classes
struct CustomType {
    std::string name;
    
    CustomType(const std::string& name) : name(name) {}
};

// Base class for all AST nodes
struct ASTNode {
    virtual ~ASTNode() = default;
};

// Base class for statements
struct Statement : ASTNode {
    virtual ~Statement() = default;
};

// Base class for expressions
struct Expression : ASTNode {
    virtual ~Expression() = default;
};

// Boolean literal: true, false
struct BooleanLiteral : Expression {
    bool value;
    
    BooleanLiteral(bool value) : value(value) {}
};

// Number literal: 42, 0, 123
struct NumberLiteral : Expression {
    int value;
    
    NumberLiteral(int value) : value(value) {}
};

// String literal: "Hello", "World"
struct StringLiteral : Expression {
    std::string value;
    
    StringLiteral(const std::string& value) : value(value) {}
};

// Binary expression: a < b, x == y, a + b, x * y
struct BinaryExpression : Expression {
    std::unique_ptr<Expression> left;
    std::string operator_token;  // "<", ">", "==", "!=", "<=", ">=", "+", "-", "*", "/", "%"
    std::unique_ptr<Expression> right;
    
    BinaryExpression(std::unique_ptr<Expression> left, 
                    const std::string& op, 
                    std::unique_ptr<Expression> right)
        : left(std::move(left)), operator_token(op), right(std::move(right)) {}
};

// Range expression: range(n)
struct RangeExpression : Expression {
    std::unique_ptr<Expression> limit;
    
    RangeExpression(std::unique_ptr<Expression> limit) : limit(std::move(limit)) {}
};

// Variable reference: x, y, result
struct VariableReference : Expression {
    std::string name;
    
    VariableReference(const std::string& name) : name(name) {}
};

// Variable declaration: auto x = 5; int y = 10;
struct VariableDeclaration : Statement {
    Type type;
    std::string name;
    std::unique_ptr<Expression> initializer;
    
    VariableDeclaration(Type type, const std::string& name, std::unique_ptr<Expression> initializer)
        : type(type), name(name), initializer(std::move(initializer)) {}
};

// Assignment: x = 5;
struct Assignment : Statement {
    std::string name;
    std::unique_ptr<Expression> value;
    
    Assignment(const std::string& name, std::unique_ptr<Expression> value)
        : name(name), value(std::move(value)) {}
};

// Function call: print("Hello"), print(x), print(x + y)
struct FunctionCall : Statement {
    std::string name;
    std::vector<std::unique_ptr<Expression>> args;  // Now supports expressions
    
    FunctionCall(const std::string& name) : name(name) {}
};

// Block statement: { statements... }
struct Block : Statement {
    std::vector<std::unique_ptr<Statement>> statements;
};

// If statement: if (condition) { ... } else { ... }
struct IfStatement : Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Block> then_block;
    std::unique_ptr<Block> else_block;  // optional
    
    IfStatement(std::unique_ptr<Expression> condition, 
               std::unique_ptr<Block> then_block,
               std::unique_ptr<Block> else_block = nullptr)
        : condition(std::move(condition)), 
          then_block(std::move(then_block)), 
          else_block(std::move(else_block)) {}
};

// While loop: while (condition) { ... }
struct WhileLoop : Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Block> body;
    
    WhileLoop(std::unique_ptr<Expression> condition, std::unique_ptr<Block> body)
        : condition(std::move(condition)), body(std::move(body)) {}
};

// For loop: for (var in range) { ... }
struct ForLoop : Statement {
    std::string variable;  // iterator variable name
    std::unique_ptr<Expression> iterable;  // range expression
    std::unique_ptr<Block> body;
    
    ForLoop(const std::string& variable, 
           std::unique_ptr<Expression> iterable, 
           std::unique_ptr<Block> body)
        : variable(variable), iterable(std::move(iterable)), body(std::move(body)) {}
};

// Function definition: int main() { ... }, void helper() { ... }
struct Function : ASTNode {
    Type return_type;
    std::string name;
    std::unique_ptr<Block> body;
    
    Function(Type return_type, const std::string& name) 
        : return_type(return_type), name(name) {}
};

// Field access expression: obj.field
struct FieldAccess : Expression {
    std::unique_ptr<Expression> object;
    std::string field_name;
    
    FieldAccess(std::unique_ptr<Expression> object, const std::string& field_name)
        : object(std::move(object)), field_name(field_name) {}
};

// Constructor types
enum class ConstructorType {
    DEFAULT,     // Class() = default;
    COPY,        // Class(const Class& other) = default;
    MOVE,        // Class(Class&& other) = default;
    DESTRUCTOR   // ~Class() = default;
};

// Field declaration: name: type
struct FieldDeclaration : ASTNode {
    std::string name;
    Type type;
    std::unique_ptr<CustomType> custom_type;  // Set when type is CUSTOM
    
    FieldDeclaration(const std::string& name, Type type) 
        : name(name), type(type) {}
    
    FieldDeclaration(const std::string& name, std::unique_ptr<CustomType> custom_type)
        : name(name), type(Type::CUSTOM), custom_type(std::move(custom_type)) {}
};

// Constructor declaration: Class() = default; or explicit definition
struct ConstructorDeclaration : ASTNode {
    ConstructorType type;
    bool is_default;
    bool is_explicit;
    std::unique_ptr<Block> body;  // nullptr for = default
    
    ConstructorDeclaration(ConstructorType type, bool is_default = true, bool is_explicit = false)
        : type(type), is_default(is_default), is_explicit(is_explicit) {}
};

// Class definition: class Name { fields... constructors... };
struct ClassDefinition : ASTNode {
    std::string name;
    std::vector<std::unique_ptr<FieldDeclaration>> fields;
    std::vector<std::unique_ptr<ConstructorDeclaration>> constructors;
    
    ClassDefinition(const std::string& name) : name(name) {}
};

// Root node representing the entire program
struct Program : ASTNode {
    std::vector<std::unique_ptr<Function>> functions;
    std::vector<std::unique_ptr<ClassDefinition>> classes;
};

} // namespace cprime