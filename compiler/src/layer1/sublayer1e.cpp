#include "layer1.h"
#include <unordered_map>
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
std::vector<RawToken> sublayer1e(const std::vector<ProcessingChunk>& input, StringTable& string_table) {
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

} // namespace layer1_sublayers
} // namespace cprime