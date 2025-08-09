#pragma once

#include "../common/structural_types.h"
#include "../common/token_types.h"
#include "scope_destructor_tracker.h"
#include "defer_validation.h"
#include <vector>
#include <stack>
#include <unordered_map>

namespace cprime {

/**
 * RAIIFlowAnalyzer - Layer 4 processor for RAII flow analysis and defer processing.
 * 
 * Takes contextualized StructuredTokens from Layer 3 and processes defer statements,
 * generating appropriate cleanup sequences for all exit points within function scopes.
 * 
 * Key responsibilities:
 * 1. Identify function scopes and build variable tracking
 * 2. Process defer statements with bump-to-front reordering
 * 3. Validate conditional defer patterns
 * 4. Inject cleanup token sequences at return points and scope exits
 * 5. Maintain dual deque inheritance for proper cleanup ordering
 */
class RAIIFlowAnalyzer {
public:
    /**
     * Process StructuredTokens from Layer 3, adding RAII cleanup sequences.
     * 
     * @param input StructuredTokens from Layer 3 (must be contextualized)
     * @return Enhanced StructuredTokens with cleanup sequences injected
     */
    StructuredTokens process(const StructuredTokens& input);

private:
    // Current processing state
    std::vector<ScopeDestructorTracker> scope_stack_;
    std::unordered_map<std::string, size_t> variable_to_scope_map_;  // Track which scope declared each variable
    
    /**
     * Process a single function scope and all its nested scopes.
     */
    void analyze_function_scope(StructuredTokens& structured_tokens, size_t function_scope_index);
    
    /**
     * Process the content of a scope, tracking variables and defer statements.
     */
    void process_scope_content(StructuredTokens& structured_tokens, 
                              size_t scope_index,
                              ScopeDestructorTracker& scope_tracker);
    
    /**
     * Identify and extract variable declarations from token sequence.
     */
    std::string extract_variable_declaration(const std::vector<uint32_t>& tokens, size_t& position);
    
    /**
     * Identify and extract defer statements from token sequence.
     */
    std::string extract_defer_statement(const std::vector<uint32_t>& tokens, size_t& position);
    
    /**
     * Check if token at position is a variable declaration.
     */
    bool is_variable_declaration_at(const std::vector<uint32_t>& tokens, size_t position);
    
    /**
     * Check if token at position is a defer statement.
     */
    bool is_defer_statement_at(const std::vector<uint32_t>& tokens, size_t position);
    
    /**
     * Check if token at position is a return statement.
     */
    bool is_return_statement_at(const std::vector<uint32_t>& tokens, size_t position);
    
    /**
     * Generate cleanup token sequence for a list of variables.
     */
    std::vector<uint32_t> generate_cleanup_tokens(const std::deque<std::string>& destruction_order);
    
    /**
     * Generate destructor call tokens for a single variable.
     */
    std::vector<uint32_t> generate_destructor_call_tokens(const std::string& var_name);
    
    /**
     * Inject cleanup sequences before return statements.
     */
    void inject_return_point_cleanup(StructuredTokens& structured_tokens,
                                    size_t scope_index,
                                    const ScopeDestructorTracker& scope_tracker);
    
    /**
     * Inject cleanup sequences at scope exit points.
     */
    void inject_scope_exit_cleanup(StructuredTokens& structured_tokens,
                                  size_t scope_index, 
                                  const ScopeDestructorTracker& scope_tracker);
    
    /**
     * Find all return statement positions in scope content.
     */
    std::vector<size_t> find_return_positions(const std::vector<uint32_t>& content);
    
    /**
     * Insert tokens at specified position in content vector.
     */
    void insert_tokens_at_position(std::vector<uint32_t>& content,
                                  size_t position,
                                  const std::vector<uint32_t>& tokens_to_insert);
    
    /**
     * Check if a scope is conditional (if/else/loop/switch).
     */
    bool is_conditional_scope_type(Scope::Type scope_type);
    
    /**
     * Clear processing state for next function.
     */
    void reset_analysis_state();
    
    /**
     * Validate that input is properly contextualized.
     */
    void validate_input(const StructuredTokens& input);
};

} // namespace cprime