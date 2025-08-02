#pragma once

#include "ast.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace cprime {

class CodeGenerator {
public:
    CodeGenerator();
    void generate(const Program& program);
    void write_ir_to_file(const std::string& filename);
    
private:
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module;
    llvm::IRBuilder<> builder;
    
    // Code generation methods
    void generate_function(const Function& func);
    void generate_block(const Block& block);
    void generate_statement(const Statement& stmt);
    void generate_variable_declaration(const VariableDeclaration& var_decl);
    void generate_assignment(const Assignment& assignment);
    void generate_function_call(const FunctionCall& call);
    void generate_if_statement(const IfStatement& if_stmt);
    void generate_while_loop(const WhileLoop& while_loop);
    void generate_for_loop(const ForLoop& for_loop);
    
    // Expression generation
    llvm::Value* generate_expression(const Expression& expr);
    llvm::Value* generate_binary_expression(const BinaryExpression& expr);
    llvm::Value* generate_boolean_literal(const BooleanLiteral& lit);
    llvm::Value* generate_number_literal(const NumberLiteral& lit);
    llvm::Value* generate_string_literal(const StringLiteral& lit);
    llvm::Value* generate_variable_reference(const VariableReference& var_ref);
    llvm::Value* generate_range_expression(const RangeExpression& range);
    
    // Helper methods
    llvm::Function* get_or_declare_printf();
    llvm::Type* get_llvm_type(Type type);
    
    // Print function helpers
    std::string process_format_string(const std::string& format, const std::vector<std::unique_ptr<Expression>>& args);
    std::string get_format_specifier(const Expression& expr);
    int count_placeholders(const std::string& format);
    
    // Variable management
    std::unordered_map<std::string, llvm::AllocaInst*> variables;
};

} // namespace cprime