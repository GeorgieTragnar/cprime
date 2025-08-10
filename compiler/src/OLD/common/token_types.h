#pragma once

namespace cprime {

/**
 * Comprehensive token classification for CPrime language.
 * Each token kind represents a specific lexical element with no ambiguity.
 * This eliminates string comparisons in Layer 2 and provides typed literal values.
 */
enum class TokenKind {
    // === KEYWORDS ===
    // Core language constructs
    CLASS, STRUCT, UNION, INTERFACE, PLEX,
    
    // Context-sensitive keywords (reserved but meaning depends on context)
    RUNTIME, DEFER,
    
    // Control flow
    IF, ELSE, WHILE, FOR, CASE, SWITCH, DEFAULT,
    BREAK, CONTINUE, RETURN, GOTO,
    
    // Exception handling
    THROW, TRY, CATCH,
    
    // Type system
    AUTO, VOID, BOOL, CHAR, WCHAR_T,
    INT, SHORT, LONG, SIGNED, UNSIGNED,
    FLOAT, DOUBLE,
    INT8_T, INT16_T, INT32_T, INT64_T,
    UINT8_T, UINT16_T, UINT32_T, UINT64_T,
    CHAR8_T, CHAR16_T, CHAR32_T,
    
    // Type qualifiers and storage
    CONST, MUT, STATIC, EXTERN, REGISTER, THREAD_LOCAL, VOLATILE,
    CONSTEXPR, CONSTEVAL, CONSTINIT, NOEXCEPT, INLINE,
    
    // Memory management
    NEW, DELETE, DANGER,
    
    // Access control
    PUBLIC, PRIVATE, PROTECTED, FRIEND,
    
    // Metaprogramming
    SIZEOF, ALIGNOF, ALIGNAS, DECLTYPE, TYPEOF, TYPEID,
    TEMPLATE, TYPENAME, USING, NAMESPACE,
    
    // === OPERATORS AND PUNCTUATION ===
    // Arithmetic operators
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO,
    
    // Assignment operators
    ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN, MULTIPLY_ASSIGN, DIVIDE_ASSIGN, MODULO_ASSIGN,
    
    // Increment/decrement
    INCREMENT, DECREMENT,
    
    // Comparison operators
    EQUAL_EQUAL, NOT_EQUAL, LESS_THAN, GREATER_THAN, LESS_EQUAL, GREATER_EQUAL,
    SPACESHIP, // <=> (three-way comparison)
    
    // Logical operators
    LOGICAL_AND, LOGICAL_OR, LOGICAL_NOT,
    
    // Bitwise operators
    BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, LEFT_SHIFT, RIGHT_SHIFT,
    BIT_AND_ASSIGN, BIT_OR_ASSIGN, BIT_XOR_ASSIGN, LEFT_SHIFT_ASSIGN, RIGHT_SHIFT_ASSIGN,
    
    // Member access
    DOT, ARROW, SCOPE_RESOLUTION, DOT_STAR, ARROW_STAR,
    
    // Punctuation
    LEFT_PAREN, RIGHT_PAREN,        // ( )
    LEFT_BRACE, RIGHT_BRACE,        // { }
    LEFT_BRACKET, RIGHT_BRACKET,    // [ ]
    SEMICOLON, COMMA, COLON, QUESTION, ELLIPSIS,
    
    // === LITERALS ===
    // Boolean and null
    TRUE_LITERAL, FALSE_LITERAL, NULLPTR_LITERAL,
    
    // Integer literals (with suffix variants)
    INT_LITERAL,           // 42
    UINT_LITERAL,          // 42u, 42U
    LONG_LITERAL,          // 42l, 42L  
    ULONG_LITERAL,         // 42ul, 42UL, 42Lu, 42LU
    LONG_LONG_LITERAL,     // 42ll, 42LL
    ULONG_LONG_LITERAL,    // 42ull, 42ULL, 42LLu, 42LLU
    
