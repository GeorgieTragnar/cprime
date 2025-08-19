#include "../layer2.h"
#include "../../commons/logger.h"
#include "../../layer1/layer1.h"
#include <sstream>
#include <cassert>

namespace cprime::layer2_contextualization {

// Helper function to detokenize an entire scope for header template functionality
std::string detokenize_scope_for_template(const Scope& scope, 
                                          const std::vector<Scope>& all_scopes,
                                          const std::map<std::string, std::vector<RawToken>>& streams,
                                          const StringTable& string_table) {
    std::ostringstream oss;
    
    // Helper lambda to get raw token content
    auto get_token_content = [&streams, &string_table](const Token& token) -> std::string {
        // Find the token in the raw token streams
        for (const auto& [stream_name, raw_tokens] : streams) {
            if (token._tokenIndex < raw_tokens.size()) {
                const auto& raw_token = raw_tokens[token._tokenIndex];
                
                // Handle different token types
                if (std::holds_alternative<StringIndex>(raw_token._literal_value)) {
                    StringIndex str_idx = std::get<StringIndex>(raw_token._literal_value);
                    return string_table.get_string(str_idx);
                } else {
                    // For non-string tokens (operators, punctuation), return token symbol
                    switch (raw_token._token) {
                        case EToken::LEFT_BRACE: return "{";
                        case EToken::RIGHT_BRACE: return "}";
                        case EToken::LEFT_PAREN: return "(";
                        case EToken::RIGHT_PAREN: return ")";
                        case EToken::SEMICOLON: return ";";
                        case EToken::SPACE: return " ";
                        case EToken::NEWLINE: return "\n";
                        case EToken::ASSIGN: return "=";
                        case EToken::LESS_THAN: return "<";
                        case EToken::GREATER_THAN: return ">";
                        case EToken::COMMA: return ",";
                        default: return ""; // Other tokens
                    }
                }
            }
        }
        return ""; // Fallback
    };
    
    // Helper lambda to detokenize instruction
    auto detokenize_instruction = [&get_token_content](const Instruction& instruction) -> std::string {
        std::string result;
        for (const auto& token : instruction._tokens) {
            result += get_token_content(token);
        }
        return result;
    };
    
    // Detokenize header
    if (!scope._header._tokens.empty()) {
        oss << detokenize_instruction(scope._header);
    }
    
    // Detokenize body instructions
    for (const auto& instruction_variant : scope._instructions) {
        if (std::holds_alternative<Instruction>(instruction_variant)) {
            const Instruction& instruction = std::get<Instruction>(instruction_variant);
            oss << detokenize_instruction(instruction);
        } else {
            // For nested scopes, recursively detokenize them
            uint32_t nested_scope_index = std::get<uint32_t>(instruction_variant);
            if (nested_scope_index < all_scopes.size()) {
                oss << detokenize_scope_for_template(all_scopes[nested_scope_index], all_scopes, streams, string_table);
            }
        }
    }
    
    // Detokenize footer
    if (std::holds_alternative<Instruction>(scope._footer)) {
        const Instruction& footer_instruction = std::get<Instruction>(scope._footer);
        oss << detokenize_instruction(footer_instruction);
    }
    // Note: If footer is a scope index, we don't include it in template content
    // since it represents generated code, not original template content
    
    return oss.str();
}

// Helper function to extract template parameters from tokens between < >
std::vector<std::string> extract_template_parameters(const std::vector<Token>& tokens, size_t start_idx, size_t end_idx) {
    std::vector<std::string> parameters;
    
    // Simple parameter extraction - look for IDENTIFIER and STRING_LITERAL tokens
    // Skip LESS_THAN and GREATER_THAN tokens
    for (size_t i = start_idx + 1; i < end_idx; ++i) {
        const auto& token = tokens[i];
        
        if (token._token == EToken::IDENTIFIER || 
            token._token == EToken::STRING_LITERAL ||
            token._token == EToken::INT_LITERAL) {
            // For now, create placeholder parameter names
            // TODO: Extract actual parameter values from tokens
            if (token._token == EToken::IDENTIFIER) {
                parameters.push_back("identifier");
            } else if (token._token == EToken::STRING_LITERAL) {
                parameters.push_back("string_literal");
            } else if (token._token == EToken::INT_LITERAL) {
                parameters.push_back("int_literal");
            }
        }
    }
    
    return parameters;
}

// Helper structures for exec execution info
struct ExecExecutionInfo {
    enum Type { ALIAS_CALL, NONAME_EXEC, DIRECT_CALL };
    Type type;
    std::string alias_name;                    // For alias calls
    std::vector<std::string> parameters;       // Type parameters
    std::string inline_lua_code;              // For noname exec
};

// Extract exec execution information from instruction tokens
ExecExecutionInfo extract_exec_info(const Instruction& exec_instruction, 
                                    const ExecAliasRegistry& exec_registry,
                                    const std::map<std::string, std::vector<RawToken>>& streams) {
    ExecExecutionInfo info;
    const auto& tokens = exec_instruction._tokens;
    
    auto logger = cprime::LoggerFactory::get_logger("exec_processing");
    
    if (tokens.empty()) {
        throw std::runtime_error("Empty instruction passed to extract_exec_info");
    }
    
    // Scan through all tokens to find exec patterns (skip whitespace/comments)
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        
        // Pattern 1: Noname exec declaration - "exec <params> { lua_code }" (should not be processed here)
        if (token._token == EToken::EXEC) {
            // This is exec declaration header, not execution
            // Skip this - we're looking for the execution pattern, not the declaration
            // The execution pattern is just "<args>" without "exec" keyword
            LOG_DEBUG("Skipping EXEC token - this is declaration, not execution");
            continue;
        }
        
        // Pattern 1b: Noname exec footer execution - "<args>"
        if (token._token == EToken::LESS_THAN) {
            // Look for matching GREATER_THAN to confirm <args> pattern
            for (size_t j = i + 1; j < tokens.size(); ++j) {
                if (tokens[j]._token == EToken::GREATER_THAN) {
                    info.type = ExecExecutionInfo::NONAME_EXEC;
                    LOG_INFO("Detected noname exec footer execution: <args>");
                    
                    // Extract parameters between < >
                    info.parameters = extract_template_parameters(tokens, i, j);
                    LOG_INFO("Extracted {} parameters from footer", info.parameters.size());
                    
                    // For noname exec, we need to find the corresponding exec block
                    // This will be handled in get_executable_lambda by looking up the current scope
                    info.alias_name = "NONAME_EXEC";  // Special marker for noname exec
                    return info;
                }
            }
        }
        
        // Pattern 2: Exec alias call - "EXEC_ALIAS<params>()"
        if (token._token == EToken::EXEC_ALIAS) {
            info.type = ExecExecutionInfo::ALIAS_CALL;
            
            LOG_INFO("Detected exec alias call at token index {}", i);
            
            // Extract alias name from EXEC_ALIAS token's literal value
            // Use token index to look up original RawToken
            uint32_t raw_token_index = token._tokenIndex;
            
            // Find the raw token in the streams (assume first stream for now)
            const std::vector<RawToken>* raw_tokens = nullptr;
            for (const auto& [stream_name, tokens_vec] : streams) {
                if (!tokens_vec.empty()) {
                    raw_tokens = &tokens_vec;
                    break;
                }
            }
            
            if (!raw_tokens || raw_token_index >= raw_tokens->size()) {
                throw std::runtime_error("Cannot find RawToken for EXEC_ALIAS token");
            }
            
            const auto& raw_token = (*raw_tokens)[raw_token_index];
            if (std::holds_alternative<ExecAliasIndex>(raw_token._literal_value)) {
                ExecAliasIndex alias_idx = std::get<ExecAliasIndex>(raw_token._literal_value);
                info.alias_name = exec_registry.get_alias(alias_idx);
                LOG_INFO("Extracted alias name: '{}'", info.alias_name);
            } else {
                throw std::runtime_error("EXEC_ALIAS RawToken does not contain ExecAliasIndex");
            }
            
            // TODO: Extract parameters between < >
            info.parameters = {"int", "string"}; // Hardcoded for now - should extract from < >
            
            LOG_INFO("Exec alias info: type=ALIAS_CALL, name='{}', params={}", 
                     info.alias_name, info.parameters.size());
            return info;
        }
        
        // Pattern 3: Direct identifier call - "identifier<params>()"
        if (token._token == EToken::IDENTIFIER && i + 1 < tokens.size() &&
            tokens[i + 1]._token == EToken::LESS_THAN) {
            // TODO: Extract identifier name and check if it exists in exec registry
            // TODO: Extract parameters between < >
            
            info.type = ExecExecutionInfo::DIRECT_CALL;
            info.alias_name = "code_gen";         // Placeholder
            info.parameters = {"int", "string"};  // Placeholder
            
            LOG_DEBUG("Detected direct identifier exec call: {}", info.alias_name);
            return info;
        }
    }
    
