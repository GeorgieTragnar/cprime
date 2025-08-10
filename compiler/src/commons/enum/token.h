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
    
    // Primitive Types
    INT8_T = 50,
    INT16_T,
    INT32_T,
    INT64_T,
    UINT8_T,
    UINT16_T,
    UINT32_T,
    UINT64_T,
    SIZE_T,
    FLOAT,
    DOUBLE,
    BOOL,
    CHAR,
    VOID,           // Void return type
    
    // Keywords - Context-sensitive
    CLASS = 100,
    STRUCT,
    INTERFACE,
    UNION,
    FUNCTION,
    FUNCTIONAL,     // Functional class declaration
    DATA,           // Data class declaration (explicit compatibility)
    RUNTIME,        // Context-sensitive: runtime allocation vs runtime keyword
    COMPTIME,       // Compile-time evaluation keyword
    CONSTEXPR,      // Compile-time evaluation (C++ style)
    DEFER,          // Context-sensitive: defer statement vs defer modifier
    AUTO,
    VAR,
    CONST,
    SEMCONST,       // Special field modifier for 1:1 move policy
    STATIC,         // Static variables
    INLINE,         // Forces inlining
    VOLATILE,       // Prevents optimization
    DANGER,         // Marks unsafe operations/code boundaries
    SUSPEND,        // Suspend function modifier (for coroutines)
    SPAWN,          // Task/coroutine spawning
    IMPLEMENTS,     // Interface implementation
    EXTERN,         // External template declaration
    MODULE,         // Module declaration
    DEFAULT,        // Default openness mode
    FUNC,           // Function section label (interfaces)
    OPEN,           // Open access mode
    CLOSED,         // Closed access mode
    
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
    RECOVER,        // Signal/exception recovery
    FINALLY,
    CO_AWAIT,       // Coroutine await keyword
    
    // Casting Keywords
    CAST,           // Type inspection for control flow (switch/if only)
    STATIC_CAST,    // Fast unsafe casting (C++ equivalent)
    DYNAMIC_CAST,   // Safe runtime casting (C++ equivalent)
    SELECT,         // Control flow for multiple awaits
    
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
    FIELD_LINK,     // <- (field linking operator)
    BITWISE_AND,    // & (also reference operator)
    BITWISE_OR,     // |
    BITWISE_XOR,    // ^
    BITWISE_NOT,    // ~
    DEREFERENCE,    // * (also multiply)
    SCOPE_RESOLUTION, // ::
    
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
    SINGLE_QUOTE,   // ' (lifetime/character literal delimiter)
    HASH,           // # (attribute prefix)
    
    // Special
    IDENTIFIER = 400,
    WHITESPACE,
    COMMENT,
    NEWLINE,
    EOF_TOKEN
};

} // namespace cprime