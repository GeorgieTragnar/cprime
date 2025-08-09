#include "raw_token.h"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

// Include logging infrastructure
#include "../common/logger.h"
#include "../common/logger_components.h"

namespace cprime {

// Comprehensive keyword mapping - all CPrime reserved words
const std::unordered_map<std::string, TokenKind> RawTokenizer::keywords = {
    // Core language constructs
    {"class", TokenKind::CLASS}, {"struct", TokenKind::STRUCT}, {"union", TokenKind::UNION}, 
    {"interface", TokenKind::INTERFACE}, {"plex", TokenKind::PLEX},
    
    // Context-sensitive keywords
    {"runtime", TokenKind::RUNTIME}, {"defer", TokenKind::DEFER},
    
    // Control flow
    {"if", TokenKind::IF}, {"else", TokenKind::ELSE}, {"while", TokenKind::WHILE}, 
    {"for", TokenKind::FOR}, {"case", TokenKind::CASE}, {"switch", TokenKind::SWITCH}, {"default", TokenKind::DEFAULT},
    {"break", TokenKind::BREAK}, {"continue", TokenKind::CONTINUE}, {"return", TokenKind::RETURN}, {"goto", TokenKind::GOTO},
    
    // Exception handling
    {"throw", TokenKind::THROW}, {"try", TokenKind::TRY}, {"catch", TokenKind::CATCH},
    
    // Type system
    {"auto", TokenKind::AUTO}, {"void", TokenKind::VOID}, {"bool", TokenKind::BOOL}, 
    {"char", TokenKind::CHAR}, {"wchar_t", TokenKind::WCHAR_T},
    {"int", TokenKind::INT}, {"short", TokenKind::SHORT}, {"long", TokenKind::LONG}, 
    {"signed", TokenKind::SIGNED}, {"unsigned", TokenKind::UNSIGNED},
    {"float", TokenKind::FLOAT}, {"double", TokenKind::DOUBLE},
    {"int8_t", TokenKind::INT8_T}, {"int16_t", TokenKind::INT16_T}, {"int32_t", TokenKind::INT32_T}, {"int64_t", TokenKind::INT64_T},
    {"uint8_t", TokenKind::UINT8_T}, {"uint16_t", TokenKind::UINT16_T}, {"uint32_t", TokenKind::UINT32_T}, {"uint64_t", TokenKind::UINT64_T},
    {"char8_t", TokenKind::CHAR8_T}, {"char16_t", TokenKind::CHAR16_T}, {"char32_t", TokenKind::CHAR32_T},
    
    // Type qualifiers and storage
    {"const", TokenKind::CONST}, {"mut", TokenKind::MUT}, {"static", TokenKind::STATIC}, 
    {"extern", TokenKind::EXTERN}, {"register", TokenKind::REGISTER}, {"thread_local", TokenKind::THREAD_LOCAL}, {"volatile", TokenKind::VOLATILE},
    {"constexpr", TokenKind::CONSTEXPR}, {"consteval", TokenKind::CONSTEVAL}, {"constinit", TokenKind::CONSTINIT}, 
    {"noexcept", TokenKind::NOEXCEPT}, {"inline", TokenKind::INLINE},
    
    // Memory management
    {"new", TokenKind::NEW}, {"delete", TokenKind::DELETE}, {"danger", TokenKind::DANGER},
    
    // Access control
    {"public", TokenKind::PUBLIC}, {"private", TokenKind::PRIVATE}, {"protected", TokenKind::PROTECTED}, {"friend", TokenKind::FRIEND},
    
    // Metaprogramming
    {"sizeof", TokenKind::SIZEOF}, {"alignof", TokenKind::ALIGNOF}, {"alignas", TokenKind::ALIGNAS}, 
    {"decltype", TokenKind::DECLTYPE}, {"typeof", TokenKind::TYPEOF}, {"typeid", TokenKind::TYPEID},
    {"template", TokenKind::TEMPLATE}, {"typename", TokenKind::TYPENAME}, {"using", TokenKind::USING}, {"namespace", TokenKind::NAMESPACE},
    
    // Boolean and null literals
    {"true", TokenKind::TRUE_LITERAL}, {"false", TokenKind::FALSE_LITERAL}, {"nullptr", TokenKind::NULLPTR_LITERAL}
};

const std::unordered_map<std::string, TokenKind> RawTokenizer::symbols = {
    // Arithmetic operators
    {"+", TokenKind::PLUS}, {"-", TokenKind::MINUS}, {"*", TokenKind::MULTIPLY}, {"/", TokenKind::DIVIDE}, {"%", TokenKind::MODULO},
    
    // Assignment operators
    {"=", TokenKind::ASSIGN},
    {"+=", TokenKind::PLUS_ASSIGN}, {"-=", TokenKind::MINUS_ASSIGN}, {"*=", TokenKind::MULTIPLY_ASSIGN}, 
    {"/=", TokenKind::DIVIDE_ASSIGN}, {"%=", TokenKind::MODULO_ASSIGN},
    
    // Increment/decrement
    {"++", TokenKind::INCREMENT}, {"--", TokenKind::DECREMENT},
    
    // Comparison operators
    {"==", TokenKind::EQUAL_EQUAL}, {"!=", TokenKind::NOT_EQUAL}, 
    {"<", TokenKind::LESS_THAN}, {">", TokenKind::GREATER_THAN},
    {"<=", TokenKind::LESS_EQUAL}, {">=", TokenKind::GREATER_EQUAL},
    {"<=>", TokenKind::SPACESHIP}, // Three-way comparison
    
    // Logical operators
    {"&&", TokenKind::LOGICAL_AND}, {"||", TokenKind::LOGICAL_OR}, {"!", TokenKind::LOGICAL_NOT},
    
    // Bitwise operators
    {"&", TokenKind::BIT_AND}, {"|", TokenKind::BIT_OR}, {"^", TokenKind::BIT_XOR}, {"~", TokenKind::BIT_NOT},
    {"<<", TokenKind::LEFT_SHIFT}, {">>", TokenKind::RIGHT_SHIFT},
    {"&=", TokenKind::BIT_AND_ASSIGN}, {"|=", TokenKind::BIT_OR_ASSIGN}, {"^=", TokenKind::BIT_XOR_ASSIGN},
    {"<<=", TokenKind::LEFT_SHIFT_ASSIGN}, {">>=", TokenKind::RIGHT_SHIFT_ASSIGN},
    
    // Member access
    {".", TokenKind::DOT}, {"->", TokenKind::ARROW}, {"::", TokenKind::SCOPE_RESOLUTION},
    {".*", TokenKind::DOT_STAR}, {"->*", TokenKind::ARROW_STAR}, // Pointer-to-member operators
    
    // Punctuation
    {"(", TokenKind::LEFT_PAREN}, {")", TokenKind::RIGHT_PAREN},
    {"{", TokenKind::LEFT_BRACE}, {"}", TokenKind::RIGHT_BRACE},
    {"[", TokenKind::LEFT_BRACKET}, {"]", TokenKind::RIGHT_BRACKET},
    {";", TokenKind::SEMICOLON}, {",", TokenKind::COMMA}, {":", TokenKind::COLON},
    {"?", TokenKind::QUESTION}, {"...", TokenKind::ELLIPSIS}
};

const std::unordered_set<std::string> RawTokenizer::multi_char_symbols = {
    "==", "!=", "<=", ">=", "<=>",
    "&&", "||", "<<", ">>",
    "+=", "-=", "*=", "/=", "%=",
    "&=", "|=", "^=", "<<=", ">>=",
    "++", "--", // Increment/decrement
    "->", "::",
    "->*", ".*", // C++ pointer-to-member operators
    "..."
};

// RawToken implementation
std::string RawToken::to_string(const StringTable& string_table) const {
    std::stringstream ss;
    ss << "RawToken(";
    
    // Convert TokenKind to string
    ss << static_cast<int>(kind);
    
    if (has_string_value() && has_valid_string_index()) {
        ss << ", \"" << string_table.get_string(string_index) << "\"";
    } else if (has_literal_value()) {
        ss << ", literal_value";
    }
    
    ss << ", " << line << ":" << column << ")";
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
        static const RawToken eof_token(TokenKind::EOF_TOKEN, 0, 0, 0);
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
    return pos >= tokens.size() || tokens[pos].kind == TokenKind::EOF_TOKEN;
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
RawTokenizer::RawTokenizer(const std::string& source, StringTable& string_table)
    : source(source), pos(0), line(1), column(1), string_table_(string_table) {
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
        
        // Handle string and character literals  
        if (c == '"') {
            tokens.push_back(read_string_literal());
            continue;
        }
        
        if (c == '\'') {
            tokens.push_back(read_character_literal());
            continue;
        }
        
        // Handle numeric literals
        if (is_numeric(c)) {
            // Check if it looks like a floating point number
            if (peek_for_float_literal()) {
                tokens.push_back(read_float_literal());
            } else {
                tokens.push_back(read_integer_literal());
            }
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
                auto symbol_kind = symbols.at(three_char_symbol);
                tokens.emplace_back(symbol_kind, string_table_.intern(three_char_symbol), line, column, current_position());
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
                auto symbol_kind = symbols.at(potential_symbol);
                tokens.emplace_back(symbol_kind, string_table_.intern(potential_symbol), line, column, current_position());
                advance(); // First character
                advance(); // Second character
                continue;
            }
        }
        
        // Single character symbols
        trace_logger->trace("Checking single-char symbol: '{}'", c);
        if (symbols.count(std::string(1, c))) {
            trace_logger->trace("Found single-char symbol: '{}'", c);
            auto symbol_kind = symbols.at(std::string(1, c));
            tokens.emplace_back(symbol_kind, string_table_.intern(std::string(1, c)), line, column, current_position());
            advance();
            continue;
        }
        
        // Unknown character
        error("Unexpected character: '" + std::string(1, c) + "'");
    }
    