    throw std::runtime_error("Unknown exec execution pattern in extract_exec_info");
}

// Get ExecutableLambda based on exec execution info
const ExecutableLambda& get_executable_lambda(const ExecExecutionInfo& exec_info, const ExecAliasRegistry& exec_registry, uint32_t current_scope_index) {
    auto logger = cprime::LoggerFactory::get_logger("exec_processing");
    
    switch (exec_info.type) {
        case ExecExecutionInfo::NONAME_EXEC:
            // For noname exec, the execution scope contains <args> but the exec block
            // is in the preceding scope. Look for exec block in previous scopes.
            LOG_INFO("Looking up noname exec for execution scope {}", current_scope_index);
            
            // Try current scope first
            try {
                return exec_registry.get_executable_lambda(current_scope_index);
            } catch (const std::exception&) {
                LOG_DEBUG("No exec block in current scope {}, trying previous scopes", current_scope_index);
            }
            
            // Try previous scopes (typically the immediately preceding scope)
            for (int32_t scope_idx = static_cast<int32_t>(current_scope_index) - 1; scope_idx >= 0; --scope_idx) {
                try {
                    LOG_DEBUG("Trying scope {} for noname exec block", scope_idx);
                    return exec_registry.get_executable_lambda(static_cast<uint32_t>(scope_idx));
                } catch (const std::exception&) {
                    LOG_DEBUG("No exec block in scope {}", scope_idx);
                    continue;
                }
            }
            
            throw std::runtime_error("No exec block found in current or previous scopes for noname execution");
            
        case ExecExecutionInfo::ALIAS_CALL:
        case ExecExecutionInfo::DIRECT_CALL:
            // Debug: Log registry state
            LOG_INFO("Looking for exec alias: '{}'", exec_info.alias_name);
            LOG_INFO("Registry has {} aliases, {} scopes, {} mappings", 
                     exec_registry.size(), 
                     exec_registry.get_exec_scope_count(),
                     exec_registry.get_alias_to_scope_count());
            
            // Get ExecutableLambda by alias name
            if (!exec_registry.contains_alias(exec_info.alias_name)) {
                throw std::runtime_error("Exec alias not found: " + exec_info.alias_name);
            }
            
            ExecAliasIndex alias_idx = exec_registry.get_alias_index(exec_info.alias_name);
            return exec_registry.get_executable_lambda_by_alias(alias_idx);
    }
    
    throw std::runtime_error("Invalid exec execution type");
}

