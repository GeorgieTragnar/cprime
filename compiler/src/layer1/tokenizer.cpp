#include "tokenizer.h"
#include <unordered_map>
#include <cctype>

namespace cprime {

// Helper function to create RawToken
RawToken Tokenizer::create_raw_token(EToken token, ERawToken raw_token, uint32_t line, uint32_t column, uint32_t position) {
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
RawToken Tokenizer::create_raw_token_with_value(EToken token, ERawToken raw_token, uint32_t line, uint32_t column, uint32_t position, T value) {
    RawToken result;
    result._token = token;
    result._raw_token = raw_token;
    result._line = line;
    result._column = column;
    result._position = position;
    result._literal_value = value;
    return result;
}

// Layer 1A: State machine for unambiguous single-character tokens
std::vector<ProcessingChunk> Tokenizer::layer_1a_unambiguous_tokens(std::stringstream& stream) {
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
                        start_new_chunk();
                        break;
                    case '\t':
                        add_token_chunk(EToken::TAB, ERawToken::WHITESPACE);
                        start_new_chunk();
                        break;
                    case '\r':
                        add_token_chunk(EToken::CARRIAGE_RETURN, ERawToken::WHITESPACE);
                        start_new_chunk();
                        break;
                    case '\v':
                        add_token_chunk(EToken::VERTICAL_TAB, ERawToken::WHITESPACE);
                        start_new_chunk();
                        break;
                    case '\f':
                        add_token_chunk(EToken::FORM_FEED, ERawToken::WHITESPACE);
                        start_new_chunk();
                        break;
                    case '\n':
                        add_token_chunk(EToken::NEWLINE, ERawToken::NEWLINE);
                        start_new_chunk();
                        line++; column = 0; // Will be incremented at end of loop
                        break;
                    case '{':
                        add_token_chunk(EToken::LEFT_BRACE, ERawToken::LEFT_BRACE);
                        start_new_chunk();
                        break;
                    case '}':
                        add_token_chunk(EToken::RIGHT_BRACE, ERawToken::RIGHT_BRACE);
                        start_new_chunk();
                        break;
                    case ';':
                        add_token_chunk(EToken::SEMICOLON, ERawToken::SEMICOLON);
                        start_new_chunk();
                        break;
                    case '(':
                        add_token_chunk(EToken::LEFT_PAREN, ERawToken::KEYWORD);
                        start_new_chunk();
                        break;
                    case ')':
                        add_token_chunk(EToken::RIGHT_PAREN, ERawToken::KEYWORD);
                        start_new_chunk();
                        break;
                    case '[':
                        add_token_chunk(EToken::LEFT_BRACKET, ERawToken::KEYWORD);
                        start_new_chunk();
                        break;
                    case ']':
                        add_token_chunk(EToken::RIGHT_BRACKET, ERawToken::KEYWORD);
                        start_new_chunk();
                        break;
                    case ',':
                        add_token_chunk(EToken::COMMA, ERawToken::KEYWORD);
                        start_new_chunk();
                        break;
                    case '#':
                        add_token_chunk(EToken::HASH, ERawToken::KEYWORD);
                        start_new_chunk();
                        break;
                    default:
                        // Add to current unprocessed chunk
                        current_chunk += c;
                        break;
                }
                break;
                
            case State::IN_LINE_COMMENT:
                current_chunk += c;
                if (c == '\n') {
                    // Line comment ends, create comment token
                    add_string_chunk(); // This creates the comment as unprocessed string for now
                    state = State::NORMAL;
                    start_new_chunk();
                    line++; column = 0;
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
    }
    
    // Add final chunk if any
    add_string_chunk();
    
    // Add EOF token
    RawToken eof = create_raw_token(EToken::EOF_TOKEN, ERawToken::EOF_TOKEN, line, column, position);
    chunks.emplace_back(std::move(eof), position, position, line, column);
    
    return chunks;
}

// Layer 1B: Extract string and character literals (prefix-aware)
std::vector<ProcessingChunk> Tokenizer::layer_1b_string_literals(const std::vector<ProcessingChunk>& input, StringTable& string_table) {
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

// Layer 1C: Extract operators that can never be part of identifiers (longest match)
std::vector<ProcessingChunk> Tokenizer::layer_1c_operators(const std::vector<ProcessingChunk>& input) {
    // Operator mapping (longest first for proper matching)
    // Only using existing EToken values for now
    static const std::vector<std::pair<std::string, EToken>> operators = {
        // Two-character operators  
        {"==", EToken::EQUALS},
        {"!=", EToken::NOT_EQUALS},
        {"<=", EToken::LESS_EQUAL},
        {">=", EToken::GREATER_EQUAL},
        {"&&", EToken::LOGICAL_AND},
        {"||", EToken::LOGICAL_OR},
        {"->", EToken::ARROW},
        {"::", EToken::SCOPE_RESOLUTION},
        {"<-", EToken::FIELD_LINK},
        
        // Single-character operators
        {"+", EToken::PLUS},
        {"-", EToken::MINUS},
        {"*", EToken::MULTIPLY},        // Also DEREFERENCE (context-dependent)
        {"/", EToken::DIVIDE},
        {"%", EToken::MODULO},
        {"=", EToken::ASSIGN},
        {"<", EToken::LESS_THAN},
        {">", EToken::GREATER_THAN},
        {"!", EToken::LOGICAL_NOT},
        {"&", EToken::BITWISE_AND},     // Also reference operator (context-dependent)
        {"|", EToken::BITWISE_OR},
        {"^", EToken::BITWISE_XOR},
        {"~", EToken::BITWISE_NOT},
        {".", EToken::DOT},
        {":", EToken::COLON}
    };
    
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
        
        size_t pos = 0;
        size_t segment_start = 0;
        uint32_t current_line = chunk.line;
        uint32_t current_column = chunk.column;
        
        auto add_unprocessed_segment = [&](size_t start, size_t end) {
            if (start < end) {
                std::string segment = str.substr(start, end - start);
                result.emplace_back(std::move(segment), chunk.start_pos + start, 
                                  chunk.start_pos + end, current_line, current_column);
            }
        };
        
        while (pos < str.size()) {
            bool found_operator = false;
            
            // Try to match operators (longest first)
            for (const auto& [op_str, token_type] : operators) {
                if (pos + op_str.length() <= str.size() && 
                    str.substr(pos, op_str.length()) == op_str) {
                    
                    // Special case: don't match single "." if followed by digit (could be .5 float)
                    if (op_str == "." && pos + 1 < str.size() && std::isdigit(str[pos + 1])) {
                        continue; // Skip this match, let number literal layer handle it
                    }
                    
                    // Add unprocessed segment before operator
                    add_unprocessed_segment(segment_start, pos);
                    
                    // Add operator token
                    RawToken token = create_raw_token(token_type, ERawToken::KEYWORD, 
                                                    current_line, current_column, chunk.start_pos + pos);
                    result.emplace_back(std::move(token), chunk.start_pos + pos, 
                                      chunk.start_pos + pos + op_str.length(), current_line, current_column);
                    
                    pos += op_str.length();
                    segment_start = pos;
                    found_operator = true;
                    break; // Found match, move to next position
                }
            }
            
            if (!found_operator) {
                pos++;
            }
        }
        
        // Add remaining unprocessed segment
        add_unprocessed_segment(segment_start, str.size());
    }
    
    return result;
}

// Layer 1D: Extract number literals (suffix-aware)
std::vector<ProcessingChunk> Tokenizer::layer_1d_number_literals(const std::vector<ProcessingChunk>& input) {
    std::vector<ProcessingChunk> result;
    
    auto is_digit = [](char c) { return std::isdigit(c); };
    auto is_hex_digit = [](char c) { return std::isxdigit(c); };
    
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
        
        size_t pos = 0;
        size_t segment_start = 0;
        uint32_t current_line = chunk.line;
        uint32_t current_column = chunk.column;
        
        auto add_unprocessed_segment = [&](size_t start, size_t end) {
            if (start < end) {
                std::string segment = str.substr(start, end - start);
                result.emplace_back(std::move(segment), chunk.start_pos + start, 
                                  chunk.start_pos + end, current_line, current_column);
            }
        };
        
        while (pos < str.size()) {
            char c = str[pos];
            
            // Check for number literal start
            bool is_number_start = false;
            bool is_hex = false;
            bool is_float = false;
            
            // Numbers can start with digit or '.' followed by digit
            if (is_digit(c)) {
                is_number_start = true;
                // Check for hex prefix
                if (c == '0' && pos + 1 < str.size() && (str[pos+1] == 'x' || str[pos+1] == 'X')) {
                    is_hex = true;
                }
            } else if (c == '.' && pos + 1 < str.size() && is_digit(str[pos+1])) {
                is_number_start = true;
                is_float = true;
            }
            
            if (!is_number_start) {
                pos++;
                continue;
            }
            
            // Found start of number, extract complete number
            size_t number_start = pos;
            size_t number_end = pos;
            
            if (is_hex) {
                // Hex number: 0x[hex_digits]
                number_end += 2; // Skip "0x"
                while (number_end < str.size() && is_hex_digit(str[number_end])) {
                    number_end++;
                }
                
                // Check for hex float: 0x1.5p10
                if (number_end < str.size() && str[number_end] == '.') {
                    is_float = true;
                    number_end++;
                    while (number_end < str.size() && is_hex_digit(str[number_end])) {
                        number_end++;
                    }
                }
                
                // Check for hex float exponent: p/P
                if (number_end < str.size() && (str[number_end] == 'p' || str[number_end] == 'P')) {
                    is_float = true;
                    number_end++;
                    if (number_end < str.size() && (str[number_end] == '+' || str[number_end] == '-')) {
                        number_end++;
                    }
                    while (number_end < str.size() && is_digit(str[number_end])) {
                        number_end++;
                    }
                }
            } else {
                // Decimal number
                if (!is_float) {
                    // Integer part
                    while (number_end < str.size() && is_digit(str[number_end])) {
                        number_end++;
                    }
                    
                    // Check for decimal point
                    if (number_end < str.size() && str[number_end] == '.') {
                        is_float = true;
                        number_end++;
                        while (number_end < str.size() && is_digit(str[number_end])) {
                            number_end++;
                        }
                    }
                } else {
                    // Started with '.', consume fractional part
                    number_end++; // Skip '.'
                    while (number_end < str.size() && is_digit(str[number_end])) {
                        number_end++;
                    }
                }
                
                // Check for scientific notation: e/E
                if (number_end < str.size() && (str[number_end] == 'e' || str[number_end] == 'E')) {
                    is_float = true;
                    number_end++;
                    if (number_end < str.size() && (str[number_end] == '+' || str[number_end] == '-')) {
                        number_end++;
                    }
                    while (number_end < str.size() && is_digit(str[number_end])) {
                        number_end++;
                    }
                }
            }
            
            // Parse suffix
            EToken number_type;
            if (is_float) {
                // Floating-point suffixes
                if (number_end < str.size()) {
                    char suffix = std::tolower(str[number_end]);
                    if (suffix == 'f') {
                        number_type = EToken::FLOAT_LITERAL;
                        number_end++;
                    } else if (suffix == 'l') {
                        number_type = EToken::LONG_DOUBLE_LITERAL;
                        number_end++;
                    } else {
                        number_type = EToken::DOUBLE_LITERAL; // Default
                    }
                } else {
                    number_type = EToken::DOUBLE_LITERAL; // Default
                }
            } else {
                // Integer suffixes
                bool is_unsigned = false;
                bool is_long = false;
                bool is_long_long = false;
                
                size_t suffix_pos = number_end;
                while (suffix_pos < str.size()) {
                    char suffix = std::tolower(str[suffix_pos]);
                    if (suffix == 'u' && !is_unsigned) {
                        is_unsigned = true;
                        suffix_pos++;
                    } else if (suffix == 'l' && !is_long) {
                        if (suffix_pos + 1 < str.size() && std::tolower(str[suffix_pos + 1]) == 'l') {
                            is_long_long = true;
                            suffix_pos += 2;
                        } else {
                            is_long = true;
                            suffix_pos++;
                        }
                    } else {
                        break; // Unknown suffix, stop
                    }
                }
                
                number_end = suffix_pos;
                
                // Determine integer type
                if (is_long_long) {
                    number_type = is_unsigned ? EToken::ULONG_LONG_LITERAL : EToken::LONG_LONG_LITERAL;
                } else if (is_long) {
                    number_type = is_unsigned ? EToken::ULONG_LITERAL : EToken::LONG_LITERAL;
                } else {
                    number_type = is_unsigned ? EToken::UINT_LITERAL : EToken::INT_LITERAL;
                }
            }
            
            // Add unprocessed segment before number
            add_unprocessed_segment(segment_start, number_start);
            
            // Parse and create number token with appropriate value
            std::string number_str = str.substr(number_start, number_end - number_start);
            
            RawToken token;
            if (is_float) {
                if (number_type == EToken::FLOAT_LITERAL) {
                    float value = std::stof(number_str);
                    token = create_raw_token_with_value(number_type, ERawToken::LITERAL, 
                                                      current_line, current_column, chunk.start_pos + number_start, value);
                } else if (number_type == EToken::LONG_DOUBLE_LITERAL) {
                    long double value = std::stold(number_str);
                    token = create_raw_token_with_value(number_type, ERawToken::LITERAL, 
                                                      current_line, current_column, chunk.start_pos + number_start, value);
                } else {
                    double value = std::stod(number_str);
                    token = create_raw_token_with_value(number_type, ERawToken::LITERAL, 
                                                      current_line, current_column, chunk.start_pos + number_start, value);
                }
            } else {
                // Integer parsing (simplified - would need proper error handling)
                if (number_type == EToken::INT_LITERAL) {
                    int32_t value = static_cast<int32_t>(std::stoll(number_str, nullptr, is_hex ? 16 : 10));
                    token = create_raw_token_with_value(number_type, ERawToken::LITERAL, 
                                                      current_line, current_column, chunk.start_pos + number_start, value);
                } else if (number_type == EToken::UINT_LITERAL) {
                    uint32_t value = static_cast<uint32_t>(std::stoull(number_str, nullptr, is_hex ? 16 : 10));
                    token = create_raw_token_with_value(number_type, ERawToken::LITERAL, 
                                                      current_line, current_column, chunk.start_pos + number_start, value);
                } else if (number_type == EToken::LONG_LITERAL) {
                    int64_t value = std::stoll(number_str, nullptr, is_hex ? 16 : 10);
                    token = create_raw_token_with_value(number_type, ERawToken::LITERAL, 
                                                      current_line, current_column, chunk.start_pos + number_start, value);
                } else if (number_type == EToken::ULONG_LITERAL) {
                    uint64_t value = std::stoull(number_str, nullptr, is_hex ? 16 : 10);
                    token = create_raw_token_with_value(number_type, ERawToken::LITERAL, 
                                                      current_line, current_column, chunk.start_pos + number_start, value);
                } else if (number_type == EToken::LONG_LONG_LITERAL) {
                    long long value = std::stoll(number_str, nullptr, is_hex ? 16 : 10);
                    token = create_raw_token_with_value(number_type, ERawToken::LITERAL, 
                                                      current_line, current_column, chunk.start_pos + number_start, value);
                } else if (number_type == EToken::ULONG_LONG_LITERAL) {
                    unsigned long long value = std::stoull(number_str, nullptr, is_hex ? 16 : 10);
                    token = create_raw_token_with_value(number_type, ERawToken::LITERAL, 
                                                      current_line, current_column, chunk.start_pos + number_start, value);
                }
            }
            
            result.emplace_back(std::move(token), chunk.start_pos + number_start, 
                              chunk.start_pos + number_end, current_line, current_column);
            
            pos = number_end;
            segment_start = number_end;
        }
        
        // Add remaining unprocessed segment
        add_unprocessed_segment(segment_start, str.size());
    }
    
