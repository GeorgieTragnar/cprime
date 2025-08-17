#include "layer1.h"
#include "../commons/enum/token.h"
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

// Helper function to create RawToken with unresolved content for deferred tokenization
static RawToken create_unresolved_token(ERawToken raw_token, uint32_t line, uint32_t column, uint32_t position, const std::string& content, StringTable& string_table) {
    RawToken result;
    result._token = EToken::CHUNK;  // Explicit token for unresolved chunks awaiting context-aware resolution
    result._raw_token = raw_token;
    result._line = line;
    result._column = column;
    result._position = position;
    result.chunk_content_index = string_table.intern(content);  // Store StringTable index instead of raw string
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

// Layer 1E: Preserve identifier chunks as ERawToken for deferred semantic tokenization
// ARCHITECTURAL CHANGE: No longer does keyword/identifier/exec_alias resolution in Layer 1
// All identifier-like strings are preserved as unresolved content for Layer 2 context-aware resolution
std::vector<RawToken> sublayer1e(const std::vector<ProcessingChunk>& input, StringTable& string_table, ExecAliasRegistry& exec_alias_registry) {
    // Note: exec_alias_registry is intentionally unused in this deferred tokenization approach
    (void)exec_alias_registry; // Suppress unused warning
    
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
                
                // DEFERRED SEMANTIC TOKENIZATION: Preserve all identifier-like chunks
                // Layer 2 will resolve these with proper namespace context
                RawToken token = create_unresolved_token(ERawToken::IDENTIFIER, current_line, 
                                                       current_column, chunk.start_pos + id_start, identifier, string_table);
                result.push_back(std::move(token));
                
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

} // namespace layer1_sublayers
} // namespace cprime