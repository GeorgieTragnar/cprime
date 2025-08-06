#include "../layer4/raii_injector.h"
#include "../layer3/ast_builder.h"
#include "../layer2/semantic_translator.h"
#include "../layer1/context_stack.h"
#include <iostream>
#include <sstream>

namespace cprime {

// Simple test utility to create a basic AST for testing
class RAIITestHelper {
public:
    static std::shared_ptr<ast::CompilationUnit> create_test_ast() {
        // Create a simple function with local variables that need RAII cleanup
        auto location = ast::SourceLocation(1, 1, 0, 10);
        
        // Create variable declarations
        auto int_type = std::make_shared<ast::Type>(ast::Type::Kind::Builtin, "int", location);
        auto class_type = std::make_shared<ast::Type>(ast::Type::Kind::Class, "TestClass", location);
        
        auto var1 = std::make_shared<ast::VarDecl>("x", int_type, nullptr, false, location);
        auto var2 = std::make_shared<ast::VarDecl>("obj", class_type, nullptr, false, location);
        
        // Create block with variables
        ast::StmtList block_statements;
        block_statements.push_back(var1);
        block_statements.push_back(var2);
        
        auto block = std::make_shared<ast::BlockStatement>(block_statements, location);
        
        // Create function with the block as body
        std::vector<ast::Parameter> params;
        auto void_type = std::make_shared<ast::Type>(ast::Type::Kind::Builtin, "void", location);
        auto func = std::make_shared<ast::FunctionDecl>("test_func", params, void_type, block, false, location);
        
        // Create compilation unit
        ast::DeclList declarations;
        declarations.push_back(func);
        
        return std::make_shared<ast::CompilationUnit>(declarations, location);
    }
    
    static void print_ast(std::shared_ptr<ast::CompilationUnit> unit) {
        std::cout << "AST Structure:\n";
        std::cout << unit->to_string() << std::endl;
    }
};

// AST printer visitor to see the structure
class ASTPrinter : public ast::ASTVisitor {
public:
    ASTPrinter() : indent_level(0) {}
    
    void visit(const ast::CompilationUnit& node) override {
        print_indent();
        std::cout << "CompilationUnit (" << node.get_declarations().size() << " declarations)\n";
        indent_level++;
        for (const auto& decl : node.get_declarations()) {
            decl->accept(*this);
        }
        indent_level--;
    }
    
    void visit(const ast::FunctionDecl& node) override {
        print_indent();
        std::cout << "FunctionDecl: " << node.get_name() << "\n";
        if (node.get_body()) {
            indent_level++;
            node.get_body()->accept(*this);
            indent_level--;
        }
    }
    
    void visit(const ast::BlockStatement& node) override {
        print_indent();
        std::cout << "BlockStatement (" << node.get_statements().size() << " statements)\n";
        indent_level++;
        for (const auto& stmt : node.get_statements()) {
            stmt->accept(*this);
        }
        indent_level--;
    }
    
    void visit(const ast::VarDecl& node) override {
        print_indent();
        std::cout << "VarDecl: " << node.get_name() << " : " << node.get_type()->get_name() << "\n";
    }
    
    void visit(const ast::ExprStatement& node) override {
        print_indent();
        std::cout << "ExprStatement\n";
        indent_level++;
        node.get_expression()->accept(*this);
        indent_level--;
    }
    
    void visit(const ast::CallExpr& node) override {
        print_indent();
        std::cout << "CallExpr (" << node.get_arguments().size() << " args)\n";
        indent_level++;
        node.get_callee()->accept(*this);
        for (const auto& arg : node.get_arguments()) {
            arg->accept(*this);
        }
        indent_level--;
    }
    
    void visit(const ast::MemberExpr& node) override {
        print_indent();
        std::cout << "MemberExpr: " << node.get_member() << "\n";
        indent_level++;
        node.get_object()->accept(*this);
        indent_level--;
    }
    
    void visit(const ast::IdentifierExpr& node) override {
        print_indent();
        std::cout << "IdentifierExpr: " << node.get_name() << "\n";
    }
    
    // Stub implementations for required visitor methods
    void visit(const ast::LiteralExpr& node) override { print_stub("LiteralExpr"); }
    void visit(const ast::BinaryExpr& node) override { print_stub("BinaryExpr"); }
    void visit(const ast::UnaryExpr& node) override { print_stub("UnaryExpr"); }
    void visit(const ast::IfStatement& node) override { print_stub("IfStatement"); }
    void visit(const ast::WhileStatement& node) override { print_stub("WhileStatement"); }
    void visit(const ast::ForStatement& node) override { print_stub("ForStatement"); }
    void visit(const ast::ReturnStatement& node) override { print_stub("ReturnStatement"); }
    void visit(const ast::DeferStatement& node) override { print_stub("DeferStatement"); }
    void visit(const ast::ClassDecl& node) override { print_stub("ClassDecl"); }
    void visit(const ast::StructDecl& node) override { print_stub("StructDecl"); }
    void visit(const ast::UnionDecl& node) override { print_stub("UnionDecl"); }
    void visit(const ast::InterfaceDecl& node) override { print_stub("InterfaceDecl"); }
    void visit(const ast::Type& node) override { print_stub("Type"); }
    
private:
    int indent_level;
    
    void print_indent() {
        for (int i = 0; i < indent_level; i++) {
            std::cout << "  ";
        }
    }
    
    void print_stub(const std::string& name) {
        print_indent();
        std::cout << name << " (stub)\n";
    }
};

} // namespace cprime

int main() {
    using namespace cprime;
    
    std::cout << "=== RAII Injector Test ===\n\n";
    
    try {
        // Create a test AST
        auto ast = RAIITestHelper::create_test_ast();
        
        std::cout << "Original AST:\n";
        ASTPrinter printer;
        ast->accept(printer);
        std::cout << "\n";
        
        // Create symbol table and RAII injector
        SymbolTable symbol_table;
        RAIIInjector injector(symbol_table);
        
        std::cout << "Processing with RAII Injector...\n\n";
        
        // Process the AST
        auto processed_ast = injector.process(ast);
        
        std::cout << "AST after RAII injection:\n";
        processed_ast->accept(printer);
        
        std::cout << "\n=== Test Complete ===\n";
        std::cout << "RAII Injector test completed successfully!\n";
        std::cout << "Note: Full functionality requires a complete parser pipeline.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}