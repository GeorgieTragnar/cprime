#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <variant>
#include <cstdint>
#include <cassert>
#include "../common/string_table.h"

// Forward declare logger to avoid circular includes
namespace spdlog { class logger; }

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
 * Raw token with comprehensive type information and typed literal values.
 * Uses string table indices for efficient string storage and deduplication.
 */
struct RawToken {
    TokenKind kind;
    
    // Tagged union for all primitive literal types  
    std::variant<
        // Integer types
        int32_t,           // INT_LITERAL
        uint32_t,          // UINT_LITERAL
        int64_t,           // LONG_LITERAL, LONG_LONG_LITERAL
        uint64_t,          // ULONG_LITERAL, ULONG_LONG_LITERAL
        
        // Floating point types
        float,             // FLOAT_LITERAL
        double,            // DOUBLE_LITERAL
        long double,       // LONG_DOUBLE_LITERAL
        
        // Character types
        char,              // CHAR_LITERAL
        wchar_t,           // WCHAR_LITERAL
        char16_t,          // CHAR16_LITERAL
        char32_t,          // CHAR32_LITERAL
        // Note: char8_t omitted - requires C++20
        
        // Boolean
        bool               // TRUE_LITERAL, FALSE_LITERAL
    > literal_value;
    
    // String reference through global string table
    StringTable::StringIndex string_index;  // IDENTIFIER, all STRING_*, COMMENT, WHITESPACE
    
    // Position information
    uint32_t line;
    uint32_t column; 
    uint32_t position;
    
    // Constructors
    RawToken(TokenKind k, uint32_t l, uint32_t c, uint32_t p)
        : kind(k), literal_value{}, string_index(StringTable::INVALID_INDEX), line(l), column(c), position(p) {}
    
    template<typename T>
    RawToken(TokenKind k, T literal, uint32_t l, uint32_t c, uint32_t p)
        : kind(k), literal_value(literal), string_index(StringTable::INVALID_INDEX), line(l), column(c), position(p) {}
    
    RawToken(TokenKind k, StringTable::StringIndex str_idx, uint32_t l, uint32_t c, uint32_t p)
        : kind(k), literal_value{}, string_index(str_idx), line(l), column(c), position(p) {}
    
    // Utility methods for checking token categories
    bool is_keyword() const {
        return kind >= TokenKind::CLASS && kind <= TokenKind::NAMESPACE;
    }
    
    bool is_operator() const {
        return kind >= TokenKind::PLUS && kind <= TokenKind::ELLIPSIS;
    }
    
    bool is_literal() const {
        return kind >= TokenKind::TRUE_LITERAL && kind <= TokenKind::RAW_STRING_LITERAL;
    }
    
    bool has_literal_value() const {
        return (kind >= TokenKind::INT_LITERAL && kind <= TokenKind::LONG_DOUBLE_LITERAL) ||
               (kind >= TokenKind::CHAR_LITERAL && kind <= TokenKind::CHAR32_LITERAL) ||
               kind == TokenKind::TRUE_LITERAL || kind == TokenKind::FALSE_LITERAL;
    }
    
    bool has_string_value() const {
        return kind == TokenKind::IDENTIFIER || 
               (kind >= TokenKind::STRING_LITERAL && kind <= TokenKind::RAW_STRING_LITERAL) ||
               kind == TokenKind::COMMENT || kind == TokenKind::WHITESPACE;
    }
    
    // Type-safe accessors with debug assertions
    int32_t get_int() const {
        assert(kind == TokenKind::INT_LITERAL);
        return std::get<int32_t>(literal_value);
    }
    
    uint32_t get_uint() const {
        assert(kind == TokenKind::UINT_LITERAL); 
        return std::get<uint32_t>(literal_value);
    }
    
    int64_t get_long() const {
        assert(kind == TokenKind::LONG_LITERAL || kind == TokenKind::LONG_LONG_LITERAL);
        return std::get<int64_t>(literal_value);
    }
    
    uint64_t get_ulong() const {
        assert(kind == TokenKind::ULONG_LITERAL || kind == TokenKind::ULONG_LONG_LITERAL);
        return std::get<uint64_t>(literal_value);
    }
    
    float get_float() const {
        assert(kind == TokenKind::FLOAT_LITERAL);
        return std::get<float>(literal_value);
    }
    
    double get_double() const {
        assert(kind == TokenKind::DOUBLE_LITERAL);
        return std::get<double>(literal_value);
    }
    