    // Add EOF token
    tokens.emplace_back(TokenKind::EOF_TOKEN, line, column, current_position());
    
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
    
    std::string full_comment = "//" + comment_text;
    return RawToken(TokenKind::COMMENT, string_table_.intern(full_comment), start_line, start_column, start_pos);
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
    
    std::string full_comment = "/*" + comment_text + "*/";
    return RawToken(TokenKind::COMMENT, string_table_.intern(full_comment), start_line, start_column, start_pos);
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
    
    // Check if it's a keyword first
    auto keyword_it = keywords.find(text);
    if (keyword_it != keywords.end()) {
        // Handle special literal keywords
        if (keyword_it->second == TokenKind::TRUE_LITERAL) {
            return RawToken(TokenKind::TRUE_LITERAL, true, start_line, start_column, start_pos);
        } else if (keyword_it->second == TokenKind::FALSE_LITERAL) {
            return RawToken(TokenKind::FALSE_LITERAL, false, start_line, start_column, start_pos);
        } else {
            return RawToken(keyword_it->second, string_table_.intern(text), start_line, start_column, start_pos);
        }
    }
    
    // Otherwise it's an identifier
    return RawToken(TokenKind::IDENTIFIER, string_table_.intern(text), start_line, start_column, start_pos);
}

RawToken RawTokenizer::read_string_literal() {
    size_t start_line = line;
    size_t start_column = column;
    size_t start_pos = current_position();
    
    // Check for string prefixes (L, u, U, u8, R)
    std::string prefix = "";
    
    // Back up to check for prefixes
    if (pos > 0) {
        size_t check_pos = pos - 1;
        // Look for single character prefixes (L, u, U)
        if (check_pos < source.length() && 
            (source[check_pos] == 'L' || source[check_pos] == 'u' || source[check_pos] == 'U')) {
            prefix = source[check_pos];
        }
        // Look for u8 prefix
        else if (check_pos >= 1 && check_pos < source.length() - 1 && 
                 source[check_pos-1] == 'u' && source[check_pos] == '8') {
            prefix = "u8";
        }
    }
    
    std::string text = "";
    advance(); // Skip opening quote
    
    while (!is_at_end() && peek() != '"') {
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
        advance(); // Skip closing quote
    } else {
        error("Unterminated string literal");
    }
    
    // Determine string literal type based on prefix
    TokenKind kind = determine_string_prefix(prefix);
    std::string full_text = prefix + "\"" + text + "\"";
    
    return RawToken(kind, string_table_.intern(full_text), start_line, start_column, start_pos);
}

