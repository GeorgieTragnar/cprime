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
    
    // Sublayer 2C: Instruction contextualization and sequential analysis
    // - Sequential iteration through all scope instructions
    // - Flat logging format for debugging and analysis
    // - No hierarchical traversal, simple scope-by-scope processing
    // - Exec execution processing and code generation (single pass)
    void sublayer2c(std::vector<Scope>& scopes, 
                    const StringTable& string_table,
                    const std::map<std::string, std::vector<RawToken>>& streams,
                    ExecAliasRegistry& exec_registry);
    
    // Helper function to extract tokens from scope
    std::vector<Token> extract_tokens_from_scope(const Scope& scope);
}

// Layer 2 contextualization functions
namespace layer2_contextualization {
    
    // Header contextualization - processes scope headers to populate _contextualTokens
    // Returns true if header contains exec execution that needs processing
    bool contextualize_header(Instruction& header_instruction);
    
    // Footer contextualization - processes scope footers to populate _contextualTokens  
    // Returns true if footer contains exec execution that needs processing
    bool contextualize_footer(Instruction& footer_instruction);
    
    // Instruction contextualization - processes body instructions to populate _contextualTokens
    // Returns true if instruction contains exec execution that needs processing
    bool contextualize_instruction(Instruction& body_instruction);
    
    // Process exec execution and return global scope index of generated code (single pass)
    uint32_t process_exec_execution(const Instruction& exec_instruction,
                                   std::vector<Scope>& master_scopes,
                                   const StringTable& string_table,
                                   ExecAliasRegistry& exec_registry,
                                   const std::map<std::string, std::vector<RawToken>>& streams,
                                   uint32_t current_scope_index,
                                   bool is_header_exec = false);
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
        bool awaiting_exec_footer = false; // Flag for exec footer collection
        
        ScopeBuilder(ExecAliasRegistry& exec_reg) : exec_registry(exec_reg) {}
        
        void enter_scope(const Instruction& header);
        void exit_scope(const Instruction& footer);
        void add_instruction(const Instruction& instruction);
        void add_nested_scope_reference(uint32_t nested_scope_index);
        
        // Scope type detection removed - semantic analysis belongs in later layers
    };
}

} // namespace cprime