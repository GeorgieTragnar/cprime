#pragma once

#include "ast.h"
#include "lexer.h"
#include "symbol_table.h"
#include <memory>

namespace cprime {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<Program> parse();
    
private:
    std::vector<Token> tokens;
    size_t pos;
    SymbolTable symbol_table;
    
    // Token management
    Token& current();
    const Token& current() const;
    Token& previous();
    bool is_at_end() const;
    void advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    void consume(TokenType type, const std::string& error_msg);
    
    // Parsing methods
    std::unique_ptr<Function> parse_function();
    std::unique_ptr<ClassDefinition> parse_class();
    std::unique_ptr<FieldDeclaration> parse_field_declaration();
    std::unique_ptr<ConstructorDeclaration> parse_constructor_declaration(const std::string& class_name);
    std::unique_ptr<Block> parse_block();
    std::unique_ptr<Statement> parse_statement();
    std::unique_ptr<VariableDeclaration> parse_variable_declaration();
    std::unique_ptr<Assignment> parse_assignment();
    std::unique_ptr<FunctionCall> parse_function_call();
    std::unique_ptr<IfStatement> parse_if_statement();
    std::unique_ptr<WhileLoop> parse_while_loop();
    std::unique_ptr<ForLoop> parse_for_loop();
    
    // Expression parsing with precedence
    std::unique_ptr<Expression> parse_expression();
    std::unique_ptr<Expression> parse_logical_or();
    std::unique_ptr<Expression> parse_logical_and();
    std::unique_ptr<Expression> parse_equality();
    std::unique_ptr<Expression> parse_comparison();
    std::unique_ptr<Expression> parse_term();
    std::unique_ptr<Expression> parse_factor();
    std::unique_ptr<Expression> parse_primary();
    
    // Helper methods
    Type parse_type();
    std::unique_ptr<CustomType> parse_custom_type();
    std::unique_ptr<PointerType> parse_pointer_type();
    std::unique_ptr<ReferenceType> parse_reference_type();
    Type parse_base_type();
    bool is_type_keyword() const;
    bool is_constructor_declaration() const;
    bool peek_for_pointer() const;
    bool peek_for_reference() const;
    
    // Error handling
    void error(const std::string& message);
};

} // namespace cprime