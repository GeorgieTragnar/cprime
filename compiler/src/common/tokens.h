#pragma once

#include "token_types.h"
#include "parse_context.h"
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
 * Context attributes for storing metadata about token resolution.
 * Used to pass context-specific information along with tokens.
 */
struct ContextAttributes {
    std::unordered_map<std::string, std::string> data;
    
    void set(const std::string& key, const std::string& value) {
        data[key] = value;
    }
    
    std::string get(const std::string& key, const std::string& default_value = "") const {
        auto it = data.find(key);
        return it != data.end() ? it->second : default_value;
    }
    
    bool has(const std::string& key) const {
        return data.find(key) != data.end();
    }
    
    bool empty() const { return data.empty(); }
};

/**
 * Context-enriched token - Layer 2 output.
 * Contains original raw token plus full context information with enum-based resolution.
 * This enables zero string comparisons in Layer 3 and provides clear semantic meaning.
 */
struct ContextualToken {
    // Original raw token (unchanged)
    RawToken raw_token;
    
    // Primary contextual interpretation (enum-based for performance)
    ContextualTokenKind contextual_kind;
    
    // Context information
    ParseContextType current_context;
    std::vector<ParseContextType> context_stack;  // Stack snapshot at token time
    std::string context_resolution;               // Deprecated - use contextual_kind instead
    ContextAttributes attributes;                 // Context-specific metadata
    
    ContextualToken(const RawToken& raw_token, ContextualTokenKind contextual_kind, ParseContextType context)
        : raw_token(raw_token), contextual_kind(contextual_kind), current_context(context) {}
    
    // Backward compatibility constructor - will be removed
    ContextualToken(const RawToken& raw_token, ParseContextType context)
        : raw_token(raw_token), contextual_kind(ContextualTokenKind::CONTEXTUAL_TODO), current_context(context) {}
    
    // Primary accessors - use contextual_kind for semantic processing
    ContextualTokenKind get_contextual_kind() const { return contextual_kind; }
    bool is_contextual_kind(ContextualTokenKind kind) const { return contextual_kind == kind; }
    
    // Convenience accessors (delegate to raw_token)
    TokenKind kind() const { return raw_token.kind; }
    const std::string& value() const { 
        if (raw_token.has_string_value()) {
            return raw_token.get_string();
        }
        static const std::string empty_string = "";
        return empty_string;
    }
    
    // String access with StringTable
    const std::string& get_string(const StringTable& string_table) const {
        return raw_token.get_string(string_table);
    }
    
    // Legacy accessor for compatibility
    TokenKind type() const { return raw_token.kind; }
    size_t line() const { return raw_token.line; }
    size_t column() const { return raw_token.column; }
    size_t position() const { return raw_token.position; }
    
    // Context queries
    bool is_resolved_as(const std::string& resolution) const {
        return context_resolution == resolution;
    }
    
    bool has_attribute(const std::string& key) const {
        return attributes.has(key);
    }
    
    std::string get_attribute(const std::string& key, const std::string& default_value = "") const {
        return attributes.get(key, default_value);
    }
    
    void set_attribute(const std::string& key, const std::string& value) {
        attributes.set(key, value);
    }
    
    // Utility methods (delegate to raw_token)
    bool is_keyword(const std::string& keyword) const {
        return raw_token.is_keyword() && value() == keyword;
    }
    
    bool is_identifier() const {
        return raw_token.kind == TokenKind::IDENTIFIER;
    }
    
    bool is_operator(const std::string& op) const {
        return raw_token.is_operator() && value() == op;
    }
    
    bool is_punctuation(const std::string& punct) const {
        return raw_token.is_operator() && value() == punct;
    }
    
    // Debug representation
    std::string to_string() const;
};

} // namespace cprime