#include "codegen.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <stdexcept>

namespace cprime {

CodeGenerator::CodeGenerator() : builder(context) {
    module = std::make_unique<llvm::Module>("cprime_module", context);
}

void CodeGenerator::generate(const Program& program) {
    // First, declare printf
    get_or_declare_printf();
    
    // Generate all functions
    for (const auto& func : program.functions) {
        generate_function(*func);
    }
    
    // Verify the module
    std::string error_str;
    llvm::raw_string_ostream error_stream(error_str);
    if (llvm::verifyModule(*module, &error_stream)) {
        throw std::runtime_error("Module verification failed: " + error_str);
    }
}

void CodeGenerator::generate_function(const Function& func) {
    // Create function type: int main() or void func()
    llvm::Type* return_type = (func.name == "main") ? 
        llvm::Type::getInt32Ty(context) : 
        llvm::Type::getVoidTy(context);
    
    auto func_type = llvm::FunctionType::get(return_type, false);
    
    auto llvm_func = llvm::Function::Create(
        func_type,
        llvm::Function::ExternalLinkage,
        func.name,
        *module
    );
    
    // Create entry block
    auto entry_block = llvm::BasicBlock::Create(context, "entry", llvm_func);
    builder.SetInsertPoint(entry_block);
    
    // Generate function body
    for (const auto& stmt : func.body) {
        if (auto call = dynamic_cast<FunctionCall*>(stmt.get())) {
            generate_function_call(*call);
        }
    }
    
    // Add return statement
    if (func.name == "main") {
        builder.CreateRet(llvm::ConstantInt::get(context, llvm::APInt(32, 0)));
    } else {
        builder.CreateRetVoid();
    }
}

void CodeGenerator::generate_function_call(const FunctionCall& call) {
    if (call.name == "print" && !call.args.empty()) {
        auto printf_func = get_or_declare_printf();
        
        // Create format string with newline
        std::string format_str = call.args[0] + "\n";
        auto str_constant = builder.CreateGlobalStringPtr(format_str);
        
        // Call printf
        builder.CreateCall(printf_func, {str_constant});
    } else {
        throw std::runtime_error("Unknown function: " + call.name);
    }
}

llvm::Function* CodeGenerator::get_or_declare_printf() {
    // Check if printf is already declared
    auto existing = module->getFunction("printf");
    if (existing) return existing;
    
    // Declare printf - in LLVM 18, use PointerType::get
    auto char_ptr_type = llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);
    std::vector<llvm::Type*> params = {char_ptr_type};
    auto printf_type = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context),  // returns int
        params,                           // takes char*
        true                              // is variadic
    );
    
    return llvm::Function::Create(
        printf_type,
        llvm::Function::ExternalLinkage,
        "printf",
        *module
    );
}

void CodeGenerator::write_ir_to_file(const std::string& filename) {
    std::error_code ec;
    llvm::raw_fd_ostream file(filename, ec, llvm::sys::fs::OF_None);
    
    if (ec) {
        throw std::runtime_error("Failed to open file: " + filename + " - " + ec.message());
    }
    
    module->print(file, nullptr);
}


} // namespace cprime