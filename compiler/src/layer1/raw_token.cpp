#include "raw_token.h"
#include <stdexcept>
#include <sstream>
#include <iostream>

// Include logging infrastructure
#include "../common/logger.h"
#include "../common/logger_components.h"

namespace cprime {

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

const std::unordered_set<std::string> RawTokenizer::symbols = {
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
    
    // Structural punctuation
    "{", "}", "(", ")", "[", "]",
    ";", ",", ":", "?",
    
    // Note: Quote characters (', ") handled specially in string literal parsing
};

const std::unordered_set<std::string> RawTokenizer::multi_char_symbols = {
    "==", "!=", "<=", ">=", "<=>",
    "&&", "||", "<<", ">>",
    "+=", "-=", "*=", "/=", "%=",
    "&=", "|=", "^=", "<<=", ">>=",
    "++", "--", // Increment/decrement
    "->", "::",
    "->*", ".*" // C++ pointer-to-member operators
};

// RawToken implementation
std::string RawToken::to_string() const {
    std::stringstream ss;
    ss << "RawToken(";
    
    switch (type) {
        case RawTokenType::KEYWORD: ss << "KEYWORD"; break;
        case RawTokenType::IDENTIFIER: ss << "IDENTIFIER"; break;
        case RawTokenType::SYMBOL: ss << "SYMBOL"; break;
        case RawTokenType::LITERAL: ss << "LITERAL"; break;
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
    : source(source), pos(0), line(1), column(1) {
    // Initialize trace logger for detailed debugging
    trace_logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_TOKENIZER);
    trace_logger->set_level(spdlog::level::trace);
}

std::vector<RawToken> RawTokenizer::tokenize() {
    // Start trace logging with buffer
    CPRIME_BUFFER_BEGIN_TRACE(CPRIME_COMPONENT_TOKENIZER);
    trace_logger->trace("=== TOKENIZATION START ===");
    trace_logger->trace("Source length: {} characters", source.length());
    trace_logger->trace("Source content: '{}'", source);
    
    std::vector<RawToken> tokens;
    
    try {
    
    while (!is_at_end()) {
        char c = peek();
        trace_logger->trace("Main loop: pos={}, line={}, col={}, char='{}' (code={})", 
                          pos, line, column, (c >= 32 && c <= 126) ? std::string(1, c) : "\\x" + std::to_string(static_cast<unsigned char>(c)), static_cast<int>(c));
        
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
        
        // Handle symbols (unified operator-or-punctuator, check longer multi-char first)
        trace_logger->trace("Attempting symbol parsing for char: '{}'", c);
        std::string potential_symbol = "";
        potential_symbol += c;
        
        // Check for 3-character symbols first
        if (!is_at_end() && pos + 1 < source.length() && pos + 2 < source.length()) {
            std::string three_char_symbol = potential_symbol + peek_next() + source[pos + 2];
            trace_logger->trace("Checking 3-char symbol: '{}'", three_char_symbol);
            if (multi_char_symbols.count(three_char_symbol)) {
                trace_logger->trace("Found 3-char symbol: '{}'", three_char_symbol);
                tokens.emplace_back(RawTokenType::SYMBOL, three_char_symbol, line, column, current_position());
                advance(); // First character
                advance(); // Second character  
                advance(); // Third character
                continue;
            }
        }
        
        // Check for 2-character symbols
        if (!is_at_end() && pos + 1 < source.length()) {
            potential_symbol += peek_next();
            trace_logger->trace("Checking 2-char symbol: '{}'", potential_symbol);
            if (multi_char_symbols.count(potential_symbol)) {
                trace_logger->trace("Found 2-char symbol: '{}'", potential_symbol);
                tokens.emplace_back(RawTokenType::SYMBOL, potential_symbol, line, column, current_position());
                advance(); // First character
                advance(); // Second character
                continue;
            }
        }
        
        // Single character symbols
        trace_logger->trace("Checking single-char symbol: '{}'", c);
        if (symbols.count(std::string(1, c))) {
            trace_logger->trace("Found single-char symbol: '{}'", c);
            tokens.emplace_back(RawTokenType::SYMBOL, std::string(1, c), line, column, current_position());
            advance();
            continue;
        }
        
        // Unknown character
        error("Unexpected character: '" + std::string(1, c) + "'");
    }
    
    // Add EOF token
    tokens.emplace_back(RawTokenType::EOF_TOKEN, "", line, column, current_position());
    
    // Success - clear trace buffer and return
    trace_logger->trace("=== TOKENIZATION SUCCESS ===");
    trace_logger->trace("Total tokens created: {}", tokens.size());
    CPRIME_BUFFER_CLEAR(CPRIME_COMPONENT_TOKENIZER);
    
    return tokens;
    
    } catch (const std::exception& e) {
        // Error occurred - dump all trace logs for debugging
        trace_logger->error("=== TOKENIZATION FAILED ===");
        trace_logger->error("Error: {}", e.what());
        trace_logger->error("Position: {}:{} (absolute: {})", line, column, pos);
        if (pos < source.length()) {
            trace_logger->error("Current character: '{}' (code: {})", source[pos], static_cast<int>(source[pos]));
        }
        CPRIME_BUFFER_DUMP(CPRIME_COMPONENT_TOKENIZER);
        CPRIME_BUFFER_CLEAR(CPRIME_COMPONENT_TOKENIZER);
        throw; // Re-throw the original exception
    }
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
        char current_char = source[pos];
        trace_logger->trace("Advancing: consuming '{}' (code={}) at pos={}", 
                          (current_char >= 32 && current_char <= 126) ? std::string(1, current_char) : "\\x" + std::to_string(static_cast<unsigned char>(current_char)), 
                          static_cast<int>(current_char), pos);
        update_position(current_char);
        pos++;
        trace_logger->trace("After advance: pos={}, line={}, col={}", pos, line, column);
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


} // namespace cprime