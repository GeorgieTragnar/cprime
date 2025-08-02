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
    // Create function type using explicit return type
    llvm::Type* return_type = get_llvm_type(func.return_type);
    
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
    
    // Add return statement based on function return type
    if (func.return_type == Type::VOID) {
        builder.CreateRetVoid();
    } else if (func.return_type == Type::INT) {
        // For main function, return 0; for other int functions, also return 0 for now
        builder.CreateRet(llvm::ConstantInt::get(context, llvm::APInt(32, 0)));
    } else if (func.return_type == Type::BOOL) {
        // Return false for bool functions for now
        builder.CreateRet(llvm::ConstantInt::get(context, llvm::APInt(1, 0)));
    } else {
        // AUTO type should be resolved by now, but default to void
        builder.CreateRetVoid();
    }
}

void CodeGenerator::generate_block(const Block& block) {
    for (const auto& stmt : block.statements) {
        generate_statement(*stmt);
    }
}

void CodeGenerator::generate_statement(const Statement& stmt) {
    if (auto var_decl = dynamic_cast<const VariableDeclaration*>(&stmt)) {
        generate_variable_declaration(*var_decl);
    } else if (auto assignment = dynamic_cast<const Assignment*>(&stmt)) {
        generate_assignment(*assignment);
    } else if (auto call = dynamic_cast<const FunctionCall*>(&stmt)) {
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

void CodeGenerator::generate_variable_declaration(const VariableDeclaration& var_decl) {
    // Create alloca for the variable
    llvm::Type* llvm_type = get_llvm_type(var_decl.type);
    llvm::AllocaInst* alloca = builder.CreateAlloca(llvm_type, nullptr, var_decl.name);
    
    // Store the alloca for later reference
    variables[var_decl.name] = alloca;
    
    // Generate initializer and store it
    llvm::Value* init_value = generate_expression(*var_decl.initializer);
    builder.CreateStore(init_value, alloca);
}

void CodeGenerator::generate_assignment(const Assignment& assignment) {
    // Find the variable
    auto it = variables.find(assignment.name);
    if (it == variables.end()) {
        throw std::runtime_error("Undefined variable: " + assignment.name);
    }
    
    // Generate value and store it
    llvm::Value* value = generate_expression(*assignment.value);
    builder.CreateStore(value, it->second);
}

llvm::Type* CodeGenerator::get_llvm_type(Type type) {
    switch (type) {
        case Type::INT:
            return llvm::Type::getInt32Ty(context);
        case Type::BOOL:
            return llvm::Type::getInt1Ty(context);
        case Type::VOID:
            return llvm::Type::getVoidTy(context);
        default:
            throw std::runtime_error("Unknown type in get_llvm_type");
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
    // For range-based loops, we implement:
    // int i = 0;
    // while (i < limit) {
    //     // body
    //     i++;
    // }
    
    llvm::Function* current_func = builder.GetInsertBlock()->getParent();
    
    if (auto range = dynamic_cast<const RangeExpression*>(for_loop.iterable.get())) {
        if (auto limit_literal = dynamic_cast<const NumberLiteral*>(range->limit.get())) {
            int limit = limit_literal->value;
            
            // Create loop variable
            llvm::Type* int_type = llvm::Type::getInt32Ty(context);
            llvm::AllocaInst* loop_var = builder.CreateAlloca(int_type, nullptr, for_loop.variable);
            
            // Store the loop variable for body access
            std::string old_var_name = for_loop.variable;
            llvm::AllocaInst* old_var = nullptr;
            auto it = variables.find(old_var_name);
            if (it != variables.end()) {
                old_var = it->second;
            }
            variables[for_loop.variable] = loop_var;
            
            // Initialize to 0
            llvm::Value* zero = llvm::ConstantInt::get(context, llvm::APInt(32, 0));
            builder.CreateStore(zero, loop_var);
            
            // Create basic blocks
            llvm::BasicBlock* loop_header = llvm::BasicBlock::Create(context, "for.header", current_func);
            llvm::BasicBlock* loop_body = llvm::BasicBlock::Create(context, "for.body", current_func);
            llvm::BasicBlock* loop_exit = llvm::BasicBlock::Create(context, "for.exit", current_func);
            
            // Jump to loop header
            builder.CreateBr(loop_header);
            
            // Generate loop header (condition check)
            builder.SetInsertPoint(loop_header);
            llvm::Value* current_val = builder.CreateLoad(int_type, loop_var, "i");
            llvm::Value* limit_val = llvm::ConstantInt::get(context, llvm::APInt(32, limit));
            llvm::Value* condition = builder.CreateICmpSLT(current_val, limit_val, "loopcond");
            builder.CreateCondBr(condition, loop_body, loop_exit);
            
            // Generate loop body
            builder.SetInsertPoint(loop_body);
            generate_block(*for_loop.body);
            
            // Increment loop variable
            llvm::Value* incremented = builder.CreateAdd(current_val, 
                llvm::ConstantInt::get(context, llvm::APInt(32, 1)), "nextval");
            builder.CreateStore(incremented, loop_var);
            builder.CreateBr(loop_header);  // Jump back to header
            
            // Continue with exit block
            builder.SetInsertPoint(loop_exit);
            
            // Restore old variable if it existed, otherwise remove
            if (old_var) {
                variables[old_var_name] = old_var;
            } else {
                variables.erase(old_var_name);
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
    } else if (auto string = dynamic_cast<const StringLiteral*>(&expr)) {
        return generate_string_literal(*string);
    } else if (auto var_ref = dynamic_cast<const VariableReference*>(&expr)) {
        return generate_variable_reference(*var_ref);
    } else if (auto range = dynamic_cast<const RangeExpression*>(&expr)) {
        return generate_range_expression(*range);
    } else {
        throw std::runtime_error("Unknown expression type in code generation");
    }
}

llvm::Value* CodeGenerator::generate_binary_expression(const BinaryExpression& expr) {
    llvm::Value* left = generate_expression(*expr.left);
    llvm::Value* right = generate_expression(*expr.right);
    
    // Arithmetic operators
    if (expr.operator_token == "+") {
        return builder.CreateAdd(left, right, "addtmp");
    } else if (expr.operator_token == "-") {
        return builder.CreateSub(left, right, "subtmp");
    } else if (expr.operator_token == "*") {
        return builder.CreateMul(left, right, "multmp");
    } else if (expr.operator_token == "/") {
        return builder.CreateSDiv(left, right, "divtmp");
    } else if (expr.operator_token == "%") {
        return builder.CreateSRem(left, right, "modtmp");
    }
    // Comparison operators
    else if (expr.operator_token == "<") {
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

llvm::Value* CodeGenerator::generate_string_literal(const StringLiteral& lit) {
    return builder.CreateGlobalStringPtr(lit.value);
}

llvm::Value* CodeGenerator::generate_variable_reference(const VariableReference& var_ref) {
    // Find the variable
    auto it = variables.find(var_ref.name);
    if (it == variables.end()) {
        throw std::runtime_error("Undefined variable: " + var_ref.name);
    }
    
    // Load the value
    return builder.CreateLoad(it->second->getAllocatedType(), it->second, var_ref.name);
}

llvm::Value* CodeGenerator::generate_range_expression(const RangeExpression& range) {
    // For now, we'll just return the limit value
    // In a full implementation, this would create an iterator object
    return generate_expression(*range.limit);
}

void CodeGenerator::generate_function_call(const FunctionCall& call) {
    if (call.name == "print") {
        auto printf_func = get_or_declare_printf();
        
        if (call.args.empty()) {
            // Just print a newline
            auto str_constant = builder.CreateGlobalStringPtr("\n");
            builder.CreateCall(printf_func, {str_constant});
        } else {
            // First argument must be a format string
            auto format_expr = call.args[0].get();
            auto string_literal = dynamic_cast<const StringLiteral*>(format_expr);
            
            if (!string_literal) {
                throw std::runtime_error("First argument to print() must be a string literal with {} placeholders");
            }
            
            // Extract format string and remaining arguments
            std::string format_string = string_literal->value;
            std::vector<std::unique_ptr<Expression>> format_args;
            
            // Copy arguments after the format string
            for (size_t i = 1; i < call.args.size(); i++) {
                // We need to clone the expressions for processing
                // For now, we'll work with the originals and validate
            }
            
            // Validate placeholder count
            int placeholder_count = count_placeholders(format_string);
            int arg_count = static_cast<int>(call.args.size()) - 1; // Exclude format string
            
            if (placeholder_count != arg_count) {
                throw std::runtime_error("Format string has " + std::to_string(placeholder_count) + 
                                       " placeholders but " + std::to_string(arg_count) + " arguments provided");
            }
            
            // Process format string to generate printf-compatible format
            std::vector<std::unique_ptr<Expression>> args_only;
            for (size_t i = 1; i < call.args.size(); i++) {
                // Create a dummy vector for processing - we'll use the original args
            }
            
            // Manual processing since we can't easily copy unique_ptrs
            std::string processed_format;
            size_t pos = 0;
            size_t arg_index = 1; // Start from index 1 (skip format string)
            
            while (pos < format_string.length()) {
                if (pos + 1 < format_string.length() && format_string[pos] == '{' && format_string[pos + 1] == '}') {
                    // Found a placeholder {}
                    if (arg_index >= call.args.size()) {
                        throw std::runtime_error("Not enough arguments for format placeholders");
                    }
                    
                    // Replace {} with appropriate format specifier
                    std::string specifier = get_format_specifier(*call.args[arg_index]);
                    processed_format += specifier;
                    arg_index++;
                    pos += 2; // Skip {}
                } else if (pos + 1 < format_string.length() && format_string[pos] == '{' && format_string[pos + 1] == '{') {
                    // Escaped brace {{ -> {
                    processed_format += '{';
                    pos += 2;
                } else if (pos + 1 < format_string.length() && format_string[pos] == '}' && format_string[pos + 1] == '}') {
                    // Escaped brace }} -> }
                    processed_format += '}';
                    pos += 2;
                } else {
                    // Regular character
                    processed_format += format_string[pos];
                    pos++;
                }
            }
            
            // Add newline at the end
            processed_format += "\n";
            
            // Generate LLVM code
            auto format_constant = builder.CreateGlobalStringPtr(processed_format);
            
            // Collect argument values
            std::vector<llvm::Value*> llvm_args = {format_constant};
            for (size_t i = 1; i < call.args.size(); i++) {
                llvm::Value* arg_value = generate_expression(*call.args[i]);
                llvm_args.push_back(arg_value);
            }
            
            // Call printf
            builder.CreateCall(printf_func, llvm_args);
        }
    } else {
        throw std::runtime_error("Unknown function: " + call.name);
    }
}

std::string CodeGenerator::process_format_string(const std::string& format, const std::vector<std::unique_ptr<Expression>>& args) {
    std::string result;
    size_t pos = 0;
    size_t arg_index = 0;
    
    while (pos < format.length()) {
        if (pos + 1 < format.length() && format[pos] == '{' && format[pos + 1] == '}') {
            // Found a placeholder {}
            if (arg_index >= args.size()) {
                throw std::runtime_error("Not enough arguments for format placeholders");
            }
            
            // Replace {} with appropriate format specifier
            std::string specifier = get_format_specifier(*args[arg_index]);
            result += specifier;
            arg_index++;
            pos += 2; // Skip {}
        } else if (pos + 1 < format.length() && format[pos] == '{' && format[pos + 1] == '{') {
            // Escaped brace {{ -> {
            result += '{';
            pos += 2;
        } else if (pos + 1 < format.length() && format[pos] == '}' && format[pos + 1] == '}') {
            // Escaped brace }} -> }
            result += '}';
            pos += 2;
        } else {
            // Regular character
            result += format[pos];
            pos++;
        }
    }
    
    if (arg_index != args.size()) {
        throw std::runtime_error("Too many arguments for format placeholders");
    }
    
    return result;
}

std::string CodeGenerator::get_format_specifier(const Expression& expr) {
    // Determine format specifier based on expression type
    if (dynamic_cast<const StringLiteral*>(&expr)) {
        return "%s";
    } else if (dynamic_cast<const BooleanLiteral*>(&expr)) {
        return "%d";  // For now, booleans as integers
    } else if (dynamic_cast<const NumberLiteral*>(&expr)) {
        return "%d";
    } else if (auto var_ref = dynamic_cast<const VariableReference*>(&expr)) {
        // Look up variable type
        auto it = variables.find(var_ref->name);
        if (it != variables.end()) {
            // For now, assume all variables are int or bool (both use %d)
            return "%d";
        }
        return "%d"; // Default to int
    } else if (dynamic_cast<const BinaryExpression*>(&expr)) {
        // Binary expressions result in int or bool, both use %d
        return "%d";
    } else {
        return "%d"; // Default to int
    }
}

int CodeGenerator::count_placeholders(const std::string& format) {
    int count = 0;
    size_t pos = 0;
    
    while (pos < format.length()) {
        if (pos + 1 < format.length() && format[pos] == '{' && format[pos + 1] == '}') {
            count++;
            pos += 2;
        } else if (pos + 1 < format.length() && 
                  ((format[pos] == '{' && format[pos + 1] == '{') ||
                   (format[pos] == '}' && format[pos + 1] == '}'))) {
            // Skip escaped braces
            pos += 2;
        } else {
            pos++;
        }
    }
    
    return count;
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