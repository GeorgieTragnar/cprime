#pragma once

namespace cprime {

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

/**
 * Convert ContextualTokenKind to string for debugging.
 */
const char* contextual_token_kind_to_string(ContextualTokenKind kind);

/**
 * Check if a ContextualTokenKind represents a literal value.
 */
bool is_contextual_literal(ContextualTokenKind kind);

/**
 * Check if a ContextualTokenKind represents an operator.
 */
bool is_contextual_operator(ContextualTokenKind kind);

/**
 * Check if a ContextualTokenKind represents a keyword.
 */
bool is_contextual_keyword(ContextualTokenKind kind);

/**
 * Check if a ContextualTokenKind represents a type declaration.
 */
bool is_contextual_type_declaration(ContextualTokenKind kind);

/**
 * Check if a ContextualTokenKind represents a context-sensitive interpretation.
 */
bool is_context_sensitive(ContextualTokenKind kind);

} // namespace cprime