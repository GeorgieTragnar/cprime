#include "layer1.h"
#include <cctype>

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

// Helper function to create RawToken with value
template<typename T>
static RawToken create_raw_token_with_value(EToken token, ERawToken raw_token, uint32_t line, uint32_t column, uint32_t position, T value) {
    RawToken result;
    result._token = token;
    result._raw_token = raw_token;
    result._line = line;
    result._column = column;
    result._position = position;
    result._literal_value = value;
    return result;
}

// Layer 1B: Extract string and character literals (prefix-aware)
std::vector<ProcessingChunk> sublayer1b(const std::vector<ProcessingChunk>& input, StringTable& string_table) {
    std::vector<ProcessingChunk> result;
    
    for (const auto& chunk : input) {
        if (chunk.is_processed()) {
            // Already processed tokens pass through
            result.push_back(chunk);
            continue;
        }
        
        const std::string& str = chunk.get_string();
        if (str.empty()) {
            continue;
        }
        
        // Check if this string chunk contains string/char literals
        size_t pos = 0;
        uint32_t current_line = chunk.line;
        uint32_t current_column = chunk.column;
        
        auto add_unprocessed_segment = [&](size_t start, size_t end) {
            if (start < end) {
                std::string segment = str.substr(start, end - start);
                result.emplace_back(std::move(segment), chunk.start_pos + start, 
                                  chunk.start_pos + end, current_line, current_column);
            }
        };
        
        auto create_string_token = [&](EToken token_type, size_t start, size_t end, const std::string& content) {
            // Intern the string content in the string table
            StringIndex string_index = string_table.intern(content);
            RawToken token = create_raw_token_with_value(token_type, ERawToken::LITERAL, 
                                                       current_line, current_column, 
                                                       chunk.start_pos + start, string_index);
            result.emplace_back(std::move(token), chunk.start_pos + start, 
                              chunk.start_pos + end, current_line, current_column);
        };
        
        size_t segment_start = 0;
        
        while (pos < str.size()) {
            char c = str[pos];
            
            // Look for string literals
            if (c == '"') {
                // Check for prefix before quote
                size_t prefix_start = pos;
                EToken string_type = EToken::STRING_LITERAL;
                
                // Backtrack to find prefix
                if (pos >= 2 && str.substr(pos-2, 2) == "u8") {
                    prefix_start = pos - 2;
                    string_type = EToken::STRING8_LITERAL;
                } else if (pos >= 1) {
                    char prefix = str[pos-1];
                    if (prefix == 'L') {
                        prefix_start = pos - 1;
                        string_type = EToken::WSTRING_LITERAL;
                    } else if (prefix == 'u') {
                        prefix_start = pos - 1;
                        string_type = EToken::STRING16_LITERAL;
                    } else if (prefix == 'U') {
                        prefix_start = pos - 1;
                        string_type = EToken::STRING32_LITERAL;
                    } else if (prefix == 'R') {
                        prefix_start = pos - 1;
                        string_type = EToken::RAW_STRING_LITERAL;
                    }
                }
                
                // Check for combined prefixes like LR", uR", UR", u8R"
                if (string_type == EToken::RAW_STRING_LITERAL && prefix_start > 0) {
                    char before_R = str[prefix_start - 1];
                    if (before_R == 'L' || before_R == 'u' || before_R == 'U') {
                        prefix_start--;
                    } else if (prefix_start >= 2 && str.substr(prefix_start-2, 2) == "u8") {
                        prefix_start -= 2;
                    }
                }
                
                // Find end of string
                size_t string_end = pos + 1;
                bool escaped = false;
                
                if (string_type == EToken::RAW_STRING_LITERAL) {
                    // Handle raw string: R"delimiter(content)delimiter"
                    // Simplified - assumes no delimiter for now
                    while (string_end < str.size()) {
                        if (str[string_end] == ')' && string_end + 1 < str.size() && str[string_end + 1] == '"') {
                            string_end += 2;
                            break;
                        }
                        string_end++;
                    }
                } else {
                    // Regular string with escape handling
                    while (string_end < str.size()) {
                        if (str[string_end] == '\\') {
                            escaped = !escaped;
                        } else if (str[string_end] == '"' && !escaped) {
                            string_end++;
                            break;
                        } else {
                            escaped = false;
                        }
                        string_end++;
                    }
                }
                
                // Add unprocessed segment before string
                add_unprocessed_segment(segment_start, prefix_start);
                
                // Add string token
                std::string string_content = str.substr(prefix_start, string_end - prefix_start);
                create_string_token(string_type, prefix_start, string_end, string_content);
                
                pos = string_end;
                segment_start = string_end;
                continue;
            }
            
            // Look for character literals
            if (c == '\'') {
                // Check for prefix before quote
                size_t prefix_start = pos;
                EToken char_type = EToken::CHAR_LITERAL;
                
                // Backtrack to find prefix
                if (pos >= 1) {
                    char prefix = str[pos-1];
                    if (prefix == 'L') {
                        prefix_start = pos - 1;
                        char_type = EToken::WCHAR_LITERAL;
                    } else if (prefix == 'u') {
                        prefix_start = pos - 1;
                        char_type = EToken::CHAR16_LITERAL;
                    } else if (prefix == 'U') {
                        prefix_start = pos - 1;
                        char_type = EToken::CHAR32_LITERAL;
                    }
                }
                
                // Find end of char literal
                size_t char_end = pos + 1;
                bool escaped = false;
                
                while (char_end < str.size()) {
                    if (str[char_end] == '\\') {
                        escaped = !escaped;
                    } else if (str[char_end] == '\'' && !escaped) {
                        char_end++;
                        break;
                    } else {
                        escaped = false;
                    }
                    char_end++;
                }
                
                // Add unprocessed segment before char
                add_unprocessed_segment(segment_start, prefix_start);
                
                // Add char token - parse actual character value and intern the literal text
                std::string char_content = str.substr(prefix_start, char_end - prefix_start);
                StringIndex char_string_index = string_table.intern(char_content);
                
                // For simplicity, store character as single char (would need proper parsing)
                char char_value = (char_content.length() >= 3) ? char_content[char_content.length()-2] : '\0';
                RawToken token = create_raw_token_with_value(char_type, ERawToken::LITERAL, 
                                                           current_line, current_column, 
                                                           chunk.start_pos + prefix_start, char_value);
                result.emplace_back(std::move(token), chunk.start_pos + prefix_start, 
                                  chunk.start_pos + char_end, current_line, current_column);
                
                pos = char_end;
                segment_start = char_end;
                continue;
            }
            
            // Check for comments that should become comment tokens
            if (pos + 1 < str.size() && str[pos] == '/' && (str[pos+1] == '/' || str[pos+1] == '*')) {
                // Find comment end
                size_t comment_end;
                if (str[pos+1] == '/') {
                    // Line comment - find newline
                    comment_end = str.find('\n', pos);
                    if (comment_end == std::string::npos) {
                        comment_end = str.size();
                    } else {
                        comment_end++; // Include newline
                    }
                } else {
                    // Block comment - find */
                    comment_end = str.find("*/", pos + 2);
                    if (comment_end == std::string::npos) {
                        comment_end = str.size();
                    } else {
                        comment_end += 2; // Include */
                    }
                }
                
                // Add unprocessed segment before comment
                add_unprocessed_segment(segment_start, pos);
                
                // Add comment token
                std::string comment_content = str.substr(pos, comment_end - pos);
                StringIndex comment_index = string_table.intern(comment_content);
                RawToken token = create_raw_token_with_value(EToken::COMMENT, ERawToken::COMMENT, 
                                                           current_line, current_column, 
                                                           chunk.start_pos + pos, comment_index);
                result.emplace_back(std::move(token), chunk.start_pos + pos, 
                                  chunk.start_pos + comment_end, current_line, current_column);
                
                pos = comment_end;
                segment_start = comment_end;
                continue;
            }
            
            pos++;
        }
        
        // Add remaining unprocessed segment
        add_unprocessed_segment(segment_start, str.size());
    }
    
    return result;
}

} // namespace layer1_sublayers
} // namespace cprime