// Validate that generated code contains no exec constructs (single pass enforcement)
void validate_pure_cprime_output(const std::string& generated_code) {
    auto logger = cprime::LoggerFactory::get_logger("exec_processing");
    
    // More sophisticated validation: check for actual exec keywords, not just string content
    // Look for "exec" as a keyword (not inside string literals)
    bool in_string = false;
    char quote_char = '\0';
    
    for (size_t i = 0; i < generated_code.length(); ++i) {
        char c = generated_code[i];
        
        // Handle string literals
        if (!in_string && (c == '"' || c == '\'')) {
            in_string = true;
            quote_char = c;
            continue;
        }
        if (in_string && c == quote_char) {
            in_string = false;
            quote_char = '\0';
            continue;
        }
        
        // Skip checking inside string literals
        if (in_string) continue;
        
        // Check for "exec" keyword outside of strings
        if (i + 4 <= generated_code.length() && 
            generated_code.substr(i, 4) == "exec" &&
            (i == 0 || !std::isalnum(generated_code[i-1])) &&
            (i + 4 >= generated_code.length() || !std::isalnum(generated_code[i+4]))) {
            LOG_ERROR("Generated code contains forbidden 'exec' keyword at position {}", i);
            throw std::runtime_error("Generated code cannot contain exec constructs (single pass only)");
        }
    }
    
    LOG_DEBUG("Generated code validation passed - pure CPrime output confirmed");
}

