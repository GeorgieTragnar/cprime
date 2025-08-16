#include "token_detokenizer.h"
#include "../commons/enum/token.h"
#include <sstream>

namespace cprime {

std::string TokenDetokenizer::detokenize_to_string(
    const std::vector<Token>& tokens,
    const StringTable& string_table,
    const std::vector<RawToken>& raw_tokens) {
    
    if (tokens.empty()) {
        return "";
    }
    
    std::string result;
    result.reserve(1024); // Pre-allocate some space for efficiency
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        const Token& token = tokens[i];
        
        // Convert this token back to its original string
        std::string token_string = token_to_original_string(token, string_table, raw_tokens);
        
        // Append to result
        result += token_string;
        
        // Debug: Log individual token reconstruction (can be disabled in production)
        // std::cout << "[" << i << "] " << token_string << std::endl;
    }
    
    return result;
}

std::string TokenDetokenizer::detokenize_raw_tokens_to_string(
    const std::vector<RawToken>& raw_tokens,
    const StringTable& string_table) {
    
    if (raw_tokens.empty()) {
        return "";
    }
    
    std::string result;
    result.reserve(1024); // Pre-allocate some space for efficiency
    
    for (size_t i = 0; i < raw_tokens.size(); ++i) {
        const RawToken& raw_token = raw_tokens[i];
        
        // Convert this raw token back to its original string
        std::string token_string = raw_token_to_original_string(raw_token, string_table);
        
        // Append to result
        result += token_string;
        
        // Debug: Log individual token reconstruction (can be disabled in production)
        // std::cout << "[" << i << "] " << token_string << std::endl;
    }
    
    return result;
}

std::string TokenDetokenizer::get_test_script_1() {
    return R"LUA(
-- Test Script 1: Type Analysis and Class Generation
print("🔍 SCRIPT 1: Type Analysis Engine")
local param_count = 0
while params[param_count] do param_count = param_count + 1 end

math.randomseed(os.time())
local analysis_id = math.random(1000, 9999)
print("📊 Analysis ID:", analysis_id)

local categories = {primitives = {}, objects = {}, templates = {}}
for i = 0, param_count - 1 do
    if params[i] then
        local param = params[i]
        if string.find(param, "int") or string.find(param, "float") or string.find(param, "bool") then
            table.insert(categories.primitives, param)
        elseif string.find(param, "template") or string.find(param, "<") then
            table.insert(categories.templates, param)
        else
            table.insert(categories.objects, param)
        end
        print("   🎯 Categorized:", param)
    end
end

cprime.emit_line("// Type Analysis Report #" .. analysis_id)
cprime.emit_line("namespace analysis_" .. analysis_id .. " {")
cprime.emit_line("    const int primitive_count = " .. #categories.primitives .. ";")
cprime.emit_line("    const int template_count = " .. #categories.templates .. ";")
cprime.emit_line("    const int object_count = " .. #categories.objects .. ";")
cprime.emit_line("}")

return "ANALYSIS_" .. analysis_id .. ": Found " .. #categories.primitives .. " primitives, " .. #categories.templates .. " templates, " .. #categories.objects .. " objects"
)LUA";
}

std::string TokenDetokenizer::get_test_script_2() {
    return R"LUA(
-- Test Script 2: Code Generator with Statistics
print("⚙️ SCRIPT 2: Code Generator")
local param_count = 0
while params[param_count] do param_count = param_count + 1 end

math.randomseed(os.time() + 123)
local generator_id = math.random(2000, 2999)
print("🏭 Generator ID:", generator_id)

local total_complexity = 0
local function_count = 0
for i = 0, param_count - 1 do
    if params[i] then
        local param = params[i]
        local complexity = string.len(param) + math.random(1, 10)
        total_complexity = total_complexity + complexity
        function_count = function_count + 1
        
        print("   🛠️ Generating for:", param, "(complexity:", complexity .. ")")
        cprime.emit_line("func<auto> process_" .. string.gsub(param, "[^%w]", "_") .. "_" .. i .. "() {")
        cprime.emit_line("    // Generated function for " .. param .. " (complexity: " .. complexity .. ")")
        cprime.emit_line("    return create_" .. string.gsub(param, "[^%w]", "_") .. "();")
        cprime.emit_line("}")
    end
end

local avg_complexity = function_count > 0 and (total_complexity / function_count) or 0
cprime.emit_line("")
cprime.emit_line("// Generator Statistics")
cprime.emit_line("constexpr int TOTAL_FUNCTIONS = " .. function_count .. ";")
cprime.emit_line("constexpr int TOTAL_COMPLEXITY = " .. total_complexity .. ";")
cprime.emit_line("constexpr double AVG_COMPLEXITY = " .. string.format("%.2f", avg_complexity) .. ";")

return "GENERATOR_" .. generator_id .. ": Generated " .. function_count .. " functions with total complexity " .. total_complexity .. " (avg: " .. string.format("%.2f", avg_complexity) .. ")"
)LUA";
}

std::string TokenDetokenizer::get_test_script_3() {
    return R"LUA(
-- Test Script 3: Interface Builder with Validation
print("🏗️ SCRIPT 3: Interface Builder")
local param_count = 0
while params[param_count] do param_count = param_count + 1 end

math.randomseed(os.time() + 456)
local interface_id = math.random(3000, 3999)
print("🎨 Interface ID:", interface_id)

local validation_rules = {}
local interface_methods = {}
for i = 0, param_count - 1 do
    if params[i] then
        local param = params[i]
        local method_name = "handle_" .. string.gsub(param, "[^%w]", "_")
        local validation_level = math.random(1, 5)
        
        table.insert(interface_methods, method_name)
        table.insert(validation_rules, validation_level)
        
        print("   🎭 Building interface for:", param, "(validation level:", validation_level .. ")")
    end
end

cprime.emit_line("// Interface Definition #" .. interface_id)
cprime.emit_line("class IProcessor" .. interface_id .. " {")
cprime.emit_line("public:")
for i, method in ipairs(interface_methods) do
    local validation = validation_rules[i]
    cprime.emit_line("    virtual bool " .. method .. "() = 0;  // Validation level: " .. validation)
end
cprime.emit_line("    virtual ~IProcessor" .. interface_id .. "() = default;")
cprime.emit_line("};")

local total_validation = 0
for _, v in ipairs(validation_rules) do total_validation = total_validation + v end

return "INTERFACE_" .. interface_id .. ": Built " .. #interface_methods .. " methods with total validation score " .. total_validation
)LUA";
}

