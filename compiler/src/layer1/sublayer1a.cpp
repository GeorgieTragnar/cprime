#include "layer1.h"
#include <cctype>
#include <cstdio>

namespace cprime {
namespace layer1_sublayers {

// Helper function to create RawToken
static RawToken create_raw_token(EToken token, ERawToken raw_token, uint32_t line, uint32_t column, uint32_t position) {
    RawToken result;
    result._token = token;
    result._raw_token = raw_token;
    result._line = line;
    result._column = column;
    result._position = position;
    result._literal_value = std::monostate{};
    return result;
}

// Layer 1A: State machine for unambiguous single-character tokens
std::vector<ProcessingChunk> sublayer1a(std::stringstream& stream) {
    std::string source = stream.str();
    std::vector<ProcessingChunk> chunks;
    
    enum class State {
        NORMAL,
        IN_LINE_COMMENT,
        IN_BLOCK_COMMENT,
        IN_STRING,
        IN_CHAR
    };
    
    State state = State::NORMAL;
    uint32_t line = 1, column = 1, position = 0;
    uint32_t chunk_start_pos = 0;
    uint32_t chunk_start_line = 1, chunk_start_column = 1;
    std::string current_chunk;
    bool need_start_new_chunk = false;
    
    auto add_string_chunk = [&]() {
        if (!current_chunk.empty()) {
            chunks.emplace_back(std::move(current_chunk), chunk_start_pos, position, chunk_start_line, chunk_start_column);
            current_chunk.clear();
        }
    };
    
    auto add_token_chunk = [&](EToken token, ERawToken raw_token) {
        add_string_chunk();
        RawToken raw = create_raw_token(token, raw_token, line, column, position);
        chunks.emplace_back(std::move(raw), position, position + 1, line, column);
    };
    
    auto start_new_chunk = [&]() {
        chunk_start_pos = position;
        chunk_start_line = line; 
        chunk_start_column = column;
    };
    
    start_new_chunk();
    
    while (position < source.size()) {
        char c = source[position];
        char next_c = (position + 1 < source.size()) ? source[position + 1] : '\0';
        
        switch (state) {
            case State::NORMAL:
                // Check for comment start
                if (c == '/' && next_c == '/') {
                    state = State::IN_LINE_COMMENT;
                    current_chunk += c;
                    position++; column++;
                    current_chunk += source[position];
                    position++; column++;
                    continue;
                }
                if (c == '/' && next_c == '*') {
                    state = State::IN_BLOCK_COMMENT;
                    current_chunk += c;
                    position++; column++;
                    current_chunk += source[position];
                    position++; column++;
                    continue;
                }
                
                // Check for string/char boundaries
                if (c == '"') {
                    state = State::IN_STRING;
                    current_chunk += c;
                    break;
                }
                if (c == '\'') {
                    state = State::IN_CHAR;
                    current_chunk += c;
                    break;
                }
                
                // Process unambiguous single-character tokens
                switch (c) {
                    case ' ':
                        add_token_chunk(EToken::SPACE, ERawToken::WHITESPACE);
                        need_start_new_chunk = true;
                        break;
                    case '\t':
                        add_token_chunk(EToken::TAB, ERawToken::WHITESPACE);
                        need_start_new_chunk = true;
                        break;
                    case '\r':
                        add_token_chunk(EToken::CARRIAGE_RETURN, ERawToken::WHITESPACE);
                        need_start_new_chunk = true;
                        break;
                    case '\v':
                        add_token_chunk(EToken::VERTICAL_TAB, ERawToken::WHITESPACE);
                        need_start_new_chunk = true;
                        break;
                    case '\f':
                        add_token_chunk(EToken::FORM_FEED, ERawToken::WHITESPACE);
                        need_start_new_chunk = true;
                        break;
                    case '\n':
                        add_token_chunk(EToken::NEWLINE, ERawToken::NEWLINE);
                        need_start_new_chunk = true;
                        line++; column = 0; // Will be incremented at end of loop
                        break;
                    case '{':
                        add_token_chunk(EToken::LEFT_BRACE, ERawToken::LEFT_BRACE);
                        need_start_new_chunk = true;
                        break;
                    case '}':
                        add_token_chunk(EToken::RIGHT_BRACE, ERawToken::RIGHT_BRACE);
                        need_start_new_chunk = true;
                        break;
                    case ';':
                        add_token_chunk(EToken::SEMICOLON, ERawToken::SEMICOLON);
                        need_start_new_chunk = true;
                        break;
                    case '(':
                        add_token_chunk(EToken::LEFT_PAREN, ERawToken::KEYWORD);
                        need_start_new_chunk = true;
                        break;
                    case ')':
                        add_token_chunk(EToken::RIGHT_PAREN, ERawToken::KEYWORD);
                        need_start_new_chunk = true;
                        break;
                    case '[':
                        add_token_chunk(EToken::LEFT_BRACKET, ERawToken::KEYWORD);
                        need_start_new_chunk = true;
                        break;
                    case ']':
                        add_token_chunk(EToken::RIGHT_BRACKET, ERawToken::KEYWORD);
                        need_start_new_chunk = true;
                        break;
                    case ',':
                        add_token_chunk(EToken::COMMA, ERawToken::KEYWORD);
                        need_start_new_chunk = true;
                        break;
                    case '#':
                        add_token_chunk(EToken::HASH, ERawToken::KEYWORD);
                        need_start_new_chunk = true;
                        break;
                    default:
                        // Add to current unprocessed chunk
                        current_chunk += c;
                        break;
                }
                break;
                
            case State::IN_LINE_COMMENT:
                if (c == '\n') {
                    // Line comment ends - don't include newline in comment
                    add_string_chunk(); // This creates the comment as unprocessed string for now
                    // Now create the newline token separately
                    add_token_chunk(EToken::NEWLINE, ERawToken::NEWLINE);
                    state = State::NORMAL;
                    need_start_new_chunk = true; // Defer until after position increment
                    line++; column = 0;
                } else {
                    current_chunk += c;
                }
                break;
                
            case State::IN_BLOCK_COMMENT:
                current_chunk += c;
                if (c == '*' && next_c == '/') {
                    // Block comment ends
                    current_chunk += next_c;
                    position++; column++;
                    add_string_chunk(); // This creates the comment as unprocessed string for now
                    state = State::NORMAL;
                    start_new_chunk();
                }
                break;
                
            case State::IN_STRING:
                current_chunk += c;
                if (c == '"' && (position == 0 || source[position-1] != '\\')) {
                    // String ends (simple escape check - not complete)
                    state = State::NORMAL;
                }
                break;
                
            case State::IN_CHAR:
                current_chunk += c;
                if (c == '\'' && (position == 0 || source[position-1] != '\\')) {
                    // Char literal ends (simple escape check - not complete)
                    state = State::NORMAL;
                }
                break;
        }
        
        position++;
        column++;
        
        // Handle deferred start_new_chunk after position increment
        if (need_start_new_chunk) {
            start_new_chunk();
            need_start_new_chunk = false;
        }
    }
    
    // Add final chunk if any
    add_string_chunk();
    
    // Add EOF token
    RawToken eof = create_raw_token(EToken::EOF_TOKEN, ERawToken::EOF_TOKEN, line, column, position);
    chunks.emplace_back(std::move(eof), position, position, line, column);
    
    return chunks;
}

} // namespace layer1_sublayers
} // namespace cprime