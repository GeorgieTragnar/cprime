#include "layer2.h"
#include "token_detokenizer.h"
#include "../commons/enum/token.h"
#include "../commons/instruction.h"
#include "../commons/logger.h"
#include <sstream>
#include <magic_enum.hpp>

namespace cprime::layer2_sublayers {

// Extract the parent alias name from a specialization exec scope header
// For "exec helper<...> scope_analyzer<...>", returns "helper"
std::string extract_parent_alias_name(const Scope& scope, const StringTable& string_table, const std::map<std::string, std::vector<RawToken>>& streams) {
    const auto& tokens = scope._header._tokens;
    
    bool found_exec = false;
    
    for (const auto& token : tokens) {
        if (token._token == EToken::CHUNK) {
            // Find the RawToken from streams
            for (const auto& [stream_name, raw_tokens] : streams) {
                if (token._stringstreamId == 0 && token._tokenIndex < raw_tokens.size()) { // TODO: Map stream names to IDs
                    const RawToken& raw_token = raw_tokens[token._tokenIndex];
                    if (raw_token._token == EToken::CHUNK) {
                        std::string chunk_content = string_table.get_string(raw_token.chunk_content_index);
                        
                        if (chunk_content == "exec") {
                            found_exec = true;
                        } else if (found_exec) {
                            // This is the first identifier after exec - the parent alias name
                            return chunk_content;
                        }
                    }
                    break;
                }
            }
        }
    }
    
    return ""; // No parent alias found
}

// Helper function to determine if an exec scope is a specialization (has multiple identifiers after exec)
bool is_exec_specialization(const Scope& scope, const StringTable& string_table, const std::map<std::string, std::vector<RawToken>>& streams) {
    const auto& tokens = scope._header._tokens;
    
    // Look for pattern: "exec" + multiple identifiers (excluding template parameters)
    // Parent: "exec helper<...>"  (1 identifier after exec)
    // Specialization: "exec helper<...> scope_analyzer<...>" (2+ identifiers after exec)
    
    bool found_exec = false;
    int identifier_count = 0;
    bool inside_template_brackets = false;
    
    for (const auto& token : tokens) {
        // Track template brackets to skip identifiers inside them
        if (token._token == EToken::LESS_THAN) {
            inside_template_brackets = true;
            continue;
        } else if (token._token == EToken::GREATER_THAN) {
            inside_template_brackets = false;
            continue;
        }
        
        // Get the RawToken to check if it's a CHUNK
        if (token._token == EToken::CHUNK && !inside_template_brackets) {
            // Find the RawToken from streams
            for (const auto& [stream_name, raw_tokens] : streams) {
                if (token._stringstreamId == 0 && token._tokenIndex < raw_tokens.size()) { // TODO: Map stream names to IDs
                    const RawToken& raw_token = raw_tokens[token._tokenIndex];
                    if (raw_token._token == EToken::CHUNK) {
                        std::string chunk_content = string_table.get_string(raw_token.chunk_content_index);
                        
                        if (chunk_content == "exec") {
                            found_exec = true;
                        } else if (found_exec) {
                            // Count only identifiers outside template brackets
                            identifier_count++;
                        }
                    }
                    break;
                }
            }
        }
    }
    
    // Specialization has 2+ identifiers after exec (outside template brackets)
    // Parent has 1 identifier after exec (outside template brackets)
    return found_exec && identifier_count >= 2;
}

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
    
    // Helper function to extract tokens from an instruction
    auto extract_from_instruction = [&](const Instruction& instruction) {
        for (const auto& token : instruction._tokens) {
            // Safety check: validate token stream ID
            if (token._stringstreamId >= streams.size()) {
                continue;
            }
            
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
            
            // Safety checks: validate stream and token index
            if (!target_stream) {
                continue;
            }
            if (token._tokenIndex >= target_stream->size()) {
                continue;
            }
            
            const RawToken& raw_token = (*target_stream)[token._tokenIndex];
            
            // Skip structural tokens that shouldn't be in Lua scripts
            if (raw_token._token == EToken::LEFT_BRACE || 
                raw_token._token == EToken::RIGHT_BRACE ||
                raw_token._token == EToken::SEMICOLON) {
                continue;
            }
            
            body_tokens.push_back(raw_token);
        }
    };
    
    // Extract from body instructions
    for (const auto& instruction_variant : scope._instructions) {
        // Check if this variant contains an Instruction
        if (std::holds_alternative<Instruction>(instruction_variant)) {
            const Instruction& instruction = std::get<Instruction>(instruction_variant);
            extract_from_instruction(instruction);
        }
        // Handle the case where instruction_variant contains a nested scope index
        // For now, we'll just process Instruction variants
    }
    
    // For exec scopes, also check the footer (where Lua script content is often stored)
    if (std::holds_alternative<Instruction>(scope._footer)) {
        const Instruction& footer_instruction = std::get<Instruction>(scope._footer);
        extract_from_instruction(footer_instruction);
    }
    
    return body_tokens;
}

