#pragma once

#include <string>
#include <vector>
#include <memory>

namespace cprime {

// Base class for all AST nodes
struct ASTNode {
    virtual ~ASTNode() = default;
};

// Function call: print("Hello")
struct FunctionCall : ASTNode {
    std::string name;
    std::vector<std::string> args;  // Just string literals for now
    
    FunctionCall(const std::string& name) : name(name) {}
};

// Function definition: fn main() { ... }
struct Function : ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> body;
    
    Function(const std::string& name) : name(name) {}
};

// Root node representing the entire program
struct Program : ASTNode {
    std::vector<std::unique_ptr<Function>> functions;
};

} // namespace cprime