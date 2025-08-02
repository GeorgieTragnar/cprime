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
    std::unique_ptr<FunctionCall> parse_function_call();
    
    // Error handling
    void error(const std::string& message);
};

} // namespace cprime