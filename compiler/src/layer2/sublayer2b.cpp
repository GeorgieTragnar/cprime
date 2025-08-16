#include "layer2.h"
#include "token_detokenizer.h"
#include "../commons/enum/token.h"
#include "../commons/instruction.h"
#include "../commons/logger.h"
#include <sstream>
#include <magic_enum.hpp>

namespace cprime::layer2_sublayers {

// Clean up CPrime formatting artifacts from Lua script content
std::string clean_lua_script_formatting(const std::string& raw_lua) {
    if (raw_lua.empty()) {
        return "";
    }
    
    // Split into lines for processing
    std::vector<std::string> lines;
    std::stringstream ss(raw_lua);
    std::string line;
    
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
    
    if (lines.empty()) {
        return "";
    }
    
    // Remove leading and trailing empty lines
    while (!lines.empty() && lines.front().empty()) {
        lines.erase(lines.begin());
    }
    while (!lines.empty() && lines.back().empty()) {
        lines.pop_back();
    }
    
    if (lines.empty()) {
        return "";
    }
    
    // Find minimum indentation level (ignoring empty lines)
    size_t min_indent = SIZE_MAX;
    for (const auto& line : lines) {
        if (line.empty()) continue;
        
        size_t indent = 0;
        for (char c : line) {
            if (c == ' ') indent++;
            else if (c == '\t') indent += 4; // Convert tabs to spaces
            else break;
        }
        min_indent = std::min(min_indent, indent);
    }
    
    if (min_indent == SIZE_MAX) {
        min_indent = 0;
    }
    
    // Remove common indentation from all lines
    std::string result;
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string& line = lines[i];
        
        if (!line.empty()) {
            // Remove minimum indentation
            size_t spaces_to_remove = 0;
            for (char c : line) {
                if (c == ' ' && spaces_to_remove < min_indent) {
                    spaces_to_remove++;
                } else if (c == '\t' && spaces_to_remove + 4 <= min_indent) {
                    spaces_to_remove += 4;
                } else {
                    break;
                }
            }
            line = line.substr(spaces_to_remove);
        }
        
        result += line;
        if (i < lines.size() - 1) {
            result += "\n";
        }
    }
    
    return result;
}

// Extract identifiers between < > tokens from scope header, preserving order
std::vector<std::string> extract_parameter_identifiers(const Scope& scope, const StringTable& string_table, const std::map<std::string, std::vector<RawToken>>& streams) {
    std::vector<std::string> parameters;
    
    if (scope._header._tokens.empty()) {
        return parameters;
    }
    
    // Look for < token, then collect identifiers until > token
    bool inside_angle_brackets = false;
    
    for (const auto& token : scope._header._tokens) {
        // Get the actual token from streams
        if (token._stringstreamId >= streams.size()) continue;
        
        // Find the stream by matching the ID (this is a simplified approach)
        const std::vector<RawToken>* target_stream = nullptr;
        uint32_t stream_index = 0;
        for (const auto& [stream_name, raw_tokens] : streams) {
            if (stream_index == token._stringstreamId) {
                target_stream = &raw_tokens;
                break;
            }
            stream_index++;
        }
        
        if (!target_stream || token._tokenIndex >= target_stream->size()) continue;
        
        const RawToken& raw_token = (*target_stream)[token._tokenIndex];
        
        if (raw_token._token == EToken::LESS_THAN) {
            inside_angle_brackets = true;
        } else if (raw_token._token == EToken::GREATER_THAN) {
            inside_angle_brackets = false;
        } else if (inside_angle_brackets && raw_token._token == EToken::IDENTIFIER) {
            // Extract identifier string from string table
            if (std::holds_alternative<StringIndex>(raw_token._literal_value)) {
                StringIndex str_idx = std::get<StringIndex>(raw_token._literal_value);
                if (string_table.is_valid_index(str_idx)) {
                    std::string identifier = string_table.get_string(str_idx);
                    parameters.push_back(identifier);
                }
            }
        }
    }
    
    return parameters;
}

