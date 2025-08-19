#pragma once

#include "../commons/rawToken.h"
#include "../commons/scope.h"
#include "../commons/instruction.h"
#include "../commons/dirty/string_table.h"
#include "../commons/dirty/exec_alias_registry.h"
#include "../commons/contextualizationError.h"
#include <vector>
#include <map>
#include <string>

namespace cprime {

// Forward declarations
class ErrorHandler;

// Layer 2 main function - Structure Building
// Input: Map of file streams to RawToken vectors from Layer 1
// Output: Flat vector of structured Scopes with Instructions
// Note: ErrorHandler will be used internally for contextualization error reporting
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
    std::vector<Scope> sublayer2b(const std::vector<Scope>& scopes, 
                                  ExecAliasRegistry& exec_registry,
                                  const StringTable& string_table,
                                  const std::map<std::string, std::vector<RawToken>>& streams);
    
    // Sublayer 2C: Namespace-aware CHUNK token disambiguation
    // - Traverse scopes hierarchically with namespace context inheritance
    // - Resolve CHUNK tokens to KEYWORD/IDENTIFIER/EXEC_ALIAS based on namespace
    // - Replace CHUNK tokens in-place while preserving source correlation
    std::vector<Scope> sublayer2c(const std::vector<Scope>& scopes,
                                  const StringTable& string_table,
                                  std::map<std::string, std::vector<RawToken>>& streams,
                                  ExecAliasRegistry& exec_registry);
    
    // Sublayer 2D: Instruction contextualization and sequential analysis
    // - Sequential iteration through all scope instructions
    // - Flat logging format for debugging and analysis
    // - No hierarchical traversal, simple scope-by-scope processing
    // - Exec execution processing and code generation (single pass)
    // - Error handling with ErrorReporter lambdas for contextualization
    std::vector<Scope> sublayer2d(const std::vector<Scope>& scopes, 
                                  const StringTable& string_table,
                                  const std::map<std::string, std::vector<RawToken>>& streams,
                                  ExecAliasRegistry& exec_registry,
                                  ErrorHandler& error_handler);
    
    // Helper function to extract tokens from scope
    std::vector<Token> extract_tokens_from_scope(const Scope& scope);
}

// Layer 2 contextualization functions
namespace layer2_contextualization {
    
    // Structure for header exec alias information extraction
    struct HeaderExecAliasInfo {
        bool is_header_exec = false;
        std::string base_alias_name;                    // e.g., "scope_analyzer"
        std::vector<std::string> template_parameters;   // e.g., {"detailed"}
        std::vector<std::string> namespace_and_alias;   // e.g., {"current_namespace", "scope_analyzer", "detailed"}
    };
    
    // Extract header exec alias information for namespace registration
    HeaderExecAliasInfo extract_header_exec_alias_info(const Instruction& header_instruction, uint32_t scope_index = 0);
    
    // Header contextualization - processes scope headers to populate _contextualTokens
    // Returns true if header contains exec execution that needs processing
    bool contextualize_header(Instruction& header_instruction, ErrorReporter report_error);
    
    // Footer contextualization - processes scope footers to populate _contextualTokens  
    // Returns true if footer contains exec execution that needs processing
    bool contextualize_footer(Instruction& footer_instruction, ErrorReporter report_error);
    
    // Instruction contextualization - processes body instructions to populate _contextualTokens
    // Returns true if instruction contains exec execution that needs processing
    bool contextualize_instruction(Instruction& body_instruction, ErrorReporter report_error);
    
    // Result structure for exec processing that includes both scope index and exec result
    struct ExecProcessingResult {
        uint32_t generated_scope_index;
        ExecResult exec_result;
        bool is_valid;
        
        ExecProcessingResult() : generated_scope_index(UINT32_MAX), is_valid(false) {}
        ExecProcessingResult(uint32_t scope_idx, const ExecResult& result) 
            : generated_scope_index(scope_idx), exec_result(result), is_valid(true) {}
    };
    
    // Process exec execution and return both scope index and exec result (single pass)
    ExecProcessingResult process_exec_execution_with_result(const Instruction& exec_instruction,
                                                           std::vector<Scope>& master_scopes,
                                                           const StringTable& string_table,
                                                           ExecAliasRegistry& exec_registry,
                                                           const std::map<std::string, std::vector<RawToken>>& streams,
                                                           uint32_t current_scope_index,
                                                           bool is_header_exec = false);
    
    // Legacy function - maintained for backward compatibility
    uint32_t process_exec_execution(const Instruction& exec_instruction,
                                   std::vector<Scope>& master_scopes,
                                   const StringTable& string_table,
                                   ExecAliasRegistry& exec_registry,
                                   const std::map<std::string, std::vector<RawToken>>& streams,
                                   uint32_t current_scope_index,
                                   bool is_header_exec = false);
    
    // Post-exec re-contextualization based on integration type
    void handle_post_exec_recontextualization(
        std::vector<Scope>& scopes,
        size_t original_scope_index,
        size_t original_instruction_index,
        InstructionType instruction_type,
        const ExecProcessingResult& exec_processing_result,
        const StringTable& string_table,
        const std::map<std::string, std::vector<RawToken>>& streams,
        ExecAliasRegistry& exec_registry,
        ErrorHandler& error_handler
    );
    
    // Process single scope contextualization (recursive helper)
    void process_scope_contextualization(
        std::vector<Scope>& scopes,
        size_t scope_index,
        const StringTable& string_table,
        const std::map<std::string, std::vector<RawToken>>& streams,
        ExecAliasRegistry& exec_registry,
        ErrorHandler& error_handler
    );
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
        const std::map<std::string, std::vector<RawToken>>& streams;
        const StringTable& string_table;
        bool awaiting_exec_footer = false; // Flag for exec footer collection
        
        ScopeBuilder(ExecAliasRegistry& exec_reg, const std::map<std::string, std::vector<RawToken>>& input_streams, const StringTable& str_table) 
            : exec_registry(exec_reg), streams(input_streams), string_table(str_table) {}
        
        void enter_scope(const Instruction& header);
        void exit_scope(const Instruction& footer);
        void add_instruction(const Instruction& instruction);
        void add_nested_scope_reference(uint32_t nested_scope_index);
        
        // Scope type detection removed - semantic analysis belongs in later layers
    };
}

} // namespace cprime