#include "token_serializer.h"
#include "enum_stringifier.h"
#include <sstream>
#include <stdexcept>
#include <regex>

namespace cprime::layer1validation {

// ============================================================================
// Single Token Serialization
// ============================================================================

std::string TokenSerializer::serialize(const RawToken& token, const StringTable& string_table) {
    std::ostringstream oss;
    
    // Format: TOKEN: raw=IDENTIFIER, token=IDENTIFIER, pos=15, line=2, col=10, value=StringIndex[3]:"variable"
    oss << "TOKEN: raw=" << EnumStringifier::erawtoken_to_string(token._raw_token)
        << ", token=" << EnumStringifier::etoken_to_string(token._token)
        << ", pos=" << token._position
        << ", line=" << token._line
        << ", col=" << token._column
        << ", value=" << serialize_variant_value(token._literal_value, string_table);
    
    return oss.str();
}

RawToken TokenSerializer::deserialize(const std::string& serialized, StringTable& string_table) {
    // Parse: TOKEN: raw=IDENTIFIER, token=IDENTIFIER, pos=15, line=2, col=10, value=StringIndex[3]:"variable"
    std::regex pattern(R"(TOKEN: raw=(\w+), token=(\w+), pos=(\d+), line=(\d+), col=(\d+), value=(.+))");
    std::smatch matches;
    
    if (!std::regex_match(serialized, matches, pattern)) {
        throw std::invalid_argument("Invalid token format: " + serialized);
    }
    
    RawToken token;
    token._raw_token = EnumStringifier::string_to_erawtoken(matches[1].str());
    token._token = EnumStringifier::string_to_etoken(matches[2].str());
    token._position = std::stoul(matches[3].str());
    token._line = std::stoul(matches[4].str());
    token._column = std::stoul(matches[5].str());
    
    // Parse variant value
    parse_variant_value(matches[6].str(), token, string_table);
    
    return token;
}

// ============================================================================
// Batch Token Serialization
// ============================================================================

std::string TokenSerializer::serialize_tokens(const std::vector<RawToken>& tokens, const StringTable& string_table) {
    std::ostringstream oss;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) oss << "\n";
        oss << serialize(tokens[i], string_table);
    }
    return oss.str();
}

std::vector<RawToken> TokenSerializer::parse_tokens(const std::string& serialized, StringTable& string_table) {
    std::vector<RawToken> result;
    std::istringstream iss(serialized);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (!line.empty() && line.find("#") != 0) { // Skip empty lines and comments
            result.push_back(deserialize(line, string_table));
        }
    }
    
    return result;
}

std::vector<RawToken> TokenSerializer::parse_expected_output(const std::string& file_content, StringTable& string_table) {
    return parse_tokens(file_content, string_table);
}

// ============================================================================
// Validation and Comparison Utilities
// ============================================================================

