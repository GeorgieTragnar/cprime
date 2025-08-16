#pragma once

#include "../commons/rawToken.h"
#include "../commons/scope.h"
#include "../commons/instruction.h"
#include "../commons/dirty/string_table.h"
#include "../commons/dirty/exec_alias_registry.h"
#include <vector>
#include <map>
#include <string>

namespace cprime {

// Layer 2 main function - Structure Building
// Input: Map of file streams to RawToken vectors from Layer 1
// Output: Flat vector of structured Scopes with Instructions
std::vector<Scope> layer2(const std::map<std::string, std::vector<RawToken>>& streams, 
                         const StringTable& string_table, 
                         ExecAliasRegistry& exec_registry);

// Layer 2 sublayer implementations 
namespace layer2_sublayers {
    
    // Sublayer 2A: Pure structural scope building with cache-and-semicolon parsing
    // - Convert RawTokens to lightweight Token references
    // - Build flat scope vector with parent/child indexing
    // - Use mandatory semicolons for unambiguous parsing
    // - Register exec scopes in ExecAliasRegistry
    std::vector<Scope> sublayer2a(const std::map<std::string, std::vector<RawToken>>& streams,
                                  const StringTable& string_table,
                                  ExecAliasRegistry& exec_registry);
    
    // Sublayer 2B: Exec logic compilation and resolution
    // - Compile exec blocks to ExecutableLambda format with Lua scripts
    // - Link scope indices to executable logic
    // - Populate ExecAliasRegistry with compiled exec blocks
    void sublayer2b(std::vector<Scope>& scopes, 
                    ExecAliasRegistry& exec_registry,
                    const StringTable& string_table,
                    const std::map<std::string, std::vector<RawToken>>& streams);
    
    // Helper function to extract tokens from scope
    std::vector<Token> extract_tokens_from_scope(const Scope& scope);
}

// Internal helper structures for Sublayer 2A
namespace layer2_internal {
    
    // Token cache for accumulating tokens until boundaries
    struct TokenCache {
        std::vector<Token> cached_tokens;
        uint32_t current_stringstream_id;
        
        void add_token(const RawToken& raw_token, uint32_t token_index);
        void clear();
        bool empty() const;
        Instruction create_instruction();
    };
    
    // Scope building state machine
    struct ScopeBuilder {
        std::vector<Scope> scopes;
        uint32_t current_scope_index;
        TokenCache token_cache;
        ExecAliasRegistry& exec_registry;
        
        ScopeBuilder(ExecAliasRegistry& exec_reg) : exec_registry(exec_reg) {}
        
        void enter_scope(const Instruction& header);
        void exit_scope(const Instruction& footer);
        void add_instruction(const Instruction& instruction);
        void add_nested_scope_reference(uint32_t nested_scope_index);
        
        // Scope type detection removed - semantic analysis belongs in later layers
    };
}

} // namespace cprime