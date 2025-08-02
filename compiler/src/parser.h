#pragma once

#include "ast.h"
#include "lexer.h"
#include <memory>

namespace cprime {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<Program> parse();
    
private:
    std::vector<Token> tokens;
    size_t pos;
    
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
    std::unique_ptr<Block> parse_block();
    std::unique_ptr<Statement> parse_statement();
    std::unique_ptr<FunctionCall> parse_function_call();
    std::unique_ptr<IfStatement> parse_if_statement();
    std::unique_ptr<WhileLoop> parse_while_loop();
    std::unique_ptr<ForLoop> parse_for_loop();
    
    // Expression parsing
    std::unique_ptr<Expression> parse_expression();
    std::unique_ptr<Expression> parse_comparison();
    std::unique_ptr<Expression> parse_primary();
    
    // Error handling
    void error(const std::string& message);
};

} // namespace cprime