std::vector<Scope> sublayer2b(const std::vector<Scope>& input_scopes, 
                              ExecAliasRegistry& exec_registry,
                              const StringTable& string_table,
                              const std::map<std::string, std::vector<RawToken>>& streams) {
    
    // Create a mutable copy of the input scopes for processing
    std::vector<Scope> scopes = input_scopes;
    
    auto logger = cprime::LoggerFactory::get_logger("sublayer2b");
    
    LOG_INFO("=== Sublayer 2B: Exec Block Processing ===");
    LOG_INFO("Processing {} scopes for exec block compilation", scopes.size());
    
    // Two-pass processing: parents first, then specializations
    std::vector<uint32_t> parent_scopes;
    std::vector<uint32_t> specialization_scopes;
    
    // Classify exec scopes into parents and specializations
    for (const auto& [scope_index, executable_lambda] : exec_registry.get_scope_to_lambda_map()) {
        if (scope_index >= scopes.size()) {
            LOG_ERROR("Invalid scope index {} (only {} scopes available)", scope_index, scopes.size());
            continue;
        }
        
        const Scope& scope = scopes[scope_index];
        
        // Determine if this is a parent or specialization by analyzing the header
        bool is_specialization = is_exec_specialization(scope, string_table, streams);
        
        if (is_specialization) {
            specialization_scopes.push_back(scope_index);
            LOG_INFO("Classified scope {} as SPECIALIZATION - will process in second pass", scope_index);
        } else {
            parent_scopes.push_back(scope_index);
            LOG_INFO("Classified scope {} as PARENT - will process in first pass", scope_index);
        }
    }
    
    // FIRST PASS: Process parent exec scopes
    LOG_INFO("=== FIRST PASS: Processing {} parent exec scopes ===", parent_scopes.size());
    for (uint32_t scope_index : parent_scopes) {
        const Scope& scope = scopes[scope_index];
        LOG_INFO("--- Processing parent exec scope {} ---", scope_index);
        
        // Step 0: Extract and register the parent alias name
        std::string parent_alias_name = extract_parent_alias_name(scope, string_table, streams);
        if (!parent_alias_name.empty()) {
            // Check if parent alias is already registered, if not register it
            ExecAliasIndex parent_alias_index;
            if (exec_registry.contains_alias(parent_alias_name)) {
                parent_alias_index = exec_registry.get_alias_index(parent_alias_name);
                LOG_INFO("Parent alias '{}' already registered with index {}", parent_alias_name, parent_alias_index.value);
            } else {
                parent_alias_index = exec_registry.register_alias(parent_alias_name);
                LOG_INFO("Registered new parent alias '{}' with index {}", parent_alias_name, parent_alias_index.value);
            }
            exec_registry.register_scope_index_to_exec_alias(parent_alias_index, scope_index);
            LOG_INFO("Linked parent alias '{}' to scope {}", parent_alias_name, scope_index);
        } else {
            LOG_WARN("Could not extract parent alias name from parent scope {}", scope_index);
        }
        
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
        
        LOG_INFO("Completed processing parent exec scope {}", scope_index);
        LOG_INFO(""); // Empty line for readability
    }
    
    // SECOND PASS: Process specialization exec scopes
    LOG_INFO("=== SECOND PASS: Processing {} specialization exec scopes ===", specialization_scopes.size());
    for (uint32_t scope_index : specialization_scopes) {
        const Scope& scope = scopes[scope_index];
        LOG_INFO("--- Processing specialization exec scope {} ---", scope_index);
        
        // TODO: Implement specialization processing logic
        // For now, we'll process them the same way as parents
        
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
        
        // Step 2: Extract scope body tokens and detokenize to CPrime content (not Lua)
        std::vector<RawToken> body_tokens = extract_scope_body_tokens(scope, streams);
        LOG_INFO("Extracted {} body tokens from specialization scope", body_tokens.size());
        
        if (body_tokens.empty()) {
            LOG_WARN("Specialization scope {} has no body tokens - skipping", scope_index);
            continue;
        }
        
        // Detokenize body to get CPrime content (this will be passed to parent's Lua script)
        std::string cprime_content = TokenDetokenizer::detokenize_raw_tokens_to_string(body_tokens, string_table);
        
        // Clean up formatting for CPrime content
        std::string cleaned_cprime_content = clean_lua_script_formatting(cprime_content);
        
        LOG_INFO("Detokenized specialization CPrime content ({} chars):", cleaned_cprime_content.length());
        LOG_INFO("{}", cleaned_cprime_content);
        
        // Step 3: Extract parent alias name and register the parent-specialization relationship
        std::string parent_alias_name = extract_parent_alias_name(scope, string_table, streams);
        if (!parent_alias_name.empty()) {
            exec_registry.register_specialization_to_parent(scope_index, parent_alias_name);
            LOG_INFO("Registered specialization {} to parent alias '{}'", scope_index, parent_alias_name);
        } else {
            LOG_WARN("Could not extract parent alias name from specialization scope {}", scope_index);
        }
        
        // Step 4: Create SpecializationLambda that stores CPrime content and parent reference
        // For now, we'll store the CPrime content in lua_script field with a special marker
        ExecutableLambda specialization_lambda;
        specialization_lambda.lua_script = "SPECIALIZATION:" + cleaned_cprime_content;
        
        LOG_INFO("=== SPECIALIZATION EXEC BLOCK PREPARED ===");
        LOG_INFO("Scope Index: {}", scope_index);
        LOG_INFO("Parameters: {} items", parameters.size());
        LOG_INFO("CPrime Content Length: {} chars", cleaned_cprime_content.length());
        LOG_INFO("Status: Ready for parent script execution");
        LOG_INFO("=== END SPECIALIZATION EXEC BLOCK PREPARATION ===");
        
        // Update the registry with the specialization lambda
        exec_registry.update_executable_lambda(scope_index, specialization_lambda);
        
        LOG_INFO("Completed processing specialization exec scope {}", scope_index);
        LOG_INFO(""); // Empty line for readability
    }
    
    LOG_INFO("=== Sublayer 2B Processing Complete ===");
    LOG_INFO("Processed {} parent scopes and {} specialization scopes", parent_scopes.size(), specialization_scopes.size());
    LOG_INFO("Total exec scopes: {}", exec_registry.get_exec_scope_count());
    
    return scopes;
}

} // namespace cprime::layer2_sublayers