std::string TokenDetokenizer::raw_token_to_original_string(
    const RawToken& raw_token,
    const StringTable& string_table) {
    
    // For identifiers and literals, resolve from the literal value
    if (raw_token._token == EToken::IDENTIFIER ||
        raw_token._token == EToken::STRING_LITERAL ||
        raw_token._token == EToken::INT_LITERAL ||
        raw_token._token == EToken::FLOAT_LITERAL ||
        raw_token._token == EToken::CHAR_LITERAL ||
        raw_token._token == EToken::TRUE_LITERAL ||
        raw_token._token == EToken::FALSE_LITERAL ||
        raw_token._token == EToken::COMMENT ||
        raw_token._token == EToken::EXEC_ALIAS) {
        
        return format_literal_value(raw_token._literal_value, string_table);
    }
    
    // For keywords and operators, convert back to original symbol
    return etoken_to_original_symbol(raw_token._token);
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
        case EToken::FUNC:           return "func";
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
        
        // Whitespace and line endings
        case EToken::SPACE:          return " ";
        case EToken::TAB:            return "\t";
        case EToken::NEWLINE:        return "\n";
        case EToken::CARRIAGE_RETURN: return "\r";
        
        // Special tokens  
        case EToken::EOF_TOKEN:      return "";  // EOF doesn't contribute to source
        case EToken::INVALID:        return "";  // Invalid tokens should not appear in output
        
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
    
    // Exec alias - for now return a placeholder, later we should resolve from exec alias registry
    if (std::holds_alternative<ExecAliasIndex>(literal_value)) {
        ExecAliasIndex alias_idx = std::get<ExecAliasIndex>(literal_value);
        return "code_gen";  // Placeholder - should be resolved from ExecAliasRegistry
    }
    
    return "UNKNOWN_LITERAL";
}

} // namespace cprime