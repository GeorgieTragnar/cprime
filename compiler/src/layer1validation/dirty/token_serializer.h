#pragma once

#include "../commons/rawToken.h"
#include "../commons/dirty/string_table.h"
#include <string>
#include <vector>

namespace cprime::layer1validation {

/**
 * RawToken serialization utilities for Layer 1 validation and testing.
 * 
 * Provides human-readable serialization of RawToken objects for:
 * - Integration test expected output files
 * - CLI debugging utilities
 * - Test failure log generation
 * - Output comparison in tests
 * 
 * Serialization Format:
 * TOKEN: raw=IDENTIFIER, token=IDENTIFIER, pos=0, line=1, col=1, value=StringIndex[0]:"main"
 * TOKEN: raw=PUNCTUATION, token=LEFT_PAREN, pos=4, line=1, col=5, value=none
 * TOKEN: raw=LITERAL, token=INT_LITERAL, pos=10, line=1, col=11, value=int32:42
 */
class TokenSerializer {
public:
    // ========================================================================
    // Single Token Serialization
    // ========================================================================
    
    /**
     * Serialize RawToken to human-readable format.
     * 
     * Examples:
     * TOKEN: raw=IDENTIFIER, token=IDENTIFIER, pos=15, line=2, col=10, value=StringIndex[3]:"variable"
     * TOKEN: raw=LITERAL, token=INT_LITERAL, pos=20, line=2, col=15, value=int32:42
     * TOKEN: raw=PUNCTUATION, token=SEMICOLON, pos=22, line=2, col=17, value=none
     * 
     * @param token RawToken to serialize
     * @param string_table StringTable for resolving StringIndex values
     * @return Human-readable string representation
     */
    static std::string serialize(const RawToken& token, const StringTable& string_table);
    
    /**
     * Deserialize RawToken from string format.
     * Used for loading test case expected output files.
     * 
     * @param serialized Serialized token string
     * @param string_table StringTable for interning strings during deserialization
     * @return RawToken object reconstructed from string
     * @throws std::invalid_argument if format is invalid
     */
    static RawToken deserialize(const std::string& serialized, StringTable& string_table);
    
    // ========================================================================
    // Batch Token Serialization
    // ========================================================================
    
    /**
     * Serialize vector of RawTokens to multiline string.
     * Each token is serialized on a separate line.
     * 
     * @param tokens Vector of RawToken objects to serialize
     * @param string_table StringTable for resolving StringIndex values
     * @return Multiline string with one token per line
     */
    static std::string serialize_tokens(const std::vector<RawToken>& tokens, const StringTable& string_table);
    
    /**
     * Parse multiline token serialization back to vector.
     * Used for loading test case expected output files.
     * 
     * @param serialized Multiline serialized tokens string
     * @param string_table StringTable for interning strings during parsing
     * @return Vector of RawToken objects
     * @throws std::invalid_argument if any line has invalid format
     */
    static std::vector<RawToken> parse_tokens(const std::string& serialized, StringTable& string_table);
    
    /**
     * Parse expected output file content to RawToken vector.
     * Convenience method for loading test case layer2 files.
     * 
     * @param file_content Content of layer2 expected output file
     * @param string_table StringTable for interning strings
     * @return Vector of expected RawToken objects
     */
    static std::vector<RawToken> parse_expected_output(const std::string& file_content, StringTable& string_table);
    
    // ========================================================================
    // Validation and Comparison Utilities
    // ========================================================================
    
    /**
     * Check if a string is a valid token serialization format.
     * @param serialized String to validate
     * @return true if format is valid (parseable)
     */
    static bool is_valid_token_format(const std::string& serialized);
    
    /**
     * Compare two RawToken vectors for equality.
     * Provides detailed diff information on mismatch.
     * 
     * @param expected Expected tokens
     * @param actual Actual tokens
     * @param string_table StringTable for error message formatting
     * @return Empty string if equal, detailed diff message if different
     */
    static std::string compare_tokens(const std::vector<RawToken>& expected, 
                                     const std::vector<RawToken>& actual,
                                     const StringTable& string_table);
    
    /**
     * Generate detailed diff between expected and actual token at specific index.
     * Used by compare_tokens for detailed error reporting.
     * 
     * @param expected_token Expected token
     * @param actual_token Actual token
     * @param index Token index in the vector
     * @param string_table StringTable for formatting
     * @return Detailed diff message
     */
    static std::string diff_tokens(const RawToken& expected_token,
                                  const RawToken& actual_token,
                                  size_t index,
                                  const StringTable& string_table);

private:
    // Helper functions for variant value serialization/parsing
    static std::string serialize_variant_value(const decltype(RawToken::_literal_value)& value, const StringTable& string_table);
    static void parse_variant_value(const std::string& value_str, RawToken& token, StringTable& string_table);
    
    // String escaping utilities
    static std::string escape_string(const std::string& str);
    static std::string unescape_string(const std::string& str);
    
    // Individual field parsing
    static ERawToken parse_raw_token_field(const std::string& field);
    static EToken parse_token_field(const std::string& field);
    static uint32_t parse_numeric_field(const std::string& field);
};

} // namespace cprime::layer1validation