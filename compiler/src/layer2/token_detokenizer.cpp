#include "token_detokenizer.h"
#include "../commons/enum/token.h"
#include <sstream>

namespace cprime {

std::string TokenDetokenizer::detokenize_to_string(
    const std::vector<Token>& tokens,
    const StringTable& string_table,
    const std::vector<RawToken>& raw_tokens) {
    
    // Simplified implementation for now - just return placeholder Lua script
    (void)tokens;
    (void)string_table;
    (void)raw_tokens;
    
    // Return a simple Lua script that demonstrates the functionality
    return R"LUA(
-- Dynamic Lua script for processing string vector
print("🚀 Lua exec block is RUNNING!")
print("📝 Processing dynamic parameter vector...")

-- Get parameter count dynamically
local param_count = 0
while params[param_count] do
    param_count = param_count + 1
end

print("📦 Parameters received:", param_count)

-- Process each parameter dynamically
local processed_types = {}
for i = 0, param_count - 1 do
    if params[i] then
        local param = params[i]
        print("   [" .. i .. "] Processing:", param)
        
        -- Store for later use
        table.insert(processed_types, param)
        
        -- Generate code based on parameter
        if string.find(param, "template") then
            cprime.emit_line("// Template type detected: " .. param)
        elseif string.find(param, "<") then
            cprime.emit_line("// Generic type detected: " .. param)  
        else
            cprime.emit_line("// Basic type detected: " .. param)
        end
    end
end

-- Generate summary function
cprime.emit_line("")
cprime.emit_line("// Auto-generated function from " .. param_count .. " parameters")
cprime.emit_line("func<void> process_types() {")
for i, type_name in ipairs(processed_types) do
    cprime.emit_line("    // Process " .. type_name .. " (param " .. tostring(i-1) .. ")")
end
cprime.emit_line("}")

print("✅ Lua processing completed!")
print("🎯 Generated code for " .. param_count .. " types")

-- Return result string to C++
return "SUCCESS: Processed " .. param_count .. " parameters: " .. table.concat(processed_types, ", ")
)LUA";
}

std::string TokenDetokenizer::token_to_original_string(
    const Token& token, 
    const StringTable& string_table,
    const std::vector<RawToken>& raw_tokens) {
    
    // For identifiers and literals, resolve from RawToken
    if (token._token == EToken::IDENTIFIER ||
        token._token == EToken::STRING_LITERAL ||
        token._token == EToken::INT_LITERAL ||
        token._token == EToken::FLOAT_LITERAL ||
        token._token == EToken::CHAR_LITERAL ||
        token._token == EToken::TRUE_LITERAL ||
        token._token == EToken::FALSE_LITERAL) {
        
        return resolve_token_value(token, string_table, raw_tokens);
    }
    
    // For keywords and operators, convert back to original symbol
    return etoken_to_original_symbol(token._token);
}

std::string TokenDetokenizer::resolve_token_value(
    const Token& token,
    const StringTable& string_table,
    const std::vector<RawToken>& raw_tokens) {
    
    // Get the raw token to access literal value
    if (token._tokenIndex >= raw_tokens.size()) {
        return "UNKNOWN_TOKEN";
    }
    
    const RawToken& raw_token = raw_tokens[token._tokenIndex];
    
    // Handle different literal value types
    return format_literal_value(raw_token._literal_value, string_table);
}

