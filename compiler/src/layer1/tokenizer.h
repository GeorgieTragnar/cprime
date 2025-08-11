#pragma once

#include "../commons/rawToken.h"
#include "../commons/dirty/common_types.h"
#include "../commons/dirty/string_table.h"
#include "../commons/logger.h"
#include <variant>
#include <vector>
#include <string>
#include <sstream>

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

/**
 * Layer 1: Multi-Pass Tokenization
 * 
 * Clean pipeline approach:
 * - Master function calls sub-layers in sequence
 * - Each layer processes chunks and returns refined chunks
 * - Progressive reduction of unprocessed strings to final tokens
 */
class Tokenizer {
public:
    /**
     * Master tokenization function - clean pipeline
     * 
     * @param stream The stringstream containing source code
     * @param string_table Reference to string table for interning strings
     * @return const std::vector<RawToken> with all tokens
     */
    static const std::vector<RawToken> tokenize_stream(std::stringstream& stream, StringTable& string_table);

private:
    // Layer 1A: Extract unambiguous single-character tokens + state machine
    static std::vector<ProcessingChunk> layer_1a_unambiguous_tokens(std::stringstream& stream);
    
    // Layer 1B: Extract string and character literals (prefix-aware)
    static std::vector<ProcessingChunk> layer_1b_string_literals(const std::vector<ProcessingChunk>& input, StringTable& string_table);
    
    // Layer 1C: Extract operators that can never be part of identifiers
    static std::vector<ProcessingChunk> layer_1c_operators(const std::vector<ProcessingChunk>& input);
    
    // Layer 1D: Extract number literals (suffix-aware)
    static std::vector<ProcessingChunk> layer_1d_number_literals(const std::vector<ProcessingChunk>& input);
    
    // Layer 1E: Extract keywords and convert remaining strings to identifiers
    static std::vector<RawToken> layer_1e_keywords_and_identifiers(const std::vector<ProcessingChunk>& input, StringTable& string_table);
    
    // Helper functions
    static RawToken create_raw_token(EToken token, ERawToken raw_token, uint32_t line, uint32_t column, uint32_t position);
    template<typename T>
    static RawToken create_raw_token_with_value(EToken token, ERawToken raw_token, uint32_t line, uint32_t column, uint32_t position, T value);
};

} // namespace cprime