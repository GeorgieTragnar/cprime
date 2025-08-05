#include "raw_token.h"
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace cprime::v2 {

// Static keyword definitions - context-sensitive keywords handled in semantic layer
const std::unordered_set<std::string> RawTokenizer::keywords = {
    // Core language keywords
    "class", "plex", "struct", "union", "interface",
    "runtime", "defer",
    
    // Control flow
    "if", "else", "while", "for", "case",
    "break", "continue", "return",
    
    // Exception handling
    "throw", "try", "catch",
    
    // Types and modifiers
    "auto", "void", "bool", "int", "float",
    "const", "mut", "static", "extern",
    "constexpr", "consteval", "constinit", "noexcept",
    
    // Special values
    "true", "false", "nullptr",
    
    // Memory management
    "new", "delete", "danger",
    
    // Visibility
    "public", "private",
    
    // Metaprogramming
    "sizeof", "alignof", "decltype"
};

const std::unordered_set<std::string> RawTokenizer::operators = {
    // Arithmetic
    "+", "-", "*", "/", "%",
    "+=", "-=", "*=", "/=", "%=",
    "++", "--", // Increment/decrement
    
    // Comparison
    "==", "!=", "<", ">", "<=", ">=",
    "<=>", // Three-way comparison
    
    // Logical
    "&&", "||", "!", "^",
    
    // Bitwise
    "&", "|", "<<", ">>", "~",
    "&=", "|=", "^=", "<<=", ">>=",
    
    // Pointer and member access
    ".", "->", "::",
    "->*", ".*", // C++ pointer-to-member operators
    
    // Assignment
    "=",
    
    // Conditional and comma operators
    "?", ":", "," // Also in punctuation - context determines usage
};

const std::unordered_set<std::string> RawTokenizer::multi_char_operators = {
    "==", "!=", "<=", ">=", "<=>",
    "&&", "||", "<<", ">>",
    "+=", "-=", "*=", "/=", "%=",
    "&=", "|=", "^=", "<<=", ">>=",
    "++", "--", // Increment/decrement
    "->", "::",
    "->*", ".*" // C++ pointer-to-member operators
};

const std::unordered_set<char> RawTokenizer::single_char_punctuation = {
    '{', '}', '(', ')', '[', ']',
    ';', ',', ':', '?',
    '`', '\'', '"' // Quote characters handled specially
};

// RawToken implementation
std::string RawToken::to_string() const {
    std::stringstream ss;
    ss << "RawToken(";
    
    switch (type) {
        case RawTokenType::KEYWORD: ss << "KEYWORD"; break;
        case RawTokenType::IDENTIFIER: ss << "IDENTIFIER"; break;
        case RawTokenType::OPERATOR: ss << "OPERATOR"; break;
        case RawTokenType::LITERAL: ss << "LITERAL"; break;
        case RawTokenType::PUNCTUATION: ss << "PUNCTUATION"; break;
        case RawTokenType::WHITESPACE: ss << "WHITESPACE"; break;
        case RawTokenType::COMMENT: ss << "COMMENT"; break;
        case RawTokenType::EOF_TOKEN: ss << "EOF"; break;
    }
    
    ss << ", \"" << value << "\", " << line << ":" << column << ")";
    return ss.str();
}

// RawTokenStream implementation
RawTokenStream::RawTokenStream(std::vector<RawToken> tokens)
    : tokens(std::move(tokens)), pos(0) {}

const RawToken& RawTokenStream::current() const {
    ensure_valid_position();
    return tokens[pos];
}

const RawToken& RawTokenStream::peek(size_t offset) const {
    size_t peek_pos = pos + offset;
    if (peek_pos >= tokens.size()) {
        // Return EOF token if peeking beyond end
        static const RawToken eof_token(RawTokenType::EOF_TOKEN, "", 0, 0, 0);
        return eof_token;
    }
    return tokens[peek_pos];
}

const RawToken& RawTokenStream::previous() const {
    if (pos == 0) {
        throw std::runtime_error("Cannot access previous token at beginning of stream");
    }
    return tokens[pos - 1];
}

void RawTokenStream::advance() {
    if (pos < tokens.size()) {
        pos++;
    }
}

void RawTokenStream::rewind() {
    if (pos > 0) {
        pos--;
    }
}

bool RawTokenStream::is_at_end() const {
    return pos >= tokens.size() || tokens[pos].type == RawTokenType::EOF_TOKEN;
}

void RawTokenStream::set_position(size_t new_pos) {
    if (new_pos > tokens.size()) {
        throw std::runtime_error("Invalid token stream position");
    }
    pos = new_pos;
}

void RawTokenStream::ensure_valid_position() const {
    if (pos >= tokens.size()) {
        throw std::runtime_error("Token stream position out of bounds");
    }
}

// RawTokenizer implementation
RawTokenizer::RawTokenizer(const std::string& source)
    : source(source), pos(0), line(1), column(1) {}

std::vector<RawToken> RawTokenizer::tokenize() {
    std::vector<RawToken> tokens;
    
    while (!is_at_end()) {
        char c = peek();
        
        // Skip whitespace (but preserve for formatting in some contexts)
        if (is_whitespace(c)) {
            skip_whitespace();
            continue;
        }
        
        // Handle comments
        if (c == '/' && peek_next() == '/') {
            tokens.push_back(read_line_comment());
            continue;
        }
        
        if (c == '/' && peek_next() == '*') {
            tokens.push_back(read_block_comment());
            continue;
        }
        
        // Handle string literals
        if (c == '"' || c == '\'') {
            tokens.push_back(read_string_literal());
            continue;
        }
        
        // Handle numeric literals
        if (is_numeric(c)) {
            tokens.push_back(read_number_literal());
            continue;
        }
        
        // Handle identifiers and keywords
        if (is_alpha(c) || c == '_') {
            tokens.push_back(read_identifier_or_keyword());
            continue;
        }
        
        // Handle operators (check longer multi-char first)
        std::string potential_operator = "";
        potential_operator += c;
        
        // Check for 3-character operators first
        if (!is_at_end() && pos + 1 < source.length() && pos + 2 < source.length()) {
            std::string three_char_op = potential_operator + peek_next() + source[pos + 2];
            if (multi_char_operators.count(three_char_op)) {
                tokens.emplace_back(RawTokenType::OPERATOR, three_char_op, line, column, current_position());
                advance(); // First character
                advance(); // Second character  
                advance(); // Third character
                continue;
            }
        }
        
        // Check for 2-character operators
        if (!is_at_end() && pos + 1 < source.length()) {
            potential_operator += peek_next();
            if (multi_char_operators.count(potential_operator)) {
                tokens.emplace_back(RawTokenType::OPERATOR, potential_operator, line, column, current_position());
                advance(); // First character
                advance(); // Second character
                continue;
            }
        }
        
        // Single character operators
        if (operators.count(std::string(1, c))) {
            tokens.emplace_back(RawTokenType::OPERATOR, std::string(1, c), line, column, current_position());
            advance();
            continue;
        }
        
        // Handle punctuation
        if (single_char_punctuation.count(c)) {
            tokens.emplace_back(RawTokenType::PUNCTUATION, std::string(1, c), line, column, current_position());
            advance();
            continue;
        }
        
        // Unknown character
        error("Unexpected character: '" + std::string(1, c) + "'");
    }
    
    // Add EOF token
    tokens.emplace_back(RawTokenType::EOF_TOKEN, "", line, column, current_position());
    
    return tokens;
}

RawTokenStream RawTokenizer::tokenize_to_stream() {
    return RawTokenStream(tokenize());
}

char RawTokenizer::peek() const {
    if (is_at_end()) return '\0';
    return source[pos];
}

char RawTokenizer::peek_next() const {
    if (pos + 1 >= source.length()) return '\0';
    return source[pos + 1];
}

void RawTokenizer::advance() {
    if (!is_at_end()) {
        update_position(source[pos]);
        pos++;
    }
}

bool RawTokenizer::is_at_end() const {
    return pos >= source.length();
}

void RawTokenizer::skip_whitespace() {
    while (!is_at_end() && is_whitespace(peek())) {
        advance();
    }
}

RawToken RawTokenizer::read_line_comment() {
    size_t start_line = line;
    size_t start_column = column;
    size_t start_pos = current_position();
    
    std::string comment_text = "";
    
    // Skip the //
    advance();
    advance();
    
    // Read until end of line
    while (!is_at_end() && peek() != '\n') {
        comment_text += peek();
        advance();
    }
    
    return RawToken(RawTokenType::COMMENT, "//" + comment_text, start_line, start_column, start_pos);
}

RawToken RawTokenizer::read_block_comment() {
    size_t start_line = line;
    size_t start_column = column;
    size_t start_pos = current_position();
    
    std::string comment_text = "";
    
    // Skip the /*
    advance();
    advance();
    
    // Read until */
    while (!is_at_end()) {
        if (peek() == '*' && peek_next() == '/') {
            advance(); // Skip *
            advance(); // Skip /
            break;
        }
        comment_text += peek();
        advance();
    }
    
    return RawToken(RawTokenType::COMMENT, "/*" + comment_text + "*/", start_line, start_column, start_pos);
}

RawToken RawTokenizer::read_identifier_or_keyword() {
    size_t start_line = line;
    size_t start_column = column;
    size_t start_pos = current_position();
    
    std::string text = "";
    
    while (!is_at_end() && (is_alphanumeric(peek()) || peek() == '_')) {
        text += peek();
        advance();
    }
    
    // Check if it's a keyword
    RawTokenType type = keywords.count(text) ? RawTokenType::KEYWORD : RawTokenType::IDENTIFIER;
    
    return RawToken(type, text, start_line, start_column, start_pos);
}

RawToken RawTokenizer::read_string_literal() {
    size_t start_line = line;
    size_t start_column = column;
    size_t start_pos = current_position();
    
    char quote_char = peek();
    std::string text = "";
    text += quote_char;
    
    advance(); // Skip opening quote
    
    while (!is_at_end() && peek() != quote_char) {
        if (peek() == '\\') {
            text += peek();
            advance();
            if (!is_at_end()) {
                text += peek();
                advance();
            }
        } else {
            text += peek();
            advance();
        }
    }
    
    if (!is_at_end()) {
        text += peek(); // Closing quote
        advance();
    } else {
        error("Unterminated string literal");
    }
    
    return RawToken(RawTokenType::LITERAL, text, start_line, start_column, start_pos);
}

RawToken RawTokenizer::read_number_literal() {
    size_t start_line = line;
    size_t start_column = column;
    size_t start_pos = current_position();
    
    std::string text = "";
    
    // Read integer part
    while (!is_at_end() && is_numeric(peek())) {
        text += peek();
        advance();
    }
    
    // Check for decimal point
    if (!is_at_end() && peek() == '.' && pos + 1 < source.length() && is_numeric(source[pos + 1])) {
        text += peek();
        advance();
        
        // Read fractional part
        while (!is_at_end() && is_numeric(peek())) {
            text += peek();
            advance();
        }
    }
    
    // Check for scientific notation
    if (!is_at_end() && (peek() == 'e' || peek() == 'E')) {
        text += peek();
        advance();
        
        if (!is_at_end() && (peek() == '+' || peek() == '-')) {
            text += peek();
            advance();
        }
        
        while (!is_at_end() && is_numeric(peek())) {
            text += peek();
            advance();
        }
    }
    
    return RawToken(RawTokenType::LITERAL, text, start_line, start_column, start_pos);
}

bool RawTokenizer::is_alpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool RawTokenizer::is_numeric(char c) const {
    return c >= '0' && c <= '9';
}

bool RawTokenizer::is_alphanumeric(char c) const {
    return is_alpha(c) || is_numeric(c);
}

bool RawTokenizer::is_whitespace(char c) const {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

void RawTokenizer::error(const std::string& message) const {
    throw std::runtime_error("Tokenization error at line " + std::to_string(line) + 
                            ", column " + std::to_string(column) + ": " + message);
}

void RawTokenizer::update_position(char c) {
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
}

size_t RawTokenizer::current_position() const {
    return pos;
}

// ContextualToken implementation
std::string ContextualToken::to_string() const {
    std::stringstream ss;
    ss << "ContextualToken(";
    
    // Raw token info
    switch (raw_token.type) {
        case RawTokenType::KEYWORD: ss << "KEYWORD"; break;
        case RawTokenType::IDENTIFIER: ss << "IDENTIFIER"; break;
        case RawTokenType::OPERATOR: ss << "OPERATOR"; break;
        case RawTokenType::LITERAL: ss << "LITERAL"; break;
        case RawTokenType::PUNCTUATION: ss << "PUNCTUATION"; break;
        case RawTokenType::WHITESPACE: ss << "WHITESPACE"; break;
        case RawTokenType::COMMENT: ss << "COMMENT"; break;
        case RawTokenType::EOF_TOKEN: ss << "EOF"; break;
    }
    
    ss << ", \"" << raw_token.value << "\", " << raw_token.line << ":" << raw_token.column;
    
    // Context info
    if (!context_resolution.empty()) {
        ss << ", resolution=\"" << context_resolution << "\"";
    }
    
    if (!attributes.empty()) {
        ss << ", attrs={";
        bool first = true;
        for (const auto& [key, value] : attributes.data) {
            if (!first) ss << ", ";
            ss << key << "=\"" << value << "\"";
            first = false;
        }
        ss << "}";
    }
    
    ss << ")";
    return ss.str();
}

// ContextualTokenStream implementation
ContextualTokenStream::ContextualTokenStream(std::vector<ContextualToken> tokens)
    : tokens(std::move(tokens)), pos(0) {}

const ContextualToken& ContextualTokenStream::current() const {
    ensure_valid_position();
    return tokens[pos];
}

const ContextualToken& ContextualTokenStream::peek(size_t offset) const {
    size_t peek_pos = pos + offset;
    if (peek_pos >= tokens.size()) {
        // Return EOF-like token if peeking beyond end
        static const RawToken eof_raw(RawTokenType::EOF_TOKEN, "", 0, 0, 0);
        static const ContextualToken eof_contextual(eof_raw, static_cast<ParseContextType>(0));
        return eof_contextual;
    }
    return tokens[peek_pos];
}

const ContextualToken& ContextualTokenStream::previous() const {
    if (pos == 0) {
        throw std::runtime_error("Cannot access previous token at beginning of stream");
    }
    return tokens[pos - 1];
}

void ContextualTokenStream::advance() {
    if (pos < tokens.size()) {
        pos++;
    }
}

void ContextualTokenStream::rewind() {
    if (pos > 0) {
        pos--;
    }
}

bool ContextualTokenStream::is_at_end() const {
    return pos >= tokens.size();
}

void ContextualTokenStream::set_position(size_t new_pos) {
    if (new_pos > tokens.size()) {
        throw std::runtime_error("Invalid token stream position");
    }
    pos = new_pos;
}

std::vector<ContextualToken> ContextualTokenStream::filter_by_resolution(const std::string& resolution) const {
    std::vector<ContextualToken> result;
    for (const auto& token : tokens) {
        if (token.is_resolved_as(resolution)) {
            result.push_back(token);
        }
    }
    return result;
}

std::vector<ContextualToken> ContextualTokenStream::filter_by_context(ParseContextType context) const {
    std::vector<ContextualToken> result;
    for (const auto& token : tokens) {
        if (token.current_context == context) {
            result.push_back(token);
        }
    }
    return result;
}

size_t ContextualTokenStream::count_by_resolution(const std::string& resolution) const {
    size_t count = 0;
    for (const auto& token : tokens) {
        if (token.is_resolved_as(resolution)) {
            count++;
        }
    }
    return count;
}

void ContextualTokenStream::ensure_valid_position() const {
    if (pos >= tokens.size()) {
        throw std::runtime_error("Token stream position out of bounds");
    }
}

} // namespace cprime::v2