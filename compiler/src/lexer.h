#pragma once

#include <string>
#include <vector>

namespace cprime {

enum class TokenType {
    // Keywords
    FN,
    
    // Identifiers and literals
    IDENTIFIER,
    STRING_LITERAL,
    
    // Punctuation
    LBRACE,     // {
    RBRACE,     // }
    LPAREN,     // (
    RPAREN,     // )
    SEMICOLON,  // ;
    
    // Special
    EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;
    
    Token(TokenType type, const std::string& value, size_t line, size_t column)
        : type(type), value(value), line(line), column(column) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& input);
    std::vector<Token> tokenize();
    
private:
    std::string input;
    size_t pos;
    size_t line;
    size_t column;
    
    char peek();
    char peek_next();
    void advance();
    void skip_whitespace();
    Token read_identifier();
    Token read_string();
    
    bool is_at_end() const { return pos >= input.length(); }
};

} // namespace cprime