    return result;
}

// Layer 1E: Extract keywords and convert remaining strings to identifiers
std::vector<RawToken> Tokenizer::layer_1e_keywords_and_identifiers(const std::vector<ProcessingChunk>& input, StringTable& string_table) {
    // Keyword mapping - all CPrime keywords
    static const std::unordered_map<std::string, EToken> keywords = {
        // Class/Structure keywords
        {"class", EToken::CLASS},
        {"struct", EToken::STRUCT},
        {"interface", EToken::INTERFACE},
        {"union", EToken::UNION},
        {"function", EToken::FUNCTION},
        {"functional", EToken::FUNCTIONAL},
        {"data", EToken::DATA},
        
        // Context-sensitive keywords
        {"runtime", EToken::RUNTIME},
        {"comptime", EToken::COMPTIME},
        {"constexpr", EToken::CONSTEXPR},
        {"defer", EToken::DEFER},
        {"auto", EToken::AUTO},
        {"var", EToken::VAR},
        {"const", EToken::CONST},
        {"semconst", EToken::SEMCONST},
        {"static", EToken::STATIC},
        {"inline", EToken::INLINE},
        {"volatile", EToken::VOLATILE},
        {"danger", EToken::DANGER},
        {"implements", EToken::IMPLEMENTS},
        {"extern", EToken::EXTERN},
        {"module", EToken::MODULE},
        {"default", EToken::DEFAULT},
        {"func", EToken::FUNC},
        {"open", EToken::OPEN},
        {"closed", EToken::CLOSED},
        
        // Control flow
        {"if", EToken::IF},
        {"else", EToken::ELSE},
        {"while", EToken::WHILE},
        {"for", EToken::FOR},
        {"return", EToken::RETURN},
        {"break", EToken::BREAK},
        {"continue", EToken::CONTINUE},
        {"try", EToken::TRY},
        {"catch", EToken::CATCH},
        {"recover", EToken::RECOVER},
        {"finally", EToken::FINALLY},
        {"signal", EToken::SIGNAL},
        {"except", EToken::EXCEPT},
        {"raise", EToken::RAISE},
        
        // Casting keywords
        {"cast", EToken::CAST},
        {"static_cast", EToken::STATIC_CAST},
        {"dynamic_cast", EToken::DYNAMIC_CAST},
        {"select", EToken::SELECT},
        
        // Primitive types
        {"int8_t", EToken::INT8_T},
        {"int16_t", EToken::INT16_T},
        {"int32_t", EToken::INT32_T},
        {"int64_t", EToken::INT64_T},
        {"uint8_t", EToken::UINT8_T},
        {"uint16_t", EToken::UINT16_T},
        {"uint32_t", EToken::UINT32_T},
        {"uint64_t", EToken::UINT64_T},
        {"size_t", EToken::SIZE_T},
        {"float", EToken::FLOAT},
        {"double", EToken::DOUBLE},
        {"bool", EToken::BOOL},
        {"char", EToken::CHAR},
        {"void", EToken::VOID},
        
        // Boolean and null literals
        {"true", EToken::TRUE_LITERAL},
        {"false", EToken::FALSE_LITERAL},
        {"nullptr", EToken::NULLPTR_LITERAL}
    };
    
    std::vector<RawToken> result;
    
    auto is_identifier_start = [](char c) {
        return std::isalpha(c) || c == '_';
    };
    
    auto is_identifier_char = [](char c) {
        return std::isalnum(c) || c == '_';
    };
    
    for (const auto& chunk : input) {
        if (chunk.is_processed()) {
            result.push_back(chunk.get_token());
            continue;
        }
        
        const std::string& str = chunk.get_string();
        if (str.empty()) {
            continue;
        }
        
        size_t pos = 0;
        uint32_t current_line = chunk.line;
        uint32_t current_column = chunk.column;
        
        while (pos < str.size()) {
            char c = str[pos];
            
            // Skip whitespace (shouldn't be any left, but just in case)
            if (std::isspace(c)) {
                pos++;
                continue;
            }
            
            // Check for identifier/keyword
            if (is_identifier_start(c)) {
                size_t id_start = pos;
                size_t id_end = pos;
                
                // Extract complete identifier
                while (id_end < str.size() && is_identifier_char(str[id_end])) {
                    id_end++;
                }
                
                std::string identifier = str.substr(id_start, id_end - id_start);
                
                // Check if it's a keyword
                auto keyword_it = keywords.find(identifier);
                if (keyword_it != keywords.end()) {
                    EToken token_type = keyword_it->second;
                    ERawToken raw_type;
                    
                    // Determine ERawToken category
                    if (token_type == EToken::TRUE_LITERAL || 
                        token_type == EToken::FALSE_LITERAL || 
                        token_type == EToken::NULLPTR_LITERAL) {
                        raw_type = ERawToken::LITERAL;
                        
                        // Create appropriate literal value
                        RawToken token;
                        if (token_type == EToken::TRUE_LITERAL) {
                            token = create_raw_token_with_value(token_type, raw_type, current_line, 
                                                              current_column, chunk.start_pos + id_start, true);
                        } else if (token_type == EToken::FALSE_LITERAL) {
                            token = create_raw_token_with_value(token_type, raw_type, current_line, 
                                                              current_column, chunk.start_pos + id_start, false);
                        } else { // NULLPTR_LITERAL
                            token = create_raw_token(token_type, raw_type, current_line, 
                                                   current_column, chunk.start_pos + id_start);
                        }
                        result.push_back(std::move(token));
                    } else {
                        raw_type = ERawToken::KEYWORD;
                        RawToken token = create_raw_token(token_type, raw_type, current_line, 
                                                        current_column, chunk.start_pos + id_start);
                        result.push_back(std::move(token));
                    }
                } else {
                    // It's an identifier - intern the identifier string
                    StringIndex identifier_index = string_table.intern(identifier);
                    RawToken token = create_raw_token_with_value(EToken::IDENTIFIER, ERawToken::IDENTIFIER, 
                                                               current_line, current_column, 
                                                               chunk.start_pos + id_start, identifier_index);
                    result.push_back(std::move(token));
                }
                
                pos = id_end;
            } else {
                // Unknown character - create error token or skip
                // For now, just skip
                pos++;
            }
        }
    }
    
    return result;
}

// Master function
const std::vector<RawToken> Tokenizer::tokenize_stream(std::stringstream& stream, StringTable& string_table) {
    // Layer 1A: Extract unambiguous tokens + state machine
    auto chunks_1a = layer_1a_unambiguous_tokens(stream);
    
    // Layer 1B: Extract string/char literals (prefix-aware)
    auto chunks_1b = layer_1b_string_literals(chunks_1a, string_table);
    
    // Layer 1C: Extract operators that can't be part of identifiers
    auto chunks_1c = layer_1c_operators(chunks_1b);
    
    // Layer 1D: Extract number literals (suffix-aware)
    auto chunks_1d = layer_1d_number_literals(chunks_1c);
    
    // Layer 1E: Extract keywords and convert remaining to identifiers
    return layer_1e_keywords_and_identifiers(chunks_1d, string_table);
}

} // namespace cprime