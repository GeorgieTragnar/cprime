#pragma once

#include <cstdint>

namespace cprime {

/**
 * Pure token enum - no methods, no constructors.
 * Clean separation of token types from data structures.
 */
enum class EToken : uint16_t {
    INVALID = 0,
    // Literals
    INT_LITERAL,
    UINT_LITERAL,
    LONG_LITERAL,
    ULONG_LITERAL,
    FLOAT_LITERAL,
    DOUBLE_LITERAL,
    STRING_LITERAL,
    CHAR_LITERAL,
    BOOL_LITERAL,
    
    // Keywords - Context-sensitive
    CLASS = 100,
    STRUCT,
    INTERFACE,
    FUNCTION,
    RUNTIME,        // Context-sensitive: runtime allocation vs runtime keyword
    DEFER,          // Context-sensitive: defer statement vs defer modifier
    AUTO,
    VAR,
    CONST,
    
    // Keywords - Fixed meaning
    IF,
    ELSE,
    WHILE,
    FOR,
    RETURN,
    BREAK,
    CONTINUE,
    TRY,
    CATCH,
    FINALLY,
    
    // Operators
    PLUS = 200,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MODULO,
    ASSIGN,
    EQUALS,
    NOT_EQUALS,
    LESS_THAN,
    GREATER_THAN,
    LESS_EQUAL,
    GREATER_EQUAL,
    LOGICAL_AND,
    LOGICAL_OR,
    LOGICAL_NOT,
    
    // Punctuation
    LEFT_PAREN = 300,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    SEMICOLON,
    COMMA,
    DOT,
    COLON,
    ARROW,          // ->
    
    // Special
    IDENTIFIER = 400,
    WHITESPACE,
    COMMENT,
    NEWLINE,
    EOF_TOKEN
};

} // namespace cprime