// Tokenize generated CPrime code using Layer 1
std::map<std::string, std::vector<RawToken>> tokenize_generated_code(const std::string& generated_code, 
                                                                     const StringTable& string_table) {
    auto logger = cprime::LoggerFactory::get_logger("exec_processing");
    
    // Create temporary string stream for generated code
    std::stringstream generated_stream(generated_code);
    
    // Use Layer 1 tokenization
    ExecAliasRegistry temp_registry;  // Temporary for tokenization (no exec aliases in generated code)
    auto tokens = layer1(generated_stream, const_cast<StringTable&>(string_table), temp_registry);
    
    // Package into expected format
    std::map<std::string, std::vector<RawToken>> token_streams;
    token_streams["generated_code"] = tokens;
    
    LOG_DEBUG("Tokenized generated code: {} tokens", tokens.size());
    return token_streams;
}

// Find the range of tokens that make up an exec alias call (alias<params>())
bool find_exec_alias_range(const Instruction& exec_instruction, int& start_pos, int& end_pos) {
    auto logger = cprime::LoggerFactory::get_logger("exec_processing");
    
    // Look for the pattern: EXEC_ALIAS < ... > ( ... )
    // We need to find the EXEC_ALIAS token and then find the matching closing parenthesis
    
    for (size_t i = 0; i < exec_instruction._tokens.size(); ++i) {
        if (exec_instruction._tokens[i]._token == EToken::EXEC_ALIAS) {
            start_pos = static_cast<int>(i);
            
            // Look for the closing pattern - we need to find the matching closing parenthesis
            // Pattern: EXEC_ALIAS < params > ( args )
            //                    ^               ^
            //                  start           end
            
            int paren_depth = 0;
            int angle_depth = 0;
            bool found_opening_paren = false;
            
            for (size_t j = i + 1; j < exec_instruction._tokens.size(); ++j) {
                EToken token = exec_instruction._tokens[j]._token;
                
                if (token == EToken::LESS_THAN) {
                    angle_depth++;
                } else if (token == EToken::GREATER_THAN) {
                    angle_depth--;
                } else if (token == EToken::LEFT_PAREN && angle_depth == 0) {
                    paren_depth++;
                    found_opening_paren = true;
                } else if (token == EToken::RIGHT_PAREN && angle_depth == 0) {
                    paren_depth--;
                    if (paren_depth == 0 && found_opening_paren) {
                        end_pos = static_cast<int>(j);
                        LOG_DEBUG("Found complete exec alias call from token {} to {}", start_pos, end_pos);
                        return true;
                    }
                }
            }
            
            // If we found an EXEC_ALIAS but no matching closing parenthesis, this might be a malformed call
            LOG_WARN("Found EXEC_ALIAS token at {} but no matching closing parenthesis", i);
            
            // Fallback: just use the EXEC_ALIAS token itself
            start_pos = static_cast<int>(i);
            end_pos = static_cast<int>(i);
            return true;
        }
    }
    
    LOG_ERROR("No EXEC_ALIAS token found in instruction");
    return false;
}

