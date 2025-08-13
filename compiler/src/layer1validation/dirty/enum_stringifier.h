#pragma once

#include "../commons/enum/token.h"
#include "../commons/enum/rawToken.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace cprime::layer1validation {

/**
 * Enum stringification utilities for Layer 1 validation and debugging.
 * Provides bidirectional conversion between enum values and human-readable strings.
 * 
 * Used by:
 * - Token serialization for integration tests
 * - CLI debugging utilities
 * - Test case expected output parsing
 */
class EnumStringifier {
public:
    // ========================================================================
    // EToken Conversion
    // ========================================================================
    
    /**
     * Convert EToken enum value to human-readable string.
     * @param token EToken value to convert
     * @return String representation (e.g., "INT_LITERAL", "CLASS", "LEFT_PAREN")
     */
    static std::string etoken_to_string(EToken token);
    
    /**
     * Parse string back to EToken enum value.
     * @param str String representation to parse
     * @return EToken value, or EToken::INVALID if string is not recognized
     */
    static EToken string_to_etoken(const std::string& str);
    
    // ========================================================================
    // ERawToken Conversion
    // ========================================================================
    
    /**
     * Convert ERawToken enum value to human-readable string.
     * @param raw_token ERawToken value to convert
     * @return String representation (e.g., "IDENTIFIER", "LITERAL", "KEYWORD")
     */
    static std::string erawtoken_to_string(ERawToken raw_token);
    
    /**
     * Parse string back to ERawToken enum value.
     * @param str String representation to parse
     * @return ERawToken value, or ERawToken::INVALID if string is not recognized
     */
    static ERawToken string_to_erawtoken(const std::string& str);
    
    // ========================================================================
    // Validation Utilities
    // ========================================================================
    
    /**
     * Check if a string is a valid EToken representation.
     * @param str String to check
     * @return true if string can be converted to valid EToken
     */
    static bool is_valid_etoken_string(const std::string& str);
    
    /**
     * Check if a string is a valid ERawToken representation.
     * @param str String to check
     * @return true if string can be converted to valid ERawToken
     */
    static bool is_valid_erawtoken_string(const std::string& str);
    
    // ========================================================================
    // Debug Utilities
    // ========================================================================
    
    /**
     * Get all valid EToken string representations.
     * Useful for CLI help messages and validation.
     * @return Vector of all EToken string names
     */
    static std::vector<std::string> get_all_etoken_strings();
    
    /**
     * Get all valid ERawToken string representations.
     * Useful for CLI help messages and validation.
     * @return Vector of all ERawToken string names
     */
    static std::vector<std::string> get_all_erawtoken_strings();
};

} // namespace cprime::layer1validation