#include "../layer2.h"
#include "../../commons/logger.h"
#include "../../layer1/layer1.h"
#include <sstream>

namespace cprime::layer2_contextualization {

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
        
        // Pattern 1: Noname exec execution - "exec { lua_code }"
        if (token._token == EToken::EXEC) {
            info.type = ExecExecutionInfo::NONAME_EXEC;
            LOG_DEBUG("Detected noname exec execution");
            
            // TODO: Extract Lua code between braces
            // For now, placeholder
            info.inline_lua_code = "return 'noname exec not implemented yet'";
            return info;
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
const ExecutableLambda& get_executable_lambda(const ExecExecutionInfo& exec_info, const ExecAliasRegistry& exec_registry) {
    auto logger = cprime::LoggerFactory::get_logger("exec_processing");
    
    switch (exec_info.type) {
        case ExecExecutionInfo::NONAME_EXEC:
            // TODO: Create temporary ExecutableLambda with inline Lua code
            throw std::runtime_error("Noname exec execution not implemented yet");
            
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
    
    // Simple validation: check for forbidden exec constructs
    if (generated_code.find("exec") != std::string::npos) {
        LOG_ERROR("Generated code contains forbidden 'exec' construct");
        throw std::runtime_error("Generated code cannot contain exec constructs (single pass only)");
    }
    
    // Additional validation can be added here
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

// Integrate generated scopes into master scope vector
uint32_t integrate_generated_scopes(const std::vector<Scope>& generated_scopes, 
                                   std::vector<Scope>& master_scopes) {
    auto logger = cprime::LoggerFactory::get_logger("exec_processing");
    
    if (generated_scopes.empty()) {
        throw std::runtime_error("No scopes generated from exec execution");
    }
    
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

// Main exec processing function (single pass)
uint32_t process_exec_execution(const Instruction& exec_instruction,
                               std::vector<Scope>& master_scopes,
                               const StringTable& string_table,
                               ExecAliasRegistry& exec_registry,
                               const std::map<std::string, std::vector<RawToken>>& streams) {
    
    auto logger = cprime::LoggerFactory::get_logger("exec_processing");
    LOG_INFO("Processing exec execution (single pass)...");
    
    try {
        // Step 1: Extract exec execution info
        ExecExecutionInfo exec_info = extract_exec_info(exec_instruction, exec_registry, streams);
        
        // Step 2: Get ExecutableLambda and execute Lua script
        const ExecutableLambda& lambda = get_executable_lambda(exec_info, exec_registry);
        std::string generated_cprime_code = lambda.execute(exec_info.parameters);
        
        LOG_INFO("Generated CPrime code ({} chars): {}", generated_cprime_code.length(), generated_cprime_code);
        
        // Step 3: Validate generated code is pure CPrime (no exec constructs)
        validate_pure_cprime_output(generated_cprime_code);
        
        // Step 4: Tokenize generated CPrime code using Layer 1
        std::map<std::string, std::vector<RawToken>> generated_tokens = 
            tokenize_generated_code(generated_cprime_code, string_table);
        
        // Step 5: Build scopes using sublayer2a (single pass)
        std::vector<Scope> generated_scopes = layer2_sublayers::sublayer2a(
            generated_tokens, string_table, exec_registry);
        
        // Step 6: Integrate generated scopes into master scope vector
        uint32_t global_scope_index = integrate_generated_scopes(generated_scopes, master_scopes);
        
        LOG_INFO("Exec execution completed successfully - generated scope index: {}", global_scope_index);
        return global_scope_index;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exec execution failed: {}", e.what());
        throw;
    }
}

} // namespace cprime::layer2_contextualization