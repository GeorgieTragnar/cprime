#include "layer2.h"
#include "token_detokenizer.h"

namespace cprime {

// Layer 2 main function - Structure Building
std::vector<Scope> layer2(const std::map<std::string, std::vector<RawToken>>& streams, 
                         const StringTable& string_table, 
                         ExecAliasRegistry& exec_registry) {
    // Sublayer 2A: Pure structural scope building
    auto scopes = layer2_sublayers::sublayer2a(streams, string_table, exec_registry);
    
    // Sublayer 2B: Exec logic compilation with Lua
    layer2_sublayers::sublayer2b(scopes, exec_registry, string_table, streams);
    
    return scopes;
}

namespace layer2_sublayers {

void sublayer2b(std::vector<Scope>& scopes, 
                ExecAliasRegistry& exec_registry,
                const StringTable& string_table,
                const std::map<std::string, std::vector<RawToken>>& streams) {
    
    // Iterate through all registered exec scope indices
    for (const auto& [scope_index, empty_lambda] : exec_registry.get_scope_to_lambda_map()) {
        
        // Safety check
        if (scope_index >= scopes.size()) {
            continue;
        }
        
        const Scope& exec_scope = scopes[scope_index];
        
        // Extract tokens from scope's instructions
        std::vector<Token> exec_tokens = extract_tokens_from_scope(exec_scope);
        
        // Find the corresponding raw tokens (we need the first stream for now)
        if (!streams.empty()) {
            const auto& first_stream = streams.begin()->second;
            
            // Detokenize exec tokens back to original source string (Lua code)
            std::string lua_script = TokenDetokenizer::detokenize_to_string(
                exec_tokens, string_table, first_stream);
            
            // Create ExecutableLambda with Lua script
            ExecutableLambda compiled_lambda;
            compiled_lambda.lua_script = lua_script;
            
            // Update the registry with the compiled lambda
            exec_registry.update_executable_lambda(scope_index, compiled_lambda);
        }
    }
}

std::vector<Token> extract_tokens_from_scope(const Scope& scope) {
    // Extract tokens from scope's _instructions variant
    std::vector<Token> tokens;
    
    if (std::holds_alternative<Instruction>(scope._instructions)) {
        const Instruction& instruction = std::get<Instruction>(scope._instructions);
        tokens = instruction._tokens;
    }
    // If variant contains uint32_t (nested scope index), we'd handle that here
    // For now, just handle the Instruction case
    
    return tokens;
}

} // namespace layer2_sublayers

} // namespace cprime