    // Floating point literals
    FLOAT_LITERAL,         // 3.14f, 3.14F
    DOUBLE_LITERAL,        // 3.14 (default floating point)
    LONG_DOUBLE_LITERAL,   // 3.14l, 3.14L
    
    // Character literals
    CHAR_LITERAL,          // 'c'
    WCHAR_LITERAL,         // L'c'
    CHAR16_LITERAL,        // u'c' 
    CHAR32_LITERAL,        // U'c'
    // CHAR8_LITERAL,      // u8'c' (requires C++20, omitted for C++17 compatibility)
    
    // String literals
    STRING_LITERAL,        // "hello"
    WSTRING_LITERAL,       // L"hello"
    STRING16_LITERAL,      // u"hello"
    STRING32_LITERAL,      // U"hello" 
    STRING8_LITERAL,       // u8"hello" (C++20 - limited support in C++17)
    RAW_STRING_LITERAL,    // R"delimiter(content)delimiter"
    
    // === DYNAMIC TOKENS ===
    IDENTIFIER,            // Variable names, function names, type names
    COMMENT,               // Line and block comments
    WHITESPACE,            // Spaces, tabs, newlines
    EOF_TOKEN,             // End of file marker
};

/**
 * Comprehensive contextual token classification for Layer 2 output.
 * Each value represents a context-resolved interpretation of raw tokens.
 * This enables zero string comparisons in Layer 2 and provides clear
 * semantic meaning for Layer 3 processing.
 */
enum class ContextualTokenKind {
    // === DIRECT MAPPINGS (no context change needed) ===
    // These map directly from TokenKind with no contextual interpretation
    
    // Basic tokens
    IDENTIFIER,              // Regular identifier (not context-sensitive)
    COMMENT,                 // Source code comment
    WHITESPACE,              // Whitespace token
    EOF_TOKEN,               // End of file
    
    // Literals (direct from TokenKind)
    INT_LITERAL,             // Integer literal
    UINT_LITERAL,            // Unsigned integer literal
    LONG_LITERAL,            // Long integer literal
    ULONG_LITERAL,           // Unsigned long literal
    LONG_LONG_LITERAL,       // Long long literal
    ULONG_LONG_LITERAL,      // Unsigned long long literal
    FLOAT_LITERAL,           // Float literal
    DOUBLE_LITERAL,          // Double literal
    LONG_DOUBLE_LITERAL,     // Long double literal
    CHAR_LITERAL,            // Character literal
    WCHAR_LITERAL,           // Wide character literal
    CHAR16_LITERAL,          // UTF-16 character literal
    CHAR32_LITERAL,          // UTF-32 character literal
    STRING_LITERAL,          // String literal
    WSTRING_LITERAL,         // Wide string literal
    STRING16_LITERAL,        // UTF-16 string literal
    STRING32_LITERAL,        // UTF-32 string literal
    STRING8_LITERAL,         // UTF-8 string literal
    RAW_STRING_LITERAL,      // Raw string literal
    TRUE_LITERAL,            // Boolean true
    FALSE_LITERAL,           // Boolean false
    NULLPTR_LITERAL,         // Null pointer literal
    
    // Operators and punctuation (direct mapping)
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO,
    ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN, MULTIPLY_ASSIGN, DIVIDE_ASSIGN, MODULO_ASSIGN,
    INCREMENT, DECREMENT,
    EQUAL_EQUAL, NOT_EQUAL, LESS_THAN, GREATER_THAN, LESS_EQUAL, GREATER_EQUAL, SPACESHIP,
    LOGICAL_AND, LOGICAL_OR, LOGICAL_NOT,
    BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, LEFT_SHIFT, RIGHT_SHIFT,
    BIT_AND_ASSIGN, BIT_OR_ASSIGN, BIT_XOR_ASSIGN, LEFT_SHIFT_ASSIGN, RIGHT_SHIFT_ASSIGN,
    DOT, ARROW, SCOPE_RESOLUTION, DOT_STAR, ARROW_STAR,
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, LEFT_BRACKET, RIGHT_BRACKET,
    SEMICOLON, COMMA, COLON, QUESTION, ELLIPSIS,
    