// Perform direct token substitution: replace exec alias with generated tokens
bool perform_token_substitution(const Instruction& exec_instruction,
                                const std::vector<RawToken>& replacement_tokens,
                                std::vector<Scope>& master_scopes,
                                uint32_t current_scope_index,
                                const StringTable& string_table) {
    
    auto logger = cprime::LoggerFactory::get_logger("exec_processing");
    
    assert(current_scope_index < master_scopes.size() && "Invalid scope index for token substitution");
    
    Scope& target_scope = master_scopes[current_scope_index];
    
    // Find the instruction that contains the exec alias in the target scope
    // We need to search through all instructions in the scope
    for (auto& instruction_variant : target_scope._instructions) {
        if (std::holds_alternative<Instruction>(instruction_variant)) {
            Instruction& instruction = std::get<Instruction>(instruction_variant);
            
            // Check if this is the exec instruction we're looking for
            if (&instruction == &exec_instruction) {
                LOG_INFO("Found exec instruction to modify in scope {}", current_scope_index);
                
                // Find the exec alias token in the instruction
                for (size_t i = 0; i < instruction._tokens.size(); ++i) {
                    if (instruction._tokens[i]._token == EToken::EXEC_ALIAS) {
                        LOG_INFO("Found EXEC_ALIAS token at position {} in instruction", i);
                        
                        // TODO: Find the complete exec alias call pattern (including <> and ())
                        // For now, just replace the single EXEC_ALIAS token
                        
                        // Create new tokens vector with replacement
                        std::vector<Token> new_tokens;
                        
                        // Add tokens before the exec alias
                        for (size_t j = 0; j < i; ++j) {
                            new_tokens.push_back(instruction._tokens[j]);
                        }
                        
                        // Add the replacement tokens (converted from RawToken to Token)
                        LOG_INFO("Inserting {} replacement tokens at position {}", replacement_tokens.size(), i);
                        for (size_t rt_idx = 0; rt_idx < replacement_tokens.size(); ++rt_idx) {
                            const RawToken& raw_token = replacement_tokens[rt_idx];
                            
                            // Create Token from RawToken
                            Token replacement_token;
                            replacement_token._token = raw_token._token;
                            replacement_token._stringstreamId = 0; // Generated tokens use stream 0
                            replacement_token._tokenIndex = rt_idx; // Index within replacement tokens
                            
                            new_tokens.push_back(replacement_token);
                            
                            LOG_DEBUG("Added replacement token {} at position {}: type={}", 
                                     rt_idx, new_tokens.size() - 1, static_cast<uint32_t>(raw_token._token));
                        }
                        
                        // Add tokens after the exec alias (skip the exec alias itself)
                        for (size_t j = i + 1; j < instruction._tokens.size(); ++j) {
                            new_tokens.push_back(instruction._tokens[j]);
                        }
                        
                        // Replace the instruction's tokens
                        instruction._tokens = new_tokens;
                        
                        LOG_INFO("✅ Successfully replaced EXEC_ALIAS token with {} replacement tokens", replacement_tokens.size());
                        LOG_INFO("Instruction now has {} total tokens (was {} before)", new_tokens.size(), instruction._tokens.size());
                        
                        return true;
                    }
                }
                
                LOG_WARN("No EXEC_ALIAS token found in the target instruction");
                return false;
            }
        }
    }
    
    LOG_WARN("Could not find the exec instruction in scope {} for token substitution", current_scope_index);
    return false;
}

// Integrate generated scopes into master scope vector
uint32_t integrate_generated_scopes(const std::vector<Scope>& generated_scopes, 
                                   std::vector<Scope>& master_scopes) {
    auto logger = cprime::LoggerFactory::get_logger("exec_processing");
    
    assert(!generated_scopes.empty() && "Cannot integrate empty generated scopes");
    
    uint32_t global_scope_index = static_cast<uint32_t>(master_scopes.size());
    
    // Update parent scope indices to reference master scope vector
    std::vector<Scope> adjusted_scopes = generated_scopes;
    for (auto& scope : adjusted_scopes) {
        if (scope._parentScopeIndex == 0) {  // If it was global scope reference
            scope._parentScopeIndex = global_scope_index;  // Point to new global
        } else {
            scope._parentScopeIndex += global_scope_index;  // Offset all references
        }
    }
    
    // Append generated scopes to master scope vector
    master_scopes.insert(master_scopes.end(), adjusted_scopes.begin(), adjusted_scopes.end());
    
    LOG_INFO("Integrated {} generated scopes starting at index {}", adjusted_scopes.size(), global_scope_index);
    return global_scope_index;  // Return index of the generated global scope
}

