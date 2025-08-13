#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <variant>
#include <iomanip>
#include "../commons/rawToken.h"
#include "../commons/dirty/string_table.h"
#include "../layer1/tokenizer.h"

// Layer 1 validation interface for testing
// This header provides validation and serialization functions for layer 1 testing

namespace cprime {
namespace layer1_sublayers {
namespace validation {

// NOTE: Removed static StringTable - it was causing StringIndex validation errors
// Serialization now uses safe StringIndex representation without dereferencing

// Helper function to convert EToken to string
inline std::string etoken_to_string(EToken token) {
    switch (token) {
        case EToken::INVALID: return "INVALID";
        case EToken::INT_LITERAL: return "INT_LITERAL";
        case EToken::UINT_LITERAL: return "UINT_LITERAL";
        case EToken::LONG_LITERAL: return "LONG_LITERAL";
        case EToken::ULONG_LITERAL: return "ULONG_LITERAL";
        case EToken::LONG_LONG_LITERAL: return "LONG_LONG_LITERAL";
        case EToken::ULONG_LONG_LITERAL: return "ULONG_LONG_LITERAL";
        case EToken::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case EToken::DOUBLE_LITERAL: return "DOUBLE_LITERAL";
        case EToken::LONG_DOUBLE_LITERAL: return "LONG_DOUBLE_LITERAL";
        case EToken::CHAR_LITERAL: return "CHAR_LITERAL";
        case EToken::WCHAR_LITERAL: return "WCHAR_LITERAL";
        case EToken::CHAR16_LITERAL: return "CHAR16_LITERAL";
        case EToken::CHAR32_LITERAL: return "CHAR32_LITERAL";
        case EToken::STRING_LITERAL: return "STRING_LITERAL";
        case EToken::WSTRING_LITERAL: return "WSTRING_LITERAL";
        case EToken::STRING8_LITERAL: return "STRING8_LITERAL";
        case EToken::STRING16_LITERAL: return "STRING16_LITERAL";
        case EToken::STRING32_LITERAL: return "STRING32_LITERAL";
        case EToken::RAW_STRING_LITERAL: return "RAW_STRING_LITERAL";
        case EToken::TRUE_LITERAL: return "TRUE_LITERAL";
        case EToken::FALSE_LITERAL: return "FALSE_LITERAL";
        case EToken::NULLPTR_LITERAL: return "NULLPTR_LITERAL";
        case EToken::SPACE: return "SPACE";
        case EToken::TAB: return "TAB";
        case EToken::NEWLINE: return "NEWLINE";
        case EToken::CARRIAGE_RETURN: return "CARRIAGE_RETURN";
        case EToken::VERTICAL_TAB: return "VERTICAL_TAB";
        case EToken::FORM_FEED: return "FORM_FEED";
        case EToken::LEFT_BRACE: return "LEFT_BRACE";
        case EToken::RIGHT_BRACE: return "RIGHT_BRACE";
        case EToken::SEMICOLON: return "SEMICOLON";
        case EToken::LEFT_PAREN: return "LEFT_PAREN";
        case EToken::RIGHT_PAREN: return "RIGHT_PAREN";
        case EToken::LEFT_BRACKET: return "LEFT_BRACKET";
        case EToken::RIGHT_BRACKET: return "RIGHT_BRACKET";
        case EToken::COMMA: return "COMMA";
        case EToken::HASH: return "HASH";
        case EToken::PLUS: return "PLUS";
        case EToken::MINUS: return "MINUS";
        case EToken::MULTIPLY: return "MULTIPLY";
        case EToken::DIVIDE: return "DIVIDE";
        case EToken::MODULO: return "MODULO";
        case EToken::ASSIGN: return "ASSIGN";
        case EToken::EQUALS: return "EQUALS";
        case EToken::NOT_EQUALS: return "NOT_EQUALS";
        case EToken::LESS_THAN: return "LESS_THAN";
        case EToken::GREATER_THAN: return "GREATER_THAN";
        case EToken::LESS_EQUAL: return "LESS_EQUAL";
        case EToken::GREATER_EQUAL: return "GREATER_EQUAL";
        case EToken::LOGICAL_AND: return "LOGICAL_AND";
        case EToken::LOGICAL_OR: return "LOGICAL_OR";
        case EToken::LOGICAL_NOT: return "LOGICAL_NOT";
        case EToken::BITWISE_AND: return "BITWISE_AND";
        case EToken::BITWISE_OR: return "BITWISE_OR";
        case EToken::BITWISE_XOR: return "BITWISE_XOR";
        case EToken::BITWISE_NOT: return "BITWISE_NOT";
        case EToken::DOT: return "DOT";
        case EToken::COLON: return "COLON";
        case EToken::ARROW: return "ARROW";
        case EToken::SCOPE_RESOLUTION: return "SCOPE_RESOLUTION";
        case EToken::FIELD_LINK: return "FIELD_LINK";
        case EToken::IDENTIFIER: return "IDENTIFIER";
        case EToken::COMMENT: return "COMMENT";
        case EToken::EOF_TOKEN: return "EOF_TOKEN";
        default: return "UNKNOWN_TOKEN";
    }
}

// Helper function to convert ERawToken to string
inline std::string erawtoken_to_string(ERawToken raw_token) {
    switch (raw_token) {
        case ERawToken::INVALID: return "INVALID";
        case ERawToken::LEFT_BRACE: return "LEFT_BRACE";
        case ERawToken::RIGHT_BRACE: return "RIGHT_BRACE";
        case ERawToken::SEMICOLON: return "SEMICOLON";
        case ERawToken::IDENTIFIER: return "IDENTIFIER";
        case ERawToken::LITERAL: return "LITERAL";
        case ERawToken::KEYWORD: return "KEYWORD";
        case ERawToken::COMMENT: return "COMMENT";
        case ERawToken::WHITESPACE: return "WHITESPACE";
        case ERawToken::NEWLINE: return "NEWLINE";
        case ERawToken::EOF_TOKEN: return "EOF_TOKEN";
        default: return "UNKNOWN_RAW_TOKEN";
    }
}

// Helper function to serialize variant value safely (no StringTable dereferencing)
inline std::string serialize_variant_value_safe(const std::variant<
    std::monostate, int32_t, uint32_t, int64_t, uint64_t,
    long long, unsigned long long, float, double, long double,
    char, wchar_t, char16_t, char32_t, bool, StringIndex>& value) {
    
    if (std::holds_alternative<std::monostate>(value)) {
        return "none";
    } else if (std::holds_alternative<int32_t>(value)) {
        return "int32:" + std::to_string(std::get<int32_t>(value));
    } else if (std::holds_alternative<uint32_t>(value)) {
        return "uint32:" + std::to_string(std::get<uint32_t>(value));
    } else if (std::holds_alternative<int64_t>(value)) {
        return "int64:" + std::to_string(std::get<int64_t>(value));
    } else if (std::holds_alternative<uint64_t>(value)) {
        return "uint64:" + std::to_string(std::get<uint64_t>(value));
    } else if (std::holds_alternative<long long>(value)) {
        return "longlong:" + std::to_string(std::get<long long>(value));
    } else if (std::holds_alternative<unsigned long long>(value)) {
        return "ulonglong:" + std::to_string(std::get<unsigned long long>(value));
    } else if (std::holds_alternative<float>(value)) {
        return "float:" + std::to_string(std::get<float>(value));
    } else if (std::holds_alternative<double>(value)) {
        return "double:" + std::to_string(std::get<double>(value));
    } else if (std::holds_alternative<long double>(value)) {
        return "longdouble:" + std::to_string(std::get<long double>(value));
    } else if (std::holds_alternative<char>(value)) {
        return "char:" + std::to_string(static_cast<int>(std::get<char>(value)));
    } else if (std::holds_alternative<wchar_t>(value)) {
        return "wchar:" + std::to_string(static_cast<int>(std::get<wchar_t>(value)));
    } else if (std::holds_alternative<char16_t>(value)) {
        return "char16:" + std::to_string(static_cast<int>(std::get<char16_t>(value)));
    } else if (std::holds_alternative<char32_t>(value)) {
        return "char32:" + std::to_string(static_cast<int>(std::get<char32_t>(value)));
    } else if (std::holds_alternative<bool>(value)) {
        return std::string("bool:") + (std::get<bool>(value) ? "true" : "false");
    } else if (std::holds_alternative<StringIndex>(value)) {
        auto idx = std::get<StringIndex>(value);
        // Don't try to dereference - just show the index for debugging
        return "StringIndex[" + std::to_string(idx.value) + "]";
    }
    return "unknown";
}

// Helper function to escape string content
inline std::string escape_string(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '\r': result += "\\r"; break;
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            default: result += c; break;
        }
    }
    return result;
}

/**
 * Serialize single ProcessingChunk to string (safe version without StringTable dereferencing)
 */
inline std::string serialize_chunk_safe(const ProcessingChunk& chunk) {
    if (chunk.is_processed()) {
        // Processed chunk contains a RawToken
        const RawToken& token = chunk.get_token();
        std::string value_str = serialize_variant_value_safe(token._literal_value);
        return "CHUNK[PROCESSED]: raw=" + erawtoken_to_string(token._raw_token) + 
               ", token=" + etoken_to_string(token._token) + 
               ", pos=" + std::to_string(token._position) + 
               ", line=" + std::to_string(token._line) + 
               ", col=" + std::to_string(token._column) + 
               ", value=" + value_str;
    } else {
        // Unprocessed chunk contains string content
        const std::string& content = chunk.get_string();
        return "CHUNK[UNPROCESSED]: content=\"" + escape_string(content) + 
               "\", start=" + std::to_string(chunk.start_pos) + 
               ", end=" + std::to_string(chunk.end_pos) + 
               ", line=" + std::to_string(chunk.line) + 
               ", col=" + std::to_string(chunk.column);
    }
}

/**
 * Serialize single RawToken to string (safe version without StringTable dereferencing)
 */
inline std::string serialize_token_safe(const RawToken& token) {
    std::string value_str = serialize_variant_value_safe(token._literal_value);
    return "TOKEN: raw=" + erawtoken_to_string(token._raw_token) + 
           ", token=" + etoken_to_string(token._token) + 
           ", pos=" + std::to_string(token._position) + 
           ", line=" + std::to_string(token._line) + 
           ", col=" + std::to_string(token._column) + 
           ", value=" + value_str;
}

/**
 * Serialize layer 1 intermediate output (ProcessingChunk vector) to string for testing
 * This function converts intermediate processing chunks to a string representation for comparison
 * Uses safe serialization that doesn't dereference StringIndex values
 */
inline std::string serialize(const std::vector<cprime::ProcessingChunk>& chunks) {
    if (chunks.empty()) {
        return "# No chunks\n";
    }
    
    std::string result = "# ProcessingChunk serialization - " + std::to_string(chunks.size()) + " chunks\n";
    
    for (size_t i = 0; i < chunks.size(); ++i) {
        result += serialize_chunk_safe(chunks[i]);
        if (i < chunks.size() - 1) {
            result += "\n";
        }
    }
    
    return result;
}

/**
 * Serialize layer 1 output (RawToken vector) to string for testing
 * This function converts the output of layer1 to a string representation for comparison
 * Uses safe serialization that doesn't dereference StringIndex values
 */
inline std::string serialize(const std::vector<cprime::RawToken>& tokens) {
    if (tokens.empty()) {
        return "# No tokens\n";
    }
    
    std::string result = "# RawToken serialization - " + std::to_string(tokens.size()) + " tokens\n";
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        result += serialize_token_safe(tokens[i]);
        if (i < tokens.size() - 1) {
            result += "\n";
        }
    }
    
    return result;
}

/**
 * Deserialize ProcessingChunk vector from string
 * This function converts serialized processing chunks back to data structures
 */
inline std::vector<cprime::ProcessingChunk> deserialize_chunks(const std::string& serialized) {
    // TODO: Implement proper deserialization when needed for test framework
    // For now, return empty vector to avoid compilation errors
    (void)serialized; // Suppress unused parameter warning
    return std::vector<cprime::ProcessingChunk>{};
}

/**
 * Deserialize RawToken vector from string
 * This function converts serialized tokens back to data structures
 */
inline std::vector<cprime::RawToken> deserialize_tokens(const std::string& serialized) {
    // TODO: Implement proper deserialization when needed for test framework
    // For now, return empty vector to avoid compilation errors
    (void)serialized; // Suppress unused parameter warning
    return std::vector<cprime::RawToken>{};
}

/**
 * Deserialize test input for layer 1
 * This function converts test case input to the format expected by layer1
 * For layer 1, this should return the same stringstream that was passed in
 */
inline std::stringstream& deserialize(std::stringstream& input) {
    // Layer 1 takes stringstream directly, so no conversion needed
    return input;
}

} // namespace validation
} // namespace layer1_sublayers
} // namespace cprime