    // Basic keywords (direct mapping)
    IF, ELSE, WHILE, FOR, CASE, SWITCH, DEFAULT, BREAK, CONTINUE, RETURN, GOTO,
    THROW, TRY, CATCH,
    AUTO, VOID, BOOL, CHAR, WCHAR_T, INT, SHORT, LONG, SIGNED, UNSIGNED, FLOAT, DOUBLE,
    INT8_T, INT16_T, INT32_T, INT64_T, UINT8_T, UINT16_T, UINT32_T, UINT64_T,
    CHAR8_T, CHAR16_T, CHAR32_T,
    CONST, MUT, STATIC, EXTERN, REGISTER, THREAD_LOCAL, VOLATILE,
    CONSTEXPR, CONSTEVAL, CONSTINIT, NOEXCEPT, INLINE,
    NEW, DELETE, DANGER,
    PUBLIC, PRIVATE, PROTECTED, FRIEND,
    SIZEOF, ALIGNOF, ALIGNAS, DECLTYPE, TYPEOF, TYPEID,
    TEMPLATE, TYPENAME, USING, NAMESPACE,
    
    // === CONTEXT-SENSITIVE INTERPRETATIONS ===
    // These represent contextual resolutions of ambiguous keywords
    
    // Runtime keyword contexts
    RUNTIME_ACCESS_RIGHT,        // "runtime exposes UserOps { ... }"
    RUNTIME_TYPE_PARAMETER,      // "Connection<runtime UserOps>"
    RUNTIME_VARIABLE_DECL,       // "let conn: runtime Connection = ..."
    RUNTIME_UNION_DECLARATION,   // "union runtime State { ... }"
    RUNTIME_COROUTINE,           // "runtime coroutine_func()" (future)
    
    // Defer keyword contexts
    DEFER_RAII,                  // "defer FileOps::destruct(&mut file)"
    DEFER_COROUTINE,             // "co_defer cleanup_resources()"
    DEFER_SCOPE_GUARD,           // "defer { cleanup_code(); }" (future)
    
    // Class type contexts
    DATA_CLASS,                  // "class User { ... }"
    FUNCTIONAL_CLASS,            // "functional class Calculator { ... }"
    DANGER_CLASS,                // "danger class RawPointer { ... }"
    STRUCT_DECLARATION,          // "struct Point { ... }"
    UNION_DECLARATION,           // "union Value { ... }"
    INTERFACE_DECLARATION,       // "interface Drawable { ... }"
    PLEX_DECLARATION,            // "plex MultiType { ... }"
    
    // Special identifier promotions (context-dependent keywords)
    EXPOSES_COMPILE_TIME,        // "exposes UserOps { ... }" (standalone)
    EXPOSES_RUNTIME,             // "runtime exposes UserOps { ... }"
    FUNCTION_DECLARATION,        // "fn" keyword in function context
    ASYNC_FUNCTION_DECLARATION,  // "async fn" in function context
    
    // Access right contexts
    ACCESS_RIGHT_DECLARATION,    // Access right being declared
    ACCESS_RIGHT_USAGE,          // Access right being used/referenced
    
    // Type expression contexts
    TYPE_IDENTIFIER,             // Identifier used as type name
    GENERIC_TYPE_PARAMETER,      // Type parameter in generic context
    
    // === FUTURE EXTENSIBILITY ===
    // Placeholders for future CPrime language features
    
    CAPABILITY_GRANT,            // Future capability security feature
    COROUTINE_YIELD,             // Future coroutine yield points
    ASYNC_AWAIT,                 // Future async/await syntax
    MEMORY_REGION,               // Future memory region annotations
    COMPILE_TIME_EVAL,           // Future compile-time evaluation
    
    // === ERROR HANDLING AND TODOS ===
    CONTEXTUAL_TODO,             // Placeholder for unimplemented contexts
    CONTEXTUAL_ERROR,            // Invalid contextual interpretation
    CONTEXTUAL_UNKNOWN,          // Unknown context - needs investigation
};

} // namespace cprime