// Token Integration Handler: Direct token substitution
uint32_t handle_token_integration(const ExecResult& exec_result, 
                                  const Instruction& exec_instruction,
                                  std::vector<Scope>& master_scopes,
                                  const StringTable& string_table,
                                  const std::map<std::string, std::vector<RawToken>>& streams,
                                  uint32_t current_scope_index) {
    
    auto logger = cprime::LoggerFactory::get_logger("exec_processing");
    LOG_INFO("Handling token integration for generated code: {}", exec_result.generated_code);
    
    // Validate generated code is pure CPrime (no exec constructs)
    validate_pure_cprime_output(exec_result.generated_code);
    
    // Tokenize generated CPrime code using Layer 1
    std::map<std::string, std::vector<RawToken>> generated_tokens = 
        tokenize_generated_code(exec_result.generated_code, string_table);
    
    // Extract the generated raw tokens (should be from single stream)
    std::vector<RawToken> replacement_tokens;
    for (const auto& [stream_name, raw_tokens] : generated_tokens) {
        replacement_tokens = raw_tokens;
        break; // Take first (and should be only) stream
    }
    
    LOG_INFO("Generated {} replacement tokens from: {}", replacement_tokens.size(), exec_result.generated_code);
    
    // Perform direct token substitution in the current scope's instruction
    bool substitution_performed = perform_token_substitution(exec_instruction, replacement_tokens, 
                                                           master_scopes, current_scope_index, string_table);
    
    if (substitution_performed) {
        LOG_INFO("✅ Token substitution completed - replaced exec alias with {} tokens", replacement_tokens.size());
        // Return current scope index since we modified in-place, no new scope created
        return current_scope_index;
    } else {
        LOG_ERROR("❌ Token substitution failed - falling back to scope creation");
        
        // Fallback to scope creation if direct substitution fails
        ExecAliasRegistry temp_registry;
        std::vector<Scope> generated_scopes = layer2_sublayers::sublayer2a(generated_tokens, string_table, temp_registry);
        uint32_t fallback_scope_index = integrate_generated_scopes(generated_scopes, master_scopes);
        
        LOG_INFO("Fallback scope creation completed - generated scope index: {}", fallback_scope_index);
        return fallback_scope_index;
    }
}

// Scope Insert Integration Handler: Transform instruction to scope with header/footer
uint32_t handle_scope_insert_integration(const ExecResult& exec_result, 
                                         const Instruction& exec_instruction,
                                         std::vector<Scope>& master_scopes,
                                         const StringTable& string_table,
                                         ExecAliasRegistry& exec_registry,
                                         const std::map<std::string, std::vector<RawToken>>& streams,
                                         uint32_t current_scope_index) {
    
    auto logger = cprime::LoggerFactory::get_logger("exec_processing");
    LOG_INFO("Handling scope_insert integration for generated code: {}", exec_result.generated_code);
    
    // Validate generated code is pure CPrime (no exec constructs)
    validate_pure_cprime_output(exec_result.generated_code);
    
    // Step 1: Find the exec alias position in the instruction
    int exec_alias_start = -1, exec_alias_end = -1;
    if (!find_exec_alias_range(exec_instruction, exec_alias_start, exec_alias_end)) {
        LOG_ERROR("Could not find exec alias range in instruction for scope insertion");
        return UINT32_MAX; // Signal error
    }
    
    LOG_INFO("Found exec alias at token range [{}, {}] in instruction", exec_alias_start, exec_alias_end);
    
    // Step 2: Extract header tokens (before exec alias)
    std::vector<Token> header_tokens;
    for (int i = 0; i < exec_alias_start; ++i) {
        header_tokens.push_back(exec_instruction._tokens[i]);
    }
    
    // Step 3: Extract footer tokens (after exec alias call)
    std::vector<Token> footer_tokens;
    for (size_t i = exec_alias_end + 1; i < exec_instruction._tokens.size(); ++i) {
        footer_tokens.push_back(exec_instruction._tokens[i]);
    }
    
    LOG_INFO("Extracted {} header tokens and {} footer tokens", header_tokens.size(), footer_tokens.size());
    
    // Step 4: Tokenize generated code for the body
    std::map<std::string, std::vector<RawToken>> generated_tokens = 
        tokenize_generated_code(exec_result.generated_code, string_table);
    
    // Step 5: Build scopes from generated code
    std::vector<Scope> generated_scopes = layer2_sublayers::sublayer2a(generated_tokens, string_table, exec_registry);
    
    // Step 6: Create the new scope with header/body/footer structure
    Scope new_scope;
    new_scope._parentScopeIndex = current_scope_index;
    
    // Set header instruction
    if (!header_tokens.empty()) {
        new_scope._header._tokens = header_tokens;
        // _contextualTokens is a vector, not a count - initialize empty for now
        new_scope._header._contextualTokens.clear();
        LOG_INFO("Created header with {} tokens", header_tokens.size());
    } else {
        LOG_DEBUG("No header tokens - header will be empty");
    }
    
    // Set body instructions from generated scopes
    if (!generated_scopes.empty()) {
        // Add generated scopes as nested scopes in the body
        uint32_t base_scope_index = static_cast<uint32_t>(master_scopes.size()) + 1; // +1 for the new scope itself
        
        for (size_t i = 0; i < generated_scopes.size(); ++i) {
            // Add generated scope to master scopes (will be added after this scope)
            uint32_t nested_scope_index = base_scope_index + static_cast<uint32_t>(i);
            new_scope._instructions.emplace_back(nested_scope_index);
            
            LOG_DEBUG("Added nested scope {} to body instructions", nested_scope_index);
        }
        
        LOG_INFO("Created body with {} nested scopes", generated_scopes.size());
    } else {
        LOG_WARN("No generated scopes - body will be empty");
    }
    
    // Set footer instruction
    if (!footer_tokens.empty()) {
        Instruction footer_instruction;
        footer_instruction._tokens = footer_tokens;
        // _contextualTokens is a vector, not a count - initialize empty for now
        footer_instruction._contextualTokens.clear();
        new_scope._footer = footer_instruction;
        LOG_INFO("Created footer with {} tokens", footer_tokens.size());
    } else {
        LOG_DEBUG("No footer tokens - footer will be empty");
    }
    
    // Step 7: Add the new scope and generated scopes to master scopes
    uint32_t new_scope_index = static_cast<uint32_t>(master_scopes.size());
    master_scopes.push_back(new_scope);
    
    // Add the generated scopes after the new scope
    for (auto& generated_scope : generated_scopes) {
        // Adjust parent scope indices to point to the new scope
        generated_scope._parentScopeIndex = new_scope_index;
        master_scopes.push_back(generated_scope);
    }
    
    LOG_INFO("✅ Scope insertion completed - created scope {} with {} nested generated scopes", 
             new_scope_index, generated_scopes.size());
    
    return new_scope_index;
}

