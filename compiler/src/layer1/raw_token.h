#pragma once

#include "../common/tokens.h"
#include "../common/token_streams.h"
#include "../common/token_utils.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>

// Forward declare logger to avoid circular includes
namespace spdlog { class logger; }

namespace cprime {

// TokenKind, RawToken, and RawTokenStream are now defined in common/ headers

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