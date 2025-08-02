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
    generate_block(*func.body);
    
    // Add return statement
    if (func.name == "main") {
        builder.CreateRet(llvm::ConstantInt::get(context, llvm::APInt(32, 0)));
    } else {
        builder.CreateRetVoid();
    }
}

void CodeGenerator::generate_block(const Block& block) {
    for (const auto& stmt : block.statements) {
        generate_statement(*stmt);
    }
}

void CodeGenerator::generate_statement(const Statement& stmt) {
    if (auto call = dynamic_cast<const FunctionCall*>(&stmt)) {
        generate_function_call(*call);
    } else if (auto block = dynamic_cast<const Block*>(&stmt)) {
        generate_block(*block);
    } else if (auto if_stmt = dynamic_cast<const IfStatement*>(&stmt)) {
        generate_if_statement(*if_stmt);
    } else if (auto while_loop = dynamic_cast<const WhileLoop*>(&stmt)) {
        generate_while_loop(*while_loop);
    } else if (auto for_loop = dynamic_cast<const ForLoop*>(&stmt)) {
        generate_for_loop(*for_loop);
    } else {
        throw std::runtime_error("Unknown statement type in code generation");
    }
}

void CodeGenerator::generate_if_statement(const IfStatement& if_stmt) {
    // Generate condition
    llvm::Value* condition = generate_expression(*if_stmt.condition);
    
    // Get current function for creating basic blocks
    llvm::Function* current_func = builder.GetInsertBlock()->getParent();
    
    // Create basic blocks
    llvm::BasicBlock* then_block = llvm::BasicBlock::Create(context, "if.then", current_func);
    llvm::BasicBlock* else_block = if_stmt.else_block ? 
        llvm::BasicBlock::Create(context, "if.else", current_func) : nullptr;
    llvm::BasicBlock* merge_block = llvm::BasicBlock::Create(context, "if.end", current_func);
    
    // Conditional branch
    if (else_block) {
        builder.CreateCondBr(condition, then_block, else_block);
    } else {
        builder.CreateCondBr(condition, then_block, merge_block);
    }
    
    // Generate then block
    builder.SetInsertPoint(then_block);
    generate_block(*if_stmt.then_block);
    builder.CreateBr(merge_block);
    
    // Generate else block if it exists
    if (else_block) {
        builder.SetInsertPoint(else_block);
        generate_block(*if_stmt.else_block);
        builder.CreateBr(merge_block);
    }
    
    // Continue with merge block
    builder.SetInsertPoint(merge_block);
}

void CodeGenerator::generate_while_loop(const WhileLoop& while_loop) {
    llvm::Function* current_func = builder.GetInsertBlock()->getParent();
    
    // Create basic blocks
    llvm::BasicBlock* loop_header = llvm::BasicBlock::Create(context, "while.header", current_func);
    llvm::BasicBlock* loop_body = llvm::BasicBlock::Create(context, "while.body", current_func);
    llvm::BasicBlock* loop_exit = llvm::BasicBlock::Create(context, "while.exit", current_func);
    
    // Jump to loop header
    builder.CreateBr(loop_header);
    
    // Generate loop header (condition check)
    builder.SetInsertPoint(loop_header);
    llvm::Value* condition = generate_expression(*while_loop.condition);
    builder.CreateCondBr(condition, loop_body, loop_exit);
    
    // Generate loop body
    builder.SetInsertPoint(loop_body);
    generate_block(*while_loop.body);
    builder.CreateBr(loop_header);  // Jump back to header
    
    // Continue with exit block
    builder.SetInsertPoint(loop_exit);
}

void CodeGenerator::generate_for_loop(const ForLoop& for_loop) {
    // For simplicity, we'll implement for loops as while loops for now
    // For a proper implementation, we'd need variable scoping
    
    // For range-based loops, we simulate:
    // int i = 0;
    // while (i < limit) {
    //     // body
    //     i++;
    // }
    
    llvm::Function* current_func = builder.GetInsertBlock()->getParent();
    
    // For now, just generate the body a fixed number of times based on range
    if (auto range = dynamic_cast<const RangeExpression*>(for_loop.iterable.get())) {
        if (auto limit_literal = dynamic_cast<const NumberLiteral*>(range->limit.get())) {
            int iterations = limit_literal->value;
            
            for (int i = 0; i < iterations; i++) {
                generate_block(*for_loop.body);
            }
        } else {
            throw std::runtime_error("For loops with non-constant ranges not yet supported");
        }
    } else {
        throw std::runtime_error("For loops with non-range iterables not yet supported");
    }
}

llvm::Value* CodeGenerator::generate_expression(const Expression& expr) {
    if (auto binary = dynamic_cast<const BinaryExpression*>(&expr)) {
        return generate_binary_expression(*binary);
    } else if (auto boolean = dynamic_cast<const BooleanLiteral*>(&expr)) {
        return generate_boolean_literal(*boolean);
    } else if (auto number = dynamic_cast<const NumberLiteral*>(&expr)) {
        return generate_number_literal(*number);
    } else if (auto range = dynamic_cast<const RangeExpression*>(&expr)) {
        return generate_range_expression(*range);
    } else {
        throw std::runtime_error("Unknown expression type in code generation");
    }
}

llvm::Value* CodeGenerator::generate_binary_expression(const BinaryExpression& expr) {
    llvm::Value* left = generate_expression(*expr.left);
    llvm::Value* right = generate_expression(*expr.right);
    
    // For now, assume integer comparisons
    if (expr.operator_token == "<") {
        return builder.CreateICmpSLT(left, right, "cmptmp");
    } else if (expr.operator_token == ">") {
        return builder.CreateICmpSGT(left, right, "cmptmp");
    } else if (expr.operator_token == "<=") {
        return builder.CreateICmpSLE(left, right, "cmptmp");
    } else if (expr.operator_token == ">=") {
        return builder.CreateICmpSGE(left, right, "cmptmp");
    } else if (expr.operator_token == "==") {
        return builder.CreateICmpEQ(left, right, "cmptmp");
    } else if (expr.operator_token == "!=") {
        return builder.CreateICmpNE(left, right, "cmptmp");
    } else {
        throw std::runtime_error("Unknown binary operator: " + expr.operator_token);
    }
}

llvm::Value* CodeGenerator::generate_boolean_literal(const BooleanLiteral& lit) {
    return llvm::ConstantInt::get(context, llvm::APInt(1, lit.value ? 1 : 0));
}

llvm::Value* CodeGenerator::generate_number_literal(const NumberLiteral& lit) {
    return llvm::ConstantInt::get(context, llvm::APInt(32, lit.value));
}

llvm::Value* CodeGenerator::generate_range_expression(const RangeExpression& range) {
    // For now, we'll just return the limit value
    // In a full implementation, this would create an iterator object
    return generate_expression(*range.limit);
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