// Scope Create Integration Handler: Create new function/class scope + identifier substitution
uint32_t handle_scope_create_integration(const ExecResult& exec_result, 
                                         const Instruction& exec_instruction,
                                         std::vector<Scope>& master_scopes,
                                         const StringTable& string_table,
                                         ExecAliasRegistry& exec_registry,
                                         const std::map<std::string, std::vector<RawToken>>& streams,
                                         uint32_t current_scope_index) {
    
    auto logger = cprime::LoggerFactory::get_logger("exec_processing");
    LOG_INFO("Handling scope_create integration - generated code: {}", exec_result.generated_code);
    LOG_INFO("Identifier for substitution: '{}'", exec_result.identifier);
    
    if (exec_result.identifier.empty()) {
        LOG_ERROR("scope_create integration requires non-empty identifier");
        return UINT32_MAX; // Signal error - invalid scope index
    }
    
    // Validate generated code is pure CPrime (no exec constructs)
    validate_pure_cprime_output(exec_result.generated_code);
    
    // Step 1: Tokenize generated code
    std::map<std::string, std::vector<RawToken>> generated_tokens = 
        tokenize_generated_code(exec_result.generated_code, string_table);
    
    // Step 2: Build scopes from generated code
    std::vector<Scope> generated_scopes = layer2_sublayers::sublayer2a(generated_tokens, string_table, exec_registry);
    
    if (generated_scopes.empty()) {
        LOG_ERROR("No scopes generated from code: {}", exec_result.generated_code);
        return UINT32_MAX; // Signal error
    }
    
    // Step 3: Add generated scopes to master scope list
    uint32_t generated_scope_index = static_cast<uint32_t>(master_scopes.size());
    
    for (auto& generated_scope : generated_scopes) {
        // Set parent to current scope
        generated_scope._parentScopeIndex = current_scope_index;
        master_scopes.push_back(generated_scope);
    }
    
    LOG_INFO("Added {} generated scopes starting at index {}", generated_scopes.size(), generated_scope_index);
    
    // Step 4: Create identifier token for substitution
    // We need to create a raw token for the identifier and tokenize it properly
    std::string identifier_code = exec_result.identifier;
    
    // Tokenize the identifier 
    std::map<std::string, std::vector<RawToken>> identifier_tokens = 
        tokenize_generated_code(identifier_code, string_table);
    
    // Extract the identifier tokens (should be a single IDENTIFIER token)
    std::vector<RawToken> replacement_tokens;
    for (const auto& [stream_name, raw_tokens] : identifier_tokens) {
        replacement_tokens = raw_tokens;
        break; // Take first (and should be only) stream
    }
    
    if (replacement_tokens.empty()) {
        LOG_ERROR("Failed to tokenize identifier: {}", exec_result.identifier);
        return UINT32_MAX; // Signal error
    }
    
    LOG_INFO("Generated {} identifier tokens for: {}", replacement_tokens.size(), exec_result.identifier);
    
    // Step 5: Perform identifier substitution at call site
    bool substitution_performed = perform_token_substitution(exec_instruction, replacement_tokens, 
                                                           master_scopes, current_scope_index, string_table);
    
    if (!substitution_performed) {
        LOG_ERROR("Failed to substitute exec alias with identifier: {}", exec_result.identifier);
        return UINT32_MAX; // Signal error
    }
    
    LOG_INFO("✅ Scope creation completed - created {} scopes starting at {} and substituted exec alias with '{}'", 
             generated_scopes.size(), generated_scope_index, exec_result.identifier);
    
    return generated_scope_index;
}

