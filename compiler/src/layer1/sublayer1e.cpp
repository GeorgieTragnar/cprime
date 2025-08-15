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

// Layer 1E: Extract keywords and convert remaining strings to identifiers
std::vector<RawToken> sublayer1e(const std::vector<ProcessingChunk>& input, StringTable& string_table, ExecAliasRegistry& exec_alias_registry) {
    
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
                
                // Check if it's a keyword using centralized lookup
                EToken token_type = string_to_etoken(identifier);
                if (token_type != EToken::INVALID) {
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
                } else if (exec_alias_registry.contains_alias(identifier)) {
                    // It's a registered exec alias - create EXEC_ALIAS token
                    ExecAliasIndex alias_index = exec_alias_registry.get_alias_index(identifier);
                    RawToken token = create_raw_token_with_value(EToken::EXEC_ALIAS, ERawToken::KEYWORD, 
                                                               current_line, current_column, 
                                                               chunk.start_pos + id_start, alias_index);
                    result.push_back(std::move(token));
                } else {
                    // It's a regular identifier - intern the identifier string
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

} // namespace layer1_sublayers
} // namespace cprime