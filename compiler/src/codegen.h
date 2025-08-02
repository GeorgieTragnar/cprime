#pragma once

#include "ast.h"
#include "symbol_table.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace cprime {

class CodeGenerator {
public:
    CodeGenerator(SymbolTable& symbol_table);
    void generate(const Program& program);
    void write_ir_to_file(const std::string& filename);
    
private:
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module;
    llvm::IRBuilder<> builder;
    SymbolTable& symbol_table;
    
    // Code generation methods
    void generate_function(const Function& func);
    void generate_class(const ClassDefinition& class_def);
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
    llvm::Value* generate_field_access(const FieldAccess& field_access);
    
    // Helper methods
    llvm::Function* get_or_declare_printf();
    llvm::Type* get_llvm_type(Type type);
    llvm::Type* get_llvm_type(const std::string& custom_type_name);
    llvm::StructType* get_or_create_struct_type(const std::string& class_name);
    
    // Print function helpers
    std::string process_format_string(const std::string& format, const std::vector<std::unique_ptr<Expression>>& args);
    std::string get_format_specifier(const Expression& expr);
    int count_placeholders(const std::string& format);
    
    // Object lifecycle management
    struct ObjectInfo {
        std::string name;
        std::string class_name;
        llvm::AllocaInst* alloca_inst;
        bool has_destructor;
        
        ObjectInfo(const std::string& name, const std::string& class_name, 
                  llvm::AllocaInst* alloca_inst, bool has_destructor)
            : name(name), class_name(class_name), alloca_inst(alloca_inst), has_destructor(has_destructor) {}
    };
    
    void enter_scope();
    void exit_scope();
    void generate_constructor_call(const std::string& class_name, llvm::AllocaInst* object_alloca);
    void generate_destructor_call(const ObjectInfo& obj);
    bool class_has_available_constructor(const std::string& class_name, SpecialMemberType constructor_type);
    bool class_has_available_destructor(const std::string& class_name);
    
    // Variable management
    std::unordered_map<std::string, llvm::AllocaInst*> variables;
    
    // Object lifecycle tracking (stack of scopes)
    std::vector<std::vector<ObjectInfo>> scope_objects;
    
    // Class type management
    std::unordered_map<std::string, llvm::StructType*> struct_types;
};

} // namespace cprime