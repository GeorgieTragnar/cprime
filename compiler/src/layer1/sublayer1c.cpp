#include "layer1.h"
#include <cctype>
#include <vector>

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

// Layer 1C: Extract operators that can never be part of identifiers (longest match)
std::vector<ProcessingChunk> sublayer1c(const std::vector<ProcessingChunk>& input) {
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

} // namespace layer1_sublayers
} // namespace cprime