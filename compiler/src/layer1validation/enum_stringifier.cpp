#include "enum_stringifier.h"
#include <magic_enum.hpp>

namespace cprime::layer1validation {

// ============================================================================
// EToken Conversion (using magic_enum for automatic enum reflection)
// ============================================================================

std::string EnumStringifier::etoken_to_string(EToken token) {
    auto name = magic_enum::enum_name(token);
    return std::string(name);
}

EToken EnumStringifier::string_to_etoken(const std::string& str) {
    auto value = magic_enum::enum_cast<EToken>(str);
    return value.has_value() ? value.value() : EToken::INVALID;
}

// ============================================================================
// ERawToken Conversion (using magic_enum for automatic enum reflection)
// ============================================================================

std::string EnumStringifier::erawtoken_to_string(ERawToken raw_token) {
    auto name = magic_enum::enum_name(raw_token);
    return std::string(name);
}

ERawToken EnumStringifier::string_to_erawtoken(const std::string& str) {
    auto value = magic_enum::enum_cast<ERawToken>(str);
    return value.has_value() ? value.value() : ERawToken::INVALID;
}

// ============================================================================
// Validation Utilities
// ============================================================================

bool EnumStringifier::is_valid_etoken_string(const std::string& str) {
    return magic_enum::enum_cast<EToken>(str).has_value();
}

bool EnumStringifier::is_valid_erawtoken_string(const std::string& str) {
    return magic_enum::enum_cast<ERawToken>(str).has_value();
}

// ============================================================================
// Debug Utilities (using magic_enum to get all values automatically)
// ============================================================================

std::vector<std::string> EnumStringifier::get_all_etoken_strings() {
    std::vector<std::string> result;
    
    // magic_enum provides constexpr enum_names() that returns all enum names
    constexpr auto names = magic_enum::enum_names<EToken>();
    result.reserve(names.size());
    
    for (const auto& name : names) {
        result.emplace_back(name);
    }
    
    return result;
}

std::vector<std::string> EnumStringifier::get_all_erawtoken_strings() {
    std::vector<std::string> result;
    
    // magic_enum provides constexpr enum_names() that returns all enum names
    constexpr auto names = magic_enum::enum_names<ERawToken>();
    result.reserve(names.size());
    
    for (const auto& name : names) {
        result.emplace_back(name);
    }
    
    return result;
}

} // namespace cprime::layer1validation