// Main exec processing function (single pass)
uint32_t process_exec_execution(const Instruction& exec_instruction,
                               std::vector<Scope>& master_scopes,
                               const StringTable& string_table,
                               ExecAliasRegistry& exec_registry,
                               const std::map<std::string, std::vector<RawToken>>& streams,
                               uint32_t current_scope_index,
                               bool is_header_exec) {
    
    auto logger = cprime::LoggerFactory::get_logger("exec_processing");
    LOG_INFO("Processing exec execution (single pass)...");
    
    try {
        // Step 1: Extract exec execution info
        ExecExecutionInfo exec_info = extract_exec_info(exec_instruction, exec_registry, streams);
        
        // Step 2: Prepare parameters for Lua execution
        std::vector<std::string> lua_parameters = exec_info.parameters;
        
        // For header exec processing: prepend detokenized scope as first parameter
        if (is_header_exec) {
            LOG_INFO("Header exec processing: detokenizing scope {} for template", current_scope_index);
            std::string scope_content = detokenize_scope_for_template(
                master_scopes[current_scope_index], master_scopes, streams, string_table);
            
            // Insert scope content as first parameter, shift others
            lua_parameters.insert(lua_parameters.begin(), scope_content);
            LOG_INFO("Header exec: scope content ({} chars) added as first parameter", scope_content.length());
        }
        
        // Step 3: Get ExecutableLambda and execute Lua script
        ExecutableLambda& lambda = const_cast<ExecutableLambda&>(get_executable_lambda(exec_info, exec_registry, current_scope_index));
        ExecResult exec_result = lambda.execute(lua_parameters);
        
        LOG_INFO("Generated CPrime code ({} chars, type: {}): {}", 
                 exec_result.generated_code.length(), exec_result.integration_type, exec_result.generated_code);
        
        if (!exec_result.is_valid) {
            LOG_ERROR("Exec execution returned invalid result: {}", exec_result.generated_code);
            return UINT32_MAX; // Signal error - invalid scope index
        }
        
        // Step 3: Handle integration based on type
        if (exec_result.integration_type == "token") {
            return handle_token_integration(exec_result, exec_instruction, master_scopes, 
                                           string_table, streams, current_scope_index);
        } else if (exec_result.integration_type == "scope_insert") {
            return handle_scope_insert_integration(exec_result, exec_instruction, master_scopes, 
                                                  string_table, exec_registry, streams, current_scope_index);
        } else if (exec_result.integration_type == "scope_create") {
            return handle_scope_create_integration(exec_result, exec_instruction, master_scopes, 
                                                  string_table, exec_registry, streams, current_scope_index);
        } else {
            LOG_ERROR("Unknown integration type: {}", exec_result.integration_type);
            return UINT32_MAX; // Signal error - invalid scope index
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exec execution failed: {}", e.what());
        throw;
    }
}

} // namespace cprime::layer2_contextualization