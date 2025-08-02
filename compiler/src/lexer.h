#pragma once

#include <string>
#include <vector>

namespace cprime {

enum class TokenType {
    // Keywords
    IF,
    ELSE,
    WHILE,
    FOR,
    IN,
    TRUE,
    FALSE,
    RANGE,
    AUTO,
    INT,
    BOOL,
    VOID,
    
    // Identifiers and literals
    IDENTIFIER,
    STRING_LITERAL,
    NUMBER,
    
    // Punctuation
    LBRACE,     // {
    RBRACE,     // }
    LPAREN,     // (
    RPAREN,     // )
    SEMICOLON,  // ;
    COMMA,      // ,
    
    // Assignment and arithmetic operators
    ASSIGN,     // =
    PLUS,       // +
    MINUS,      // -
    MULTIPLY,   // *
    DIVIDE,     // /
    MODULO,     // %
    
    // Comparison operators
    LT,         // <
    GT,         // >
    LTEQ,       // <=
    GTEQ,       // >=
    EQ,         // ==
    NEQ,        // !=
    
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
    Token read_number();
    
    bool is_at_end() const { return pos >= input.length(); }
};

} // namespace cprime