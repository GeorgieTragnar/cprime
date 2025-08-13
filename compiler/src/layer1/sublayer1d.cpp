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

// Layer 1D: Extract number literals (suffix-aware)
std::vector<ProcessingChunk> sublayer1d(const std::vector<ProcessingChunk>& input) {
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

} // namespace layer1_sublayers
} // namespace cprime