bool TokenSerializer::is_valid_token_format(const std::string& serialized) {
    try {
        StringTable temp_table;
        deserialize(serialized, temp_table);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::string TokenSerializer::compare_tokens(const std::vector<RawToken>& expected, 
                                           const std::vector<RawToken>& actual,
                                           const StringTable& string_table) {
    if (expected.size() != actual.size()) {
        return "Token count mismatch: expected " + std::to_string(expected.size()) + 
               ", got " + std::to_string(actual.size());
    }
    
    for (size_t i = 0; i < expected.size(); ++i) {
        std::string expected_str = serialize(expected[i], string_table);
        std::string actual_str = serialize(actual[i], string_table);
        
        if (expected_str != actual_str) {
            return diff_tokens(expected[i], actual[i], i, string_table);
        }
    }
    
    return ""; // All tokens match
}

std::string TokenSerializer::diff_tokens(const RawToken& expected_token,
                                        const RawToken& actual_token,
                                        size_t index,
                                        const StringTable& string_table) {
    std::ostringstream oss;
    oss << "Token " << index << " mismatch:\n";
    oss << "Expected: " << serialize(expected_token, string_table) << "\n";
    oss << "Actual:   " << serialize(actual_token, string_table) << "\n";
    
    // Detailed field-by-field comparison
    if (expected_token._raw_token != actual_token._raw_token) {
        oss << "  - raw_token: expected " << EnumStringifier::erawtoken_to_string(expected_token._raw_token)
            << ", got " << EnumStringifier::erawtoken_to_string(actual_token._raw_token) << "\n";
    }
    if (expected_token._token != actual_token._token) {
        oss << "  - token: expected " << EnumStringifier::etoken_to_string(expected_token._token)
            << ", got " << EnumStringifier::etoken_to_string(actual_token._token) << "\n";
    }
    if (expected_token._position != actual_token._position) {
        oss << "  - position: expected " << expected_token._position
            << ", got " << actual_token._position << "\n";
    }
    if (expected_token._line != actual_token._line) {
        oss << "  - line: expected " << expected_token._line
            << ", got " << actual_token._line << "\n";
    }
    if (expected_token._column != actual_token._column) {
        oss << "  - column: expected " << expected_token._column
            << ", got " << actual_token._column << "\n";
    }
    
    return oss.str();
}

// ============================================================================
// Helper Functions
// ============================================================================

std::string TokenSerializer::serialize_variant_value(const decltype(RawToken::_literal_value)& value, const StringTable& string_table) {
    return std::visit([&string_table](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return "none";
        } else if constexpr (std::is_same_v<T, StringIndex>) {
            if (string_table.is_valid_index(v)) {
                return "StringIndex[" + std::to_string(v.value) + "]:\"" + 
                       escape_string(string_table.get_string(v)) + "\"";
            } else {
                return "StringIndex[" + std::to_string(v.value) + "]:INVALID";
            }
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return "int32:" + std::to_string(v);
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return "uint32:" + std::to_string(v);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return "int64:" + std::to_string(v);
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return "uint64:" + std::to_string(v);
        } else if constexpr (std::is_same_v<T, long long>) {
            return "longlong:" + std::to_string(v);
        } else if constexpr (std::is_same_v<T, unsigned long long>) {
            return "ulonglong:" + std::to_string(v);
        } else if constexpr (std::is_same_v<T, float>) {
            return "float:" + std::to_string(v);
        } else if constexpr (std::is_same_v<T, double>) {
            return "double:" + std::to_string(v);
        } else if constexpr (std::is_same_v<T, long double>) {
            return "longdouble:" + std::to_string(v);
        } else if constexpr (std::is_same_v<T, char>) {
            return std::string("char:'") + v + "'";
        } else if constexpr (std::is_same_v<T, wchar_t>) {
            return "wchar:" + std::to_string(static_cast<int>(v));
        } else if constexpr (std::is_same_v<T, char16_t>) {
            return "char16:" + std::to_string(static_cast<int>(v));
        } else if constexpr (std::is_same_v<T, char32_t>) {
            return "char32:" + std::to_string(static_cast<int>(v));
        } else if constexpr (std::is_same_v<T, bool>) {
            return std::string("bool:") + (v ? "true" : "false");
        } else {
            return "unknown";
        }
    }, value);
}

void TokenSerializer::parse_variant_value(const std::string& value_str, RawToken& token, StringTable& string_table) {
    if (value_str == "none") {
        token._literal_value = std::monostate{};
    } else if (value_str.find("StringIndex[") == 0) {
        // Parse: StringIndex[3]:"variable"
        std::regex pattern(R"(StringIndex\[(\d+)\]:\"([^\"]*)\")"); 
        std::smatch matches;
        if (std::regex_match(value_str, matches, pattern)) {
            uint32_t index = std::stoul(matches[1].str());
            std::string str = unescape_string(matches[2].str());
            StringIndex string_index = string_table.intern(str);
            token._literal_value = string_index;
        } else {
            throw std::invalid_argument("Invalid StringIndex format: " + value_str);
        }
    } else if (value_str.find("int32:") == 0) {
        int32_t val = std::stoi(value_str.substr(6));
        token._literal_value = val;
    } else if (value_str.find("uint32:") == 0) {
        uint32_t val = std::stoul(value_str.substr(7));
        token._literal_value = val;
    } else if (value_str.find("float:") == 0) {
        float val = std::stof(value_str.substr(6));
        token._literal_value = val;
    } else if (value_str.find("double:") == 0) {
        double val = std::stod(value_str.substr(7));
        token._literal_value = val;
    } else if (value_str.find("bool:") == 0) {
        bool val = value_str.substr(5) == "true";
        token._literal_value = val;
    } else if (value_str.find("char:'") == 0 && value_str.rfind("'") == value_str.length() - 1) {
        char val = value_str[6]; // Extract character between quotes
        token._literal_value = val;
    } 
    // Add more parsing for other types as needed
    else {
        throw std::invalid_argument("Unsupported variant value format: " + value_str);
    }
}

std::string TokenSerializer::escape_string(const std::string& str) {
    std::string result;
    result.reserve(str.size() * 2);
    
    for (char c : str) {
        switch (c) {
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    
    return result;
}

std::string TokenSerializer::unescape_string(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\\' && i + 1 < str.size()) {
            switch (str[i + 1]) {
                case '\\': result += '\\'; ++i; break;
                case '"': result += '"'; ++i; break;
                case 'n': result += '\n'; ++i; break;
                case 'r': result += '\r'; ++i; break;
                case 't': result += '\t'; ++i; break;
                default: result += str[i]; break;
            }
        } else {
            result += str[i];
        }
    }
    
    return result;
}

ERawToken TokenSerializer::parse_raw_token_field(const std::string& field) {
    return EnumStringifier::string_to_erawtoken(field);
}

EToken TokenSerializer::parse_token_field(const std::string& field) {
    return EnumStringifier::string_to_etoken(field);
}

uint32_t TokenSerializer::parse_numeric_field(const std::string& field) {
    return std::stoul(field);
}

} // namespace cprime::layer1validation