#pragma once

#include "../commons/rawToken.h"
#include <variant>
#include <string>

namespace cprime {

/**
 * Processing chunk for multi-pass tokenization.
 * Contains either a processed token or unprocessed string content.
 */
struct ProcessingChunk {
    std::variant<RawToken, std::string> content;
    uint32_t start_pos;
    uint32_t end_pos;
    uint32_t line;
    uint32_t column;
    
    ProcessingChunk(RawToken token, uint32_t start, uint32_t end, uint32_t ln, uint32_t col)
        : content(std::move(token)), start_pos(start), end_pos(end), line(ln), column(col) {}
        
    ProcessingChunk(std::string str, uint32_t start, uint32_t end, uint32_t ln, uint32_t col)
        : content(std::move(str)), start_pos(start), end_pos(end), line(ln), column(col) {}
    
    bool is_processed() const { return std::holds_alternative<RawToken>(content); }
    bool is_unprocessed() const { return std::holds_alternative<std::string>(content); }
    
    const RawToken& get_token() const { return std::get<RawToken>(content); }
    const std::string& get_string() const { return std::get<std::string>(content); }
};

} // namespace cprime