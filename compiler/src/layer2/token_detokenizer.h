#pragma once

#include "../commons/token.h"
#include "../commons/rawToken.h"
#include "../commons/dirty/string_table.h"
#include <vector>
#include <string>

namespace cprime {

/**
 * Converts tokens back to their exact original string representation.
 * This allows extracting raw source text from tokenized content without
 * modifying Layer 1 logic or conforming to specific syntax requirements.
 */
class TokenDetokenizer {
public:
    /**
     * Convert a vector of tokens back to original source string.
     * @param tokens The tokens to detokenize
     * @param string_table String table for resolving identifiers and literals
     * @param raw_tokens Original raw tokens for literal value resolution
     * @return Reconstructed source string
     */
    static std::string detokenize_to_string(
        const std::vector<Token>& tokens,
        const StringTable& string_table,
        const std::vector<RawToken>& raw_tokens);

private:
    /**
     * Convert a single token back to its original string representation.
     */
    static std::string token_to_original_string(
        const Token& token, 
        const StringTable& string_table,
        const std::vector<RawToken>& raw_tokens);
    
    /**
     * Resolve identifier or literal string from token using string table.
     */
    static std::string resolve_token_value(
        const Token& token,
        const StringTable& string_table,
        const std::vector<RawToken>& raw_tokens);
    
    /**
     * Convert EToken back to its exact original symbol/keyword.
     */
    static std::string etoken_to_original_symbol(EToken token);
    
    /**
     * Handle literal values from RawToken variant.
     */
    static std::string format_literal_value(
        const std::variant<
            std::monostate, int32_t, uint32_t, int64_t, uint64_t, long long, unsigned long long,
            float, double, long double, char, wchar_t, char16_t, char32_t, bool,
            StringIndex, ExecAliasIndex
        >& literal_value,
        const StringTable& string_table);
};

} // namespace cprime