RawToken RawTokenizer::read_integer_literal() {
    size_t start_line = line;
    size_t start_column = column;
    size_t start_pos = current_position();
    
    std::string text = "";
    
    // Read integer part
    while (!is_at_end() && is_numeric(peek())) {
        text += peek();
        advance();
    }
    
    // Read suffix (u, l, ll, ul, ull, etc.)
    std::string suffix = "";
    while (!is_at_end() && (peek() == 'u' || peek() == 'U' || peek() == 'l' || peek() == 'L')) {
        suffix += peek();
        advance();
    }
    
    // Determine integer literal type based on suffix
    TokenKind kind = determine_integer_suffix(suffix);
    std::string full_text = text + suffix;
    
    // Parse the actual integer value
    try {
        if (kind == TokenKind::UINT_LITERAL) {
            uint32_t value = static_cast<uint32_t>(std::stoull(text));
            return RawToken(kind, value, start_line, start_column, start_pos);
        } else if (kind == TokenKind::LONG_LITERAL || kind == TokenKind::LONG_LONG_LITERAL) {
            int64_t value = std::stoll(text);
            return RawToken(kind, value, start_line, start_column, start_pos);
        } else if (kind == TokenKind::ULONG_LITERAL || kind == TokenKind::ULONG_LONG_LITERAL) {
            uint64_t value = std::stoull(text);
            return RawToken(kind, value, start_line, start_column, start_pos);
        } else {
            // Default to int32_t
            int32_t value = std::stoi(text);
            return RawToken(TokenKind::INT_LITERAL, value, start_line, start_column, start_pos);
        }
    } catch (const std::exception&) {
        error("Invalid integer literal: " + full_text);
        return RawToken(TokenKind::INT_LITERAL, 0, start_line, start_column, start_pos);
    }
}

