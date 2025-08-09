#pragma once

#include "token_types.h"
#include "string_table.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <cstdint>
#include <cassert>

namespace cprime {

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
 * Context-enriched token - Layer 2 output.
 * Contains original raw token plus contextual interpretation as enum.
 * This enables zero string comparisons in Layer 3 and provides clear semantic meaning.
 */
struct ContextualToken {
    // Original raw token (unchanged)
    RawToken raw_token;
    
    // Primary contextual interpretation (enum-based for performance)
    ContextualTokenKind contextual_kind;
    
    ContextualToken(const RawToken& raw_token, ContextualTokenKind contextual_kind)
        : raw_token(raw_token), contextual_kind(contextual_kind) {}
    
    // Primary accessors - use contextual_kind for semantic processing
    ContextualTokenKind get_contextual_kind() const { return contextual_kind; }
    bool is_contextual_kind(ContextualTokenKind kind) const { return contextual_kind == kind; }
    
    // Convenience accessors (delegate to raw_token)
    TokenKind kind() const { return raw_token.kind; }
    
    // String access with StringTable
    const std::string& get_string(const StringTable& string_table) const {
        return raw_token.get_string(string_table);
    }
    
    // Position information
    size_t line() const { return raw_token.line; }
    size_t column() const { return raw_token.column; }
    size_t position() const { return raw_token.position; }
    
    // Token type queries (enum-based only)
    bool is_identifier() const {
        return raw_token.kind == TokenKind::IDENTIFIER;
    }
    
    bool is_literal() const {
        return raw_token.is_literal();
    }
    
    bool is_operator() const {
        return raw_token.is_operator();
    }
    
    bool is_keyword() const {
        return raw_token.is_keyword();
    }
    
    // Debug representation
    std::string to_string() const;
};

} // namespace cprime