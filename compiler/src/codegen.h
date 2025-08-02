#pragma once

#include "ast.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <memory>
#include <string>

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
    void generate_function_call(const FunctionCall& call);
    
    // Helper methods
    llvm::Function* get_or_declare_printf();
};

} // namespace cprime