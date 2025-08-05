#pragma once

#include <string>
#include <vector>
#include <unordered_set>

namespace cprime::v2 {

/**
 * Raw token types for the new three-layer compiler architecture.
 * These represent pure syntactic tokens without semantic interpretation.
 */
enum class RawTokenType {
    // Keywords (context-sensitive interpretation happens in semantic layer)
    KEYWORD,        // "runtime", "defer", "exposes", "class", "union", etc.
    IDENTIFIER,     // Variable names, type names, function names
    OPERATOR,       // "::,", "<", ">", "=", "+", "-", etc.
    LITERAL,        // Numbers, strings, booleans
    PUNCTUATION,    // "{", "}", "(", ")", ";", ",", etc.
    
    // Meta tokens
    WHITESPACE,     // Spaces, tabs, newlines (preserved for formatting)
    COMMENT,        // Line and block comments
    EOF_TOKEN,      // End of file marker
};

/**
 * Raw token structure - pure syntactic information without semantic meaning.
 * This is the output of Layer 1 (Raw Token Parser).
 */
struct RawToken {
    RawTokenType type;
    std::string value;          // Literal text from source
    size_t line;               // Line number for error reporting
    size_t column;             // Column number for error reporting
    size_t position;           // Absolute position in source for precise mapping
    
    RawToken(RawTokenType type, const std::string& value, size_t line, size_t column, size_t position)
        : type(type), value(value), line(line), column(column), position(position) {}
    
    // Utility methods
    bool is_keyword(const std::string& keyword) const {
        return type == RawTokenType::KEYWORD && value == keyword;
    }
    
    bool is_identifier() const {
        return type == RawTokenType::IDENTIFIER;
    }
    
    bool is_operator(const std::string& op) const {
        return type == RawTokenType::OPERATOR && value == op;
    }
    
    bool is_punctuation(const std::string& punct) const {
        return type == RawTokenType::PUNCTUATION && value == punct;
    }
    
    // Debug string representation
    std::string to_string() const;
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
 */
class RawTokenizer {
public:
    explicit RawTokenizer(const std::string& source);
    
    // Main tokenization method
    std::vector<RawToken> tokenize();
    
    // Get result as stream for convenient processing
    RawTokenStream tokenize_to_stream();
    
private:
    std::string source;
    size_t pos;
    size_t line;
    size_t column;
    
    // Tokenization state
    static const std::unordered_set<std::string> keywords;
    static const std::unordered_set<std::string> operators;
    static const std::unordered_set<std::string> multi_char_operators;
    static const std::unordered_set<char> single_char_punctuation;
    
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
    RawToken read_number_literal();
    RawToken read_operator();
    RawToken read_punctuation();
    
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

} // namespace cprime::v2