    long double get_long_double() const {
        assert(kind == TokenKind::LONG_DOUBLE_LITERAL);
        return std::get<long double>(literal_value);
    }
    
    bool get_bool() const {
        assert(kind == TokenKind::TRUE_LITERAL || kind == TokenKind::FALSE_LITERAL);
        return std::get<bool>(literal_value);
    }
    
    char get_char() const {
        assert(kind == TokenKind::CHAR_LITERAL);
        return std::get<char>(literal_value);
    }
    
    const std::string& get_string(const StringTable& string_table) const {
        assert(has_string_value());
        return string_table.get_string(string_index);
    }
    
    // Temporary backward-compatible method - will be removed once all layers use StringTable
    const std::string& get_string() const {
        static thread_local std::string empty_string = "[STRING_TABLE_NOT_PROVIDED]";
        // This is a temporary compatibility method that should not be used in production
        // All callers should be updated to pass StringTable
        return empty_string;
    }
    
    StringTable::StringIndex get_string_index() const {
        return string_index;
    }
    
    bool has_valid_string_index() const {
        return string_index != StringTable::INVALID_INDEX;
    }
    
    
    // Debug string representation
    std::string to_string(const StringTable& string_table) const;
};

/**
 * Raw token stream for convenient iteration and lookahead.
 */
class RawTokenStream {
public:
    explicit RawTokenStream(std::vector<RawToken> tokens);
    
    // Navigation
    const RawToken& current() const;
    const RawToken& peek(size_t offset = 1) const;
    const RawToken& previous() const;
    void advance();
    void rewind();
    bool is_at_end() const;
    
    // Position management
    size_t position() const { return pos; }
    void set_position(size_t new_pos);
    size_t size() const { return tokens.size(); }
    
    // Token access
    const std::vector<RawToken>& get_tokens() const { return tokens; }
    
private:
    std::vector<RawToken> tokens;
    size_t pos;
    
    // Bounds checking
    void ensure_valid_position() const;
};

/**
 * Raw tokenizer - Layer 1 of the three-layer architecture.
 * Converts source code into raw tokens without semantic interpretation.
 * Uses StringTable for efficient string storage and deduplication.
 */
class RawTokenizer {
public:
    explicit RawTokenizer(const std::string& source, StringTable& string_table);
    
    // Main tokenization method - returns tokens and populated string table
    std::vector<RawToken> tokenize();
    
    // Get result as stream for convenient processing
    RawTokenStream tokenize_to_stream();
    
    // Get the string table used during tokenization
    const StringTable& get_string_table() const { return string_table_; }
    
private:
    std::string source;
    size_t pos;
    size_t line;
    size_t column;
    StringTable& string_table_;  // Reference to string table for interning
    
    // Debug trace logging
    std::shared_ptr<spdlog::logger> trace_logger;
    
    // Tokenization mappings
    static const std::unordered_map<std::string, TokenKind> keywords;
    static const std::unordered_map<std::string, TokenKind> symbols;
    static const std::unordered_set<std::string> multi_char_symbols;
    
    // Character inspection
    char peek() const;
    char peek_next() const;
    void advance();
    bool is_at_end() const;
    
    // Whitespace and comments
    void skip_whitespace();
    RawToken read_line_comment();
    RawToken read_block_comment();
    
    // Token reading
    RawToken read_identifier_or_keyword();
    RawToken read_string_literal();
    RawToken read_integer_literal();
    RawToken read_float_literal();
    RawToken read_character_literal();
    
    // Literal parsing helpers
    bool peek_for_float_literal() const;
    TokenKind determine_integer_suffix(const std::string& suffix) const;
    TokenKind determine_float_suffix(const std::string& suffix) const;
    TokenKind determine_string_prefix(const std::string& prefix) const;
    TokenKind determine_character_prefix(const std::string& prefix) const;
    
    // String helpers for suffix/prefix detection
    bool ends_with_ci(const std::string& str, const std::string& suffix) const;
    bool starts_with(const std::string& str, const std::string& prefix) const;
    
    // Character classification
    bool is_alpha(char c) const;
    bool is_numeric(char c) const;
    bool is_alphanumeric(char c) const;
    bool is_whitespace(char c) const;
    
    // Error handling
    void error(const std::string& message) const;
    
    // Position tracking
    void update_position(char c);
    size_t current_position() const;
};

} // namespace cprime