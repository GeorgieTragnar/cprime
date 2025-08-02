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
    AUTO  // For type deduction
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

// Function definition: fn main() { ... }
struct Function : ASTNode {
    std::string name;
    std::unique_ptr<Block> body;
    
    Function(const std::string& name) : name(name) {}
};

// Root node representing the entire program
struct Program : ASTNode {
    std::vector<std::unique_ptr<Function>> functions;
};

} // namespace cprime