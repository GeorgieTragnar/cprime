#include "lexer.h"
#include <cctype>
#include <stdexcept>

namespace cprime {

Lexer::Lexer(const std::string& input) 
    : input(input), pos(0), line(1), column(1) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (!is_at_end()) {
        skip_whitespace();
        
        if (is_at_end()) break;
        
        size_t start_line = line;
        size_t start_column = column;
        char c = peek();
        
        if (c == '{') {
            tokens.emplace_back(TokenType::LBRACE, "{", start_line, start_column);
            advance();
        } else if (c == '}') {
            tokens.emplace_back(TokenType::RBRACE, "}", start_line, start_column);
            advance();
        } else if (c == '(') {
            tokens.emplace_back(TokenType::LPAREN, "(", start_line, start_column);
            advance();
        } else if (c == ')') {
            tokens.emplace_back(TokenType::RPAREN, ")", start_line, start_column);
            advance();
        } else if (c == ';') {
            tokens.emplace_back(TokenType::SEMICOLON, ";", start_line, start_column);
            advance();
        } else if (c == ',') {
            tokens.emplace_back(TokenType::COMMA, ",", start_line, start_column);
            advance();
        } else if (c == ':') {
            tokens.emplace_back(TokenType::COLON, ":", start_line, start_column);
            advance();
        } else if (c == '.') {
            tokens.emplace_back(TokenType::DOT, ".", start_line, start_column);
            advance();
        } else if (c == '<') {
            if (peek_next() == '=') {
                tokens.emplace_back(TokenType::LTEQ, "<=", start_line, start_column);
                advance();
                advance();
            } else {
                tokens.emplace_back(TokenType::LT, "<", start_line, start_column);
                advance();
            }
        } else if (c == '>') {
            if (peek_next() == '=') {
                tokens.emplace_back(TokenType::GTEQ, ">=", start_line, start_column);
                advance();
                advance();
            } else {
                tokens.emplace_back(TokenType::GT, ">", start_line, start_column);
                advance();
            }
        } else if (c == '=') {
            if (peek_next() == '=') {
                tokens.emplace_back(TokenType::EQ, "==", start_line, start_column);
                advance();
                advance();
            } else {
                tokens.emplace_back(TokenType::ASSIGN, "=", start_line, start_column);
                advance();
            }
        } else if (c == '!') {
            if (peek_next() == '=') {
                tokens.emplace_back(TokenType::NEQ, "!=", start_line, start_column);
                advance();
                advance();
            } else {
                throw std::runtime_error("Unexpected character '!' at line " + 
                                       std::to_string(line) + ", column " + std::to_string(column) +
                                       " (did you mean '!='?)");
            }
        } else if (c == '+') {
            tokens.emplace_back(TokenType::PLUS, "+", start_line, start_column);
            advance();
        } else if (c == '-') {
            tokens.emplace_back(TokenType::MINUS, "-", start_line, start_column);
            advance();
        } else if (c == '*') {
            tokens.emplace_back(TokenType::MULTIPLY, "*", start_line, start_column);
            advance();
        } else if (c == '/') {
            tokens.emplace_back(TokenType::DIVIDE, "/", start_line, start_column);
            advance();
        } else if (c == '%') {
            tokens.emplace_back(TokenType::MODULO, "%", start_line, start_column);
            advance();
        } else if (c == '&') {
            if (peek_next() == '&') {
                tokens.emplace_back(TokenType::DOUBLE_AMPERSAND, "&&", start_line, start_column);
                advance();
                advance();
            } else {
                tokens.emplace_back(TokenType::AMPERSAND, "&", start_line, start_column);
                advance();
            }
        } else if (c == '~') {
            tokens.emplace_back(TokenType::TILDE, "~", start_line, start_column);
            advance();
        } else if (c == '"') {
            tokens.push_back(read_string());
        } else if (std::isdigit(c)) {
            tokens.push_back(read_number());
        } else if (std::isalpha(c) || c == '_') {
            tokens.push_back(read_identifier());
        } else {
            throw std::runtime_error("Unexpected character '" + std::string(1, c) + 
                                   "' at line " + std::to_string(line) + 
                                   ", column " + std::to_string(column));
        }
    }
    
    tokens.emplace_back(TokenType::EOF_TOKEN, "", line, column);
    return tokens;
}

char Lexer::peek() {
    return is_at_end() ? '\0' : input[pos];
}

char Lexer::peek_next() {
    return (pos + 1 >= input.length()) ? '\0' : input[pos + 1];
}

void Lexer::advance() {
    if (!is_at_end()) {
        if (input[pos] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        pos++;
    }
}

void Lexer::skip_whitespace() {
    while (!is_at_end()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peek_next() == '/') {
            // Single-line comment
            advance(); // skip first /
            advance(); // skip second /
            while (!is_at_end() && peek() != '\n') {
                advance();
            }
        } else {
            break;
        }
    }
}

Token Lexer::read_identifier() {
    size_t start_line = line;
    size_t start_column = column;
    size_t start_pos = pos;
    
    while (!is_at_end() && (std::isalnum(peek()) || peek() == '_')) {
        advance();
    }
    
    std::string value = input.substr(start_pos, pos - start_pos);
    
    TokenType type = TokenType::IDENTIFIER;
    if (value == "if") type = TokenType::IF;
    else if (value == "else") type = TokenType::ELSE;
    else if (value == "while") type = TokenType::WHILE;
    else if (value == "for") type = TokenType::FOR;
    else if (value == "in") type = TokenType::IN;
    else if (value == "true") type = TokenType::TRUE;
    else if (value == "false") type = TokenType::FALSE;
    else if (value == "range") type = TokenType::RANGE;
    else if (value == "auto") type = TokenType::AUTO;
    else if (value == "int") type = TokenType::INT;
    else if (value == "bool") type = TokenType::BOOL;
    else if (value == "void") type = TokenType::VOID;
    else if (value == "class") type = TokenType::CLASS;
    else if (value == "default") type = TokenType::DEFAULT;
    else if (value == "delete") type = TokenType::DELETE;
    else if (value == "explicit") type = TokenType::EXPLICIT;
    else if (value == "const") type = TokenType::CONST;
    else if (value == "operator") type = TokenType::OPERATOR;
    
    return Token(type, value, start_line, start_column);
}

Token Lexer::read_string() {
    size_t start_line = line;
    size_t start_column = column;
    
    advance(); // skip opening quote
    size_t start_pos = pos;
    
    while (!is_at_end() && peek() != '"') {
        if (peek() == '\\' && peek_next() == '"') {
            advance(); // skip backslash
        }
        advance();
    }
    
    if (is_at_end()) {
        throw std::runtime_error("Unterminated string at line " + 
                               std::to_string(start_line));
    }
    
    std::string value = input.substr(start_pos, pos - start_pos);
    advance(); // skip closing quote
    
    return Token(TokenType::STRING_LITERAL, value, start_line, start_column);
}

Token Lexer::read_number() {
    size_t start_line = line;
    size_t start_column = column;
    size_t start_pos = pos;
    
    while (!is_at_end() && std::isdigit(peek())) {
        advance();
    }
    
    std::string value = input.substr(start_pos, pos - start_pos);
    return Token(TokenType::NUMBER, value, start_line, start_column);
}

} // namespace cprime