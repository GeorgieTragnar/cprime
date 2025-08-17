#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace cprime {

/**
 * Pure token enum - no methods, no constructors.
 * Clean separation of token types from data structures.
 */
enum class EToken : uint16_t {
    INVALID = 0,
    
    // Literals - Reserved range: 10-49
    INT_LITERAL = 10,
    UINT_LITERAL,
    LONG_LITERAL,
    ULONG_LITERAL,
    LONG_LONG_LITERAL,
    ULONG_LONG_LITERAL,
    
    // Floating point literals
    FLOAT_LITERAL,
    DOUBLE_LITERAL,
    LONG_DOUBLE_LITERAL,
    
    // Character literals
    CHAR_LITERAL,
    WCHAR_LITERAL,
    CHAR16_LITERAL,
    CHAR32_LITERAL,
    
    // String literals
    STRING_LITERAL,
    WSTRING_LITERAL,
    STRING16_LITERAL,
    STRING32_LITERAL,
    STRING8_LITERAL,
    RAW_STRING_LITERAL,
    
    // Boolean and null literals
    TRUE_LITERAL,
    FALSE_LITERAL,
    NULLPTR_LITERAL,
    // Reserved space for future literals: 33-49
    
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
    CONST,
    SEMCONST,       // Special field modifier for 1:1 move policy
    STATIC,         // Static variables
    INLINE,         // Forces inlining
    VOLATILE,       // Prevents optimization
    DANGER,         // Marks unsafe operations/code boundaries
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
    SIGNAL,         // Signal definition
    EXCEPT,         // Function signal annotations
    RAISE,          // Raise/emit signals
    
    // Casting Keywords
    CAST,           // Type inspection for control flow (switch/if only)
    STATIC_CAST,    // Fast unsafe casting (C++ equivalent)
    DYNAMIC_CAST,   // Safe runtime casting (C++ equivalent)
    SELECT,         // Control flow for multiple awaits
    EXEC,           // Exec block keyword for compile-time code execution
    EXEC_ALIAS,     // Dynamically registered exec template alias
    
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
    
    // Whitespace Types - Reserved range: 400-410
    SPACE = 400,         // ' ' (0x20)
    TAB,                 // '\t' (0x09)
    CARRIAGE_RETURN,     // '\r' (0x0D)
    VERTICAL_TAB,        // '\v' (0x0B)  
    FORM_FEED,           // '\f' (0x0C)
    
    // Special
    IDENTIFIER = 420,
    CHUNK,              // Unresolved identifier chunk awaiting context-aware resolution
    COMMENT,
    NEWLINE,
    EOF_TOKEN
};

// Centralized keyword-to-token mapping
extern const std::unordered_map<std::string, EToken> KEYWORD_TO_ETOKEN_MAP;

// Helper function for compilation - string to token conversion
EToken string_to_etoken(const std::string& keyword);

} // namespace cprime