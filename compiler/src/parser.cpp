#include "parser.h"
#include <stdexcept>
#include <sstream>
#include <string>

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
    
    func->body = parse_block();
    return func;
}

std::unique_ptr<Block> Parser::parse_block() {
    consume(TokenType::LBRACE, "Expected '{'");
    
    auto block = std::make_unique<Block>();
    
    while (!check(TokenType::RBRACE) && !is_at_end()) {
        block->statements.push_back(parse_statement());
    }
    
    consume(TokenType::RBRACE, "Expected '}'");
    return block;
}

std::unique_ptr<Statement> Parser::parse_statement() {
    if (match(TokenType::IF)) {
        return parse_if_statement();
    } else if (match(TokenType::WHILE)) {
        return parse_while_loop();
    } else if (match(TokenType::FOR)) {
        return parse_for_loop();
    } else if (match(TokenType::LBRACE)) {
        // Standalone block
        pos--; // Back up to consume the brace in parse_block
        return parse_block();
    } else if (check(TokenType::IDENTIFIER)) {
        auto call = parse_function_call();
        consume(TokenType::SEMICOLON, "Expected ';' after statement");
        return call;
    } else {
        error("Expected statement");
        return nullptr;
    }
}

std::unique_ptr<IfStatement> Parser::parse_if_statement() {
    consume(TokenType::LPAREN, "Expected '(' after 'if'");
    auto condition = parse_expression();
    consume(TokenType::RPAREN, "Expected ')' after condition");
    
    auto then_block = parse_block();
    
    std::unique_ptr<Block> else_block = nullptr;
    if (match(TokenType::ELSE)) {
        else_block = parse_block();
    }
    
    return std::make_unique<IfStatement>(std::move(condition), std::move(then_block), std::move(else_block));
}

std::unique_ptr<WhileLoop> Parser::parse_while_loop() {
    consume(TokenType::LPAREN, "Expected '(' after 'while'");
    auto condition = parse_expression();
    consume(TokenType::RPAREN, "Expected ')' after condition");
    
    auto body = parse_block();
    
    return std::make_unique<WhileLoop>(std::move(condition), std::move(body));
}

std::unique_ptr<ForLoop> Parser::parse_for_loop() {
    consume(TokenType::LPAREN, "Expected '(' after 'for'");
    consume(TokenType::IDENTIFIER, "Expected variable name");
    std::string variable = previous().value;
    
    consume(TokenType::IN, "Expected 'in' after variable");
    auto iterable = parse_expression();
    consume(TokenType::RPAREN, "Expected ')' after iterable");
    
    auto body = parse_block();
    
    return std::make_unique<ForLoop>(variable, std::move(iterable), std::move(body));
}

std::unique_ptr<Expression> Parser::parse_expression() {
    return parse_comparison();
}

std::unique_ptr<Expression> Parser::parse_comparison() {
    auto expr = parse_primary();
    
    while (match(TokenType::LT) || match(TokenType::GT) || match(TokenType::LTEQ) || 
           match(TokenType::GTEQ) || match(TokenType::EQ) || match(TokenType::NEQ)) {
        std::string op = previous().value;
        auto right = parse_primary();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parse_primary() {
    if (match(TokenType::TRUE)) {
        return std::make_unique<BooleanLiteral>(true);
    }
    
    if (match(TokenType::FALSE)) {
        return std::make_unique<BooleanLiteral>(false);
    }
    
    if (match(TokenType::NUMBER)) {
        int value = std::stoi(previous().value);
        return std::make_unique<NumberLiteral>(value);
    }
    
    if (match(TokenType::RANGE)) {
        consume(TokenType::LPAREN, "Expected '(' after 'range'");
        auto limit = parse_expression();
        consume(TokenType::RPAREN, "Expected ')' after range limit");
        return std::make_unique<RangeExpression>(std::move(limit));
    }
    
    if (match(TokenType::LPAREN)) {
        auto expr = parse_expression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    error("Expected expression");
    return nullptr;
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