#pragma once

#include <cstdint>

namespace cprime {

/**
 * Raw token categories for Layer 2 structural processing.
 * Focused on what Layer 2 needs for scope building and data extraction.
 */
enum class ERawToken : uint8_t {
    INVALID = 0,
    // Structural tokens (what Layer 2 cares about for scope building)
    LEFT_BRACE,      // { (opens scopes)
    RIGHT_BRACE,     // } (closes scopes) 
    SEMICOLON,       // ; (terminates statements)
    
    // Data-carrying tokens (need variant storage)
    IDENTIFIER,      // Variable/function names → StringIndex in variant
    LITERAL,         // All literal values → int32_t/float/etc. in variant
    
    // Non-structural tokens (Layer 2 mostly ignores for structural analysis)
    KEYWORD,         // All keywords, operators, punctuation (class, +, ,, ->, etc.)
    WHITESPACE,      // Spaces, tabs
    COMMENT,         // Comments
    NEWLINE,         // Line breaks
    EOF_TOKEN,       // End of file
};

} // namespace cprime