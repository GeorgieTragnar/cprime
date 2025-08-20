#include "token.h"

namespace cprime {

// Centralized keyword mapping - single source of truth for all CPrime keywords
const std::unordered_map<std::string, EToken> KEYWORD_TO_ETOKEN_MAP = {
    // Class/Structure keywords  
    {"class", EToken::CLASS},
    {"struct", EToken::STRUCT},
    {"plex", EToken::PLEX},
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
    {"open", EToken::OPEN},
    {"closed", EToken::CLOSED},
    {"func", EToken::FUNC},
    
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
    {"exec", EToken::EXEC},
    
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

// Convert string to EToken, returns INVALID if not found
EToken string_to_etoken(const std::string& keyword) {
    auto it = KEYWORD_TO_ETOKEN_MAP.find(keyword);
    return (it != KEYWORD_TO_ETOKEN_MAP.end()) ? it->second : EToken::INVALID;
}


} // namespace cprime