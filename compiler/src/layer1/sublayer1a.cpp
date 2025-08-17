#include "layer1.h"
#include "commons/logger.h"
#include <cctype>
#include <cstdio>
#include <iostream>

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

// Layer 1A: State machine for unambiguous single-character tokens + exec alias detection
std::vector<ProcessingChunk> sublayer1a(std::stringstream& stream, ExecAliasRegistry& exec_alias_registry) {
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
    
    // Exec alias detection hook - checks for "exec alias_name" patterns at chunk boundaries
    auto check_exec_alias = [&]() -> bool {
        auto logger = cprime::LoggerFactory::get_logger("main");
        
        // Only check when starting a new chunk and current character is 'e'
        if (!current_chunk.empty()) {
            LOG_DEBUG("check_exec_alias: skipping - current_chunk not empty: '{}'", current_chunk);
            return false;
        }
        
        LOG_DEBUG("check_exec_alias: checking position {} (char '{}')", position, source[position]);
        
        // Check for "exec " pattern (exec followed by whitespace)
        const std::string exec_pattern = "exec ";
        if (position + exec_pattern.length() > source.size()) {
            LOG_DEBUG("check_exec_alias: not enough characters remaining for 'exec ' pattern");
            return false;
        }
        
        // Verify the pattern matches
        for (size_t i = 0; i < exec_pattern.length(); ++i) {
            if (source[position + i] != exec_pattern[i]) {
                LOG_DEBUG("check_exec_alias: pattern mismatch at position {} + {}: expected '{}', got '{}'", 
                         position, i, exec_pattern[i], source[position + i]);
                return false;
            }
        }
        
        LOG_DEBUG("check_exec_alias: found 'exec ' pattern at position {}", position);
        
        // Find the start of the alias name (skip whitespace after "exec")
        size_t alias_start = position + exec_pattern.length();
        while (alias_start < source.size() && std::isspace(source[alias_start])) {
            alias_start++;
        }
        
        // Extract alias name (alphanumeric + underscore only)
        size_t alias_end = alias_start;
        while (alias_end < source.size() && 
               (std::isalnum(source[alias_end]) || source[alias_end] == '_')) {
            alias_end++;
        }
        
        // Valid alias name found?
        if (alias_end > alias_start) {
            std::string alias_name = source.substr(alias_start, alias_end - alias_start);
            LOG_DEBUG("check_exec_alias: found potential alias name: '{}'", alias_name);
            
            // Check what follows the alias name - should be template parameters or scope
            while (alias_end < source.size() && std::isspace(source[alias_end])) {
                alias_end++;
            }
            
            char next_char = (alias_end < source.size()) ? source[alias_end] : '\0';
            LOG_DEBUG("check_exec_alias: character after alias name: '{}'", next_char);
            
            // Valid exec template pattern: "exec alias_name<...>" or "exec alias_name {"
            if (alias_end < source.size() && (source[alias_end] == '<' || source[alias_end] == '{')) {
                // DEFERRED SEMANTIC TOKENIZATION: Exec alias registration now handled in Layer 2 with namespace context
                // Old Layer 1 registration disabled to prevent conflicts with namespace-aware system
                LOG_DEBUG("check_exec_alias: detected valid exec pattern '{}' - registration deferred to Layer 2", alias_name);
                return true;
            } else {
                LOG_DEBUG("check_exec_alias: invalid pattern - expected '<' or '{{' after alias name, got '{}'", next_char);
            }
        } else {
            LOG_DEBUG("check_exec_alias: no valid alias name found after 'exec '");
        }
        
        return false;
    };
    
    start_new_chunk();
    
    while (position < source.size()) {
        char c = source[position];
        char next_c = (position + 1 < source.size()) ? source[position + 1] : '\0';
        
        switch (state) {
            case State::NORMAL:
                // Check for exec alias patterns when encountering 'e' at chunk boundary
                if (c == 'e' && check_exec_alias()) {
                    // Exec alias detected and registered, continue with normal processing
                    // The hook doesn't consume characters, just registers the alias
                }
                
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
    
    // Debug: Log the contents of the ExecAliasRegistry after processing
    auto logger = cprime::LoggerFactory::get_logger("main");
    LOG_DEBUG("=== ExecAliasRegistry Debug Info (after sublayer1a) ===");
    LOG_DEBUG("Total registered aliases: {}", exec_alias_registry.size());
    
    auto all_aliases = exec_alias_registry.get_all_aliases();
    for (const auto& alias_pair : all_aliases) {
        LOG_DEBUG("  Alias '{}' -> Index {}", alias_pair.first, alias_pair.second.value);
    }
    LOG_DEBUG("=== End ExecAliasRegistry Debug Info ===");
    
    return chunks;
}

} // namespace layer1_sublayers
} // namespace cprime