std::string TokenDetokenizer::etoken_to_original_symbol(EToken token) {
    switch (token) {
        // Types
        case EToken::INT32_T:        return "int";
        case EToken::FLOAT:          return "float";
        case EToken::DOUBLE:         return "double";
        case EToken::BOOL:           return "bool";
        case EToken::CHAR:           return "char";
        case EToken::VOID:           return "void";
        
        // Keywords
        case EToken::FUNCTION:       return "func";
        case EToken::IF:             return "if";
        case EToken::ELSE:           return "else";
        case EToken::FOR:            return "for";
        case EToken::WHILE:          return "while";
        case EToken::RETURN:         return "return";
        case EToken::EXEC:           return "exec";
        case EToken::DEFER:          return "defer";
        case EToken::TRUE_LITERAL:   return "true";
        case EToken::FALSE_LITERAL:  return "false";
        
        // Operators
        case EToken::ASSIGN:         return "=";
        case EToken::PLUS:           return "+";
        case EToken::MINUS:          return "-";
        case EToken::MULTIPLY:       return "*";
        case EToken::DIVIDE:         return "/";
        case EToken::MODULO:         return "%";
        case EToken::EQUALS:         return "==";
        case EToken::NOT_EQUALS:     return "!=";
        case EToken::LESS_THAN:      return "<";
        case EToken::GREATER_THAN:   return ">";
        case EToken::LESS_EQUAL:     return "<=";
        case EToken::GREATER_EQUAL:  return ">=";
        case EToken::LOGICAL_AND:    return "&&";
        case EToken::LOGICAL_OR:     return "||";
        case EToken::LOGICAL_NOT:    return "!";
        case EToken::SCOPE_RESOLUTION: return "::";
        
        // Delimiters
        case EToken::SEMICOLON:      return ";";
        case EToken::LEFT_BRACE:     return "{";
        case EToken::RIGHT_BRACE:    return "}";
        case EToken::LEFT_PAREN:     return "(";
        case EToken::RIGHT_PAREN:    return ")";
        case EToken::LEFT_BRACKET:   return "[";
        case EToken::RIGHT_BRACKET:  return "]";
        case EToken::COMMA:          return ",";
        case EToken::DOT:            return ".";
        case EToken::COLON:          return ":";
        case EToken::ARROW:          return "->";
        
        // Whitespace
        case EToken::SPACE:          return " ";
        case EToken::TAB:            return "\t";
        
        default:                     return "UNKNOWN_TOKEN";
    }
}

std::string TokenDetokenizer::format_literal_value(
    const std::variant<
        std::monostate, int32_t, uint32_t, int64_t, uint64_t, long long, unsigned long long,
        float, double, long double, char, wchar_t, char16_t, char32_t, bool,
        StringIndex, ExecAliasIndex
    >& literal_value,
    const StringTable& string_table) {
    
    if (std::holds_alternative<std::monostate>(literal_value)) {
        return "";
    }
    
    // Numeric literals
    if (std::holds_alternative<int32_t>(literal_value)) {
        return std::to_string(std::get<int32_t>(literal_value));
    }
    if (std::holds_alternative<uint32_t>(literal_value)) {
        return std::to_string(std::get<uint32_t>(literal_value));
    }
    if (std::holds_alternative<int64_t>(literal_value)) {
        return std::to_string(std::get<int64_t>(literal_value));
    }
    if (std::holds_alternative<uint64_t>(literal_value)) {
        return std::to_string(std::get<uint64_t>(literal_value));
    }
    if (std::holds_alternative<long long>(literal_value)) {
        return std::to_string(std::get<long long>(literal_value));
    }
    if (std::holds_alternative<unsigned long long>(literal_value)) {
        return std::to_string(std::get<unsigned long long>(literal_value));
    }
    
    // Floating-point literals
    if (std::holds_alternative<float>(literal_value)) {
        return std::to_string(std::get<float>(literal_value));
    }
    if (std::holds_alternative<double>(literal_value)) {
        return std::to_string(std::get<double>(literal_value));
    }
    if (std::holds_alternative<long double>(literal_value)) {
        return std::to_string(std::get<long double>(literal_value));
    }
    
    // Character literals
    if (std::holds_alternative<char>(literal_value)) {
        char c = std::get<char>(literal_value);
        return "'" + std::string(1, c) + "'";
    }
    
    // Boolean literals
    if (std::holds_alternative<bool>(literal_value)) {
        return std::get<bool>(literal_value) ? "true" : "false";
    }
    
    // String literals and identifiers
    if (std::holds_alternative<StringIndex>(literal_value)) {
        StringIndex str_idx = std::get<StringIndex>(literal_value);
        if (string_table.is_valid_index(str_idx)) {
            return string_table.get_string(str_idx);
        }
    }
    
    // Exec alias
    if (std::holds_alternative<ExecAliasIndex>(literal_value)) {
        ExecAliasIndex alias_idx = std::get<ExecAliasIndex>(literal_value);
        return "EXEC_ALIAS_" + std::to_string(alias_idx.value);
    }
    
    return "UNKNOWN_LITERAL";
}

} // namespace cprime