// Extract all tokens from scope body (instructions only, not header/footer)
std::vector<RawToken> extract_scope_body_tokens(const Scope& scope, const std::map<std::string, std::vector<RawToken>>& streams) {
    std::vector<RawToken> body_tokens;
    
    // Check if instructions contains an Instruction variant
    if (std::holds_alternative<Instruction>(scope._instructions)) {
        const Instruction& instruction = std::get<Instruction>(scope._instructions);
        for (const auto& token : instruction._tokens) {
            // Get the actual RawToken from streams
            if (token._stringstreamId >= streams.size()) continue;
            
            // Find the stream by matching the ID
            const std::vector<RawToken>* target_stream = nullptr;
            uint32_t stream_index = 0;
            for (const auto& [stream_name, raw_tokens] : streams) {
                if (stream_index == token._stringstreamId) {
                    target_stream = &raw_tokens;
                    break;
                }
                stream_index++;
            }
            
            if (!target_stream || token._tokenIndex >= target_stream->size()) continue;
            
            const RawToken& raw_token = (*target_stream)[token._tokenIndex];
            body_tokens.push_back(raw_token);
        }
    }
    // Handle the case where _instructions contains a nested scope index
    // For now, we'll just process single-level instructions
    
    return body_tokens;
}

void sublayer2b(std::vector<Scope>& scopes, 
                ExecAliasRegistry& exec_registry,
                const StringTable& string_table,
                const std::map<std::string, std::vector<RawToken>>& streams) {
    
    auto logger = cprime::LoggerFactory::get_logger("sublayer2b");
    
    LOG_INFO("=== Sublayer 2B: Exec Block Processing ===");
    LOG_INFO("Processing {} scopes for exec block compilation", scopes.size());
    
    // Process each registered exec scope
    for (const auto& [scope_index, executable_lambda] : exec_registry.get_scope_to_lambda_map()) {
        if (scope_index >= scopes.size()) {
            LOG_ERROR("Invalid scope index {} (only {} scopes available)", scope_index, scopes.size());
            continue;
        }
        
        const Scope& scope = scopes[scope_index];
        
        LOG_INFO("--- Processing exec scope {} ---", scope_index);
        
        // Step 1: Extract parameter identifiers between < > tokens
        std::vector<std::string> parameters = extract_parameter_identifiers(scope, string_table, streams);
        LOG_INFO("Extracted {} parameters: [{}]", parameters.size(), 
                [&parameters]() {
                    std::string result;
                    for (size_t i = 0; i < parameters.size(); ++i) {
                        if (i > 0) result += ", ";
                        result += "\"" + parameters[i] + "\"";
                    }
                    return result;
                }());
        
        // Step 2: Extract scope body tokens and detokenize to Lua script
        std::vector<RawToken> body_tokens = extract_scope_body_tokens(scope, streams);
        LOG_INFO("Extracted {} body tokens from scope", body_tokens.size());
        
        if (body_tokens.empty()) {
            LOG_WARN("Scope {} has no body tokens - skipping", scope_index);
            continue;
        }
        
        // Detokenize body to get Lua script
        std::string lua_script = TokenDetokenizer::detokenize_raw_tokens_to_string(body_tokens, string_table);
        
        // Clean up CPrime formatting artifacts from Lua script
        // Remove leading and trailing whitespace, normalize indentation
        std::string cleaned_lua_script = clean_lua_script_formatting(lua_script);
        
        LOG_INFO("Detokenized Lua script ({} chars):", cleaned_lua_script.length());
        LOG_INFO("{}", cleaned_lua_script);
        
        // Step 3: Create ExecutableLambda with the cleaned Lua script (preparation only)
        ExecutableLambda compiled_lambda;
        compiled_lambda.lua_script = cleaned_lua_script;
        
        LOG_INFO("=== EXEC BLOCK PREPARED FOR EXECUTION ===");
        LOG_INFO("Scope Index: {}", scope_index);
        LOG_INFO("Parameters: {} items", parameters.size());
        LOG_INFO("Lua Script Length: {} chars", cleaned_lua_script.length());
        LOG_INFO("Status: Ready for on-demand execution");
        LOG_INFO("=== END EXEC BLOCK PREPARATION ===");
        
        // Step 5: Update the registry with the compiled lambda
        exec_registry.update_executable_lambda(scope_index, compiled_lambda);
        
        LOG_INFO("Completed processing exec scope {}", scope_index);
        LOG_INFO(""); // Empty line for readability
    }
    
    LOG_INFO("=== Sublayer 2B Processing Complete ===");
    LOG_INFO("Processed {} exec scopes", exec_registry.get_exec_scope_count());
}

} // namespace cprime::layer2_sublayers