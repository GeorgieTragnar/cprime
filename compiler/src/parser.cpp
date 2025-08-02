#include "parser.h"
#include <stdexcept>
#include <sstream>

namespace cprime {

Parser::Parser(std::vector<Token> tokens) 
    : tokens(std::move(tokens)), pos(0) {}

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();
    
    while (!is_at_end()) {
        if (match(TokenType::FN)) {
            program->functions.push_back(parse_function());
        } else {
            error("Expected function definition");
        }
    }
    
    return program;
}

std::unique_ptr<Function> Parser::parse_function() {
    // We already consumed 'fn'
    consume(TokenType::IDENTIFIER, "Expected function name");
    std::string name = previous().value;
    
    auto func = std::make_unique<Function>(name);
    
    consume(TokenType::LPAREN, "Expected '(' after function name");
    consume(TokenType::RPAREN, "Expected ')' after '(' (no parameters supported yet)");
    consume(TokenType::LBRACE, "Expected '{' to begin function body");
    
    // Parse function body
    while (!check(TokenType::RBRACE) && !is_at_end()) {
        if (check(TokenType::IDENTIFIER)) {
            func->body.push_back(parse_function_call());
            consume(TokenType::SEMICOLON, "Expected ';' after statement");
        } else {
            error("Expected statement in function body");
        }
    }
    
    consume(TokenType::RBRACE, "Expected '}' to end function body");
    return func;
}

std::unique_ptr<FunctionCall> Parser::parse_function_call() {
    consume(TokenType::IDENTIFIER, "Expected function name");
    std::string name = previous().value;
    
    auto call = std::make_unique<FunctionCall>(name);
    
    consume(TokenType::LPAREN, "Expected '(' after function name");
    
    // Parse arguments (just string literals for now)
    if (check(TokenType::STRING_LITERAL)) {
        call->args.push_back(current().value);
        advance();
        
        // For now, we only support single argument
        // Later we can add comma-separated args
    }
    
    consume(TokenType::RPAREN, "Expected ')' after arguments");
    return call;
}

Token& Parser::current() {
    return tokens[pos];
}

const Token& Parser::current() const {
    return tokens[pos];
}

Token& Parser::previous() {
    return tokens[pos - 1];
}

bool Parser::is_at_end() const {
    return current().type == TokenType::EOF_TOKEN;
}

void Parser::advance() {
    if (!is_at_end()) pos++;
}

bool Parser::check(TokenType type) const {
    if (is_at_end()) return false;
    return current().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

void Parser::consume(TokenType type, const std::string& error_msg) {
    if (check(type)) {
        advance();
        return;
    }
    
    error(error_msg);
}

void Parser::error(const std::string& message) {
    std::ostringstream oss;
    Token& tok = current();
    oss << "Parse error at line " << tok.line << ", column " << tok.column 
        << ": " << message;
    
    if (tok.type != TokenType::EOF_TOKEN) {
        oss << " (found '" << tok.value << "')";
    } else {
        oss << " (reached end of file)";
    }
    
    throw std::runtime_error(oss.str());
}

} // namespace cprime