RawToken RawTokenizer::read_float_literal() {
    size_t start_line = line;
    size_t start_column = column;
    size_t start_pos = current_position();
    
    std::string text = "";
    
    // Read integer part
    while (!is_at_end() && is_numeric(peek())) {
        text += peek();
        advance();
    }
    
    // Read decimal point and fractional part
    if (!is_at_end() && peek() == '.') {
        text += peek();
        advance();
        
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
    
    // Read suffix (f, F, l, L)
    std::string suffix = "";
    if (!is_at_end() && (peek() == 'f' || peek() == 'F' || peek() == 'l' || peek() == 'L')) {
        suffix += peek();
        advance();
    }
    
    // Determine float literal type based on suffix
    TokenKind kind = determine_float_suffix(suffix);
    
    // Parse the actual float value
    try {
        if (kind == TokenKind::FLOAT_LITERAL) {
            float value = std::stof(text);
            return RawToken(kind, value, start_line, start_column, start_pos);
        } else if (kind == TokenKind::LONG_DOUBLE_LITERAL) {
            long double value = std::stold(text);
            return RawToken(kind, value, start_line, start_column, start_pos);
        } else {
            // Default to double
            double value = std::stod(text);
            return RawToken(TokenKind::DOUBLE_LITERAL, value, start_line, start_column, start_pos);
        }
    } catch (const std::exception&) {
        error("Invalid float literal: " + text + suffix);
        return RawToken(TokenKind::DOUBLE_LITERAL, 0.0, start_line, start_column, start_pos);
    }
}

RawToken RawTokenizer::read_character_literal() {
    size_t start_line = line;
    size_t start_column = column;
    size_t start_pos = current_position();
    
    // Check for character prefixes (L, u, U, u8)
    std::string prefix = "";
    if (pos > 0) {
        size_t check_pos = pos - 1;
        if (check_pos < source.length() && 
            (source[check_pos] == 'L' || source[check_pos] == 'u' || source[check_pos] == 'U')) {
            prefix = source[check_pos];
        }
        // Check for u8 prefix
        else if (check_pos >= 1 && source[check_pos-1] == 'u' && source[check_pos] == '8') {
            prefix = "u8";
        }
    }
    
    advance(); // Skip opening single quote
    
    char character_value = 0;
    if (!is_at_end()) {
        if (peek() == '\\') {
            // Handle escape sequences
            advance();
            if (!is_at_end()) {
                char escape_char = peek();
                switch (escape_char) {
                    case 'n': character_value = '\n'; break;
                    case 't': character_value = '\t'; break;
                    case 'r': character_value = '\r'; break;
                    case '\\': character_value = '\\'; break;
                    case '\'': character_value = '\''; break;
                    case '"': character_value = '"'; break;
                    case '0': character_value = '\0'; break;
                    default: character_value = escape_char; break;
                }
                advance();
            }
        } else {
            character_value = peek();
            advance();
        }
    }
    
    if (!is_at_end() && peek() == '\'') {
        advance(); // Skip closing single quote
    } else {
        error("Unterminated character literal");
    }
    
    // Determine character literal type based on prefix
    TokenKind kind = determine_character_prefix(prefix);
    
    // Return appropriate character type
    if (kind == TokenKind::WCHAR_LITERAL) {
        return RawToken(kind, static_cast<wchar_t>(character_value), start_line, start_column, start_pos);
    } else if (kind == TokenKind::CHAR16_LITERAL) {
        return RawToken(kind, static_cast<char16_t>(character_value), start_line, start_column, start_pos);
    } else if (kind == TokenKind::CHAR32_LITERAL) {
        return RawToken(kind, static_cast<char32_t>(character_value), start_line, start_column, start_pos);
    } else {
        return RawToken(TokenKind::CHAR_LITERAL, character_value, start_line, start_column, start_pos);
    }
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

bool RawTokenizer::peek_for_float_literal() const {
    // Look ahead to see if this looks like a floating point number
    size_t check_pos = pos;
    
    // Skip initial digits
    while (check_pos < source.length() && is_numeric(source[check_pos])) {
        check_pos++;
    }
    
    // Check for decimal point followed by digit
    if (check_pos < source.length() - 1 && source[check_pos] == '.' && 
        is_numeric(source[check_pos + 1])) {
        return true;
    }
    
    // Check for scientific notation (e/E)
    if (check_pos < source.length() && 
        (source[check_pos] == 'e' || source[check_pos] == 'E')) {
        return true;
    }
    
    return false;
}

TokenKind RawTokenizer::determine_integer_suffix(const std::string& suffix) const {
    if (suffix.empty()) {
        return TokenKind::INT_LITERAL;
    }
    
    // Convert to lowercase for comparison
    std::string lower_suffix = suffix;
    std::transform(lower_suffix.begin(), lower_suffix.end(), lower_suffix.begin(), ::tolower);
    
    if (lower_suffix == "u") {
        return TokenKind::UINT_LITERAL;
    } else if (lower_suffix == "l") {
        return TokenKind::LONG_LITERAL;
    } else if (lower_suffix == "ll") {
        return TokenKind::LONG_LONG_LITERAL;
    } else if (lower_suffix == "ul" || lower_suffix == "lu") {
        return TokenKind::ULONG_LITERAL;
    } else if (lower_suffix == "ull" || lower_suffix == "llu") {
        return TokenKind::ULONG_LONG_LITERAL;
    }
    
    return TokenKind::INT_LITERAL;
}

TokenKind RawTokenizer::determine_float_suffix(const std::string& suffix) const {
    if (suffix.empty()) {
        return TokenKind::DOUBLE_LITERAL;
    }
    
    if (suffix == "f" || suffix == "F") {
        return TokenKind::FLOAT_LITERAL;
    } else if (suffix == "l" || suffix == "L") {
        return TokenKind::LONG_DOUBLE_LITERAL;
    }
    
    return TokenKind::DOUBLE_LITERAL;
}

TokenKind RawTokenizer::determine_string_prefix(const std::string& prefix) const {
    if (prefix.empty()) {
        return TokenKind::STRING_LITERAL;
    }
    
    if (prefix == "L") {
        return TokenKind::WSTRING_LITERAL;
    } else if (prefix == "u") {
        return TokenKind::STRING16_LITERAL;
    } else if (prefix == "U") {
        return TokenKind::STRING32_LITERAL;
    } else if (prefix == "u8") {
        return TokenKind::STRING8_LITERAL;
    } else if (prefix == "R") {
        return TokenKind::RAW_STRING_LITERAL;
    }
    
    return TokenKind::STRING_LITERAL;
}

TokenKind RawTokenizer::determine_character_prefix(const std::string& prefix) const {
    if (prefix.empty()) {
        return TokenKind::CHAR_LITERAL;
    }
    
    if (prefix == "L") {
        return TokenKind::WCHAR_LITERAL;
    } else if (prefix == "u") {
        return TokenKind::CHAR16_LITERAL;
    } else if (prefix == "U") {
        return TokenKind::CHAR32_LITERAL;
    } else if (prefix == "u8") {
        // u8 character literals require C++20, treat as regular char for C++17
        return TokenKind::CHAR_LITERAL;
    }
    
    return TokenKind::CHAR_LITERAL;
}

bool RawTokenizer::ends_with_ci(const std::string& str, const std::string& suffix) const {
    if (str.length() < suffix.length()) {
        return false;
    }
    
    std::string str_end = str.substr(str.length() - suffix.length());
    std::transform(str_end.begin(), str_end.end(), str_end.begin(), ::tolower);
    
    std::string suffix_lower = suffix;
    std::transform(suffix_lower.begin(), suffix_lower.end(), suffix_lower.begin(), ::tolower);
    
    return str_end == suffix_lower;
}

bool RawTokenizer::starts_with(const std::string& str, const std::string& prefix) const {
    return str.length() >= prefix.length() && 
           str.substr(0, prefix.length()) == prefix;
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