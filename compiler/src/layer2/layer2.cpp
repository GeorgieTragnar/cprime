#include "layer2.h"
#include "token_detokenizer.h"

namespace cprime {

// Layer 2 main function - Structure Building
std::vector<Scope> layer2(const std::map<std::string, std::vector<RawToken>>& streams, 
                         const StringTable& string_table, 
                         ExecAliasRegistry& exec_registry) {
    // Sublayer 2A: Pure structural scope building
    auto retVal1 = layer2_sublayers::sublayer2a(streams, string_table, exec_registry);
    
    // Sublayer 2B: Exec logic compilation with Lua
    auto retVal2 = layer2_sublayers::sublayer2b(retVal1, exec_registry, string_table, streams);
    
    // Sublayer 2C: Instruction contextualization and analysis
    auto retVal3 = layer2_sublayers::sublayer2c(retVal2, string_table, streams, exec_registry);
    
    return retVal3;
}

namespace layer2_sublayers {

// Forward declarations - implementations are in separate files
std::vector<Scope> sublayer2b(const std::vector<Scope>& scopes, 
                              ExecAliasRegistry& exec_registry,
                              const StringTable& string_table,
                              const std::map<std::string, std::vector<RawToken>>& streams);

std::vector<Scope> sublayer2c(const std::vector<Scope>& scopes, 
                              const StringTable& string_table,
                              const std::map<std::string, std::vector<RawToken>>& streams,
                              ExecAliasRegistry& exec_registry);

std::vector<Token> extract_tokens_from_scope(const Scope& scope) {
    // Extract tokens from scope's _instructions vector of variants
    std::vector<Token> tokens;
    
    // Iterate through all instruction variants in the vector
    for (const auto& instruction_variant : scope._instructions) {
        if (std::holds_alternative<Instruction>(instruction_variant)) {
            const Instruction& instruction = std::get<Instruction>(instruction_variant);
            // Append tokens from this instruction to the result
            tokens.insert(tokens.end(), instruction._tokens.begin(), instruction._tokens.end());
        }
        // If variant contains uint32_t (nested scope index), we'd handle that here
        // For now, just handle the Instruction case
    }
    
    return tokens;
}

} // namespace layer2_sublayers

} // namespace cprime