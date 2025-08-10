#pragma once

#include "../commons/compilation_context.h"
#include "../commons/common_types.h"
#include "../commons/logger.h"
#include <stack>

namespace cprime {

/**
 * Layer 2: Structure Building
 * 
 * Responsibilities:
 * - Take token streams from Layer 1 (in root scope)
 * - Parse structural boundaries ({, }, ;) to identify scopes
 * - Build flat scope vector with parent/child relationships
 * - Group tokens into signature and instruction groups
 * - Cache tokens until boundaries per your design
 * 
 * Algorithm (your cache-and-boundary methodology):
 * 1. Cache tokens until boundary ('{', '}', ';')
 * 2. On ';': Cache → Instruction tokens, clear cache  
 * 3. On '{': Cache → Scope signature, determine scope type, enter scope, clear cache
 * 4. On '}': Validate empty cache, exit scope
 * 
 * Output: Flat vector of scopes where each scope contains:
 * - signature_tokens: tokens that defined the scope
 * - instruction_groups: grouped tokens between semicolons
 * - Proper parent/child index relationships
 */
class StructureBuilder {
public:
    /**
     * Main Layer 2 entry point.
     * Processes token streams from root scope and builds flat scope vector.
     * 
     * @param context Compilation context with Layer 1 completed
     * @return VoidResult indicating success or failure
     */
    static VoidResult build_scope_structure(CompilationContext& context);

private:
    /**
     * Processing state for structure building.
     */
    struct BuilderState {
        CompilationContext& context;
        std::stack<size_t> scope_stack;         // Stack of current scope indices
        std::vector<Token> token_cache;         // Cache tokens until boundary
        size_t current_token_index;             // Current position in token stream
        std::vector<Token> current_token_stream; // Flattened tokens from all streams
        
        explicit BuilderState(CompilationContext& ctx) : context(ctx), current_token_index(0) {}
    };
    
    /**
     * Flatten all token streams into a single stream for processing.
     */
    static void flatten_token_streams(BuilderState& state);
    
    /**
     * Main processing loop - implements cache-and-boundary algorithm.
     */
    static VoidResult process_all_tokens(BuilderState& state);
    
    /**
     * Process a single token according to cache-and-boundary methodology.
     */
    static void process_token(BuilderState& state, const Token& token);
    
    // Boundary handlers
    static void handle_semicolon(BuilderState& state);          // Cache → Instruction
    static void handle_left_brace(BuilderState& state);         // Cache → Scope signature, enter scope
    static void handle_right_brace(BuilderState& state);        // Validate empty cache, exit scope
    
    // Cache management
    static void add_token_to_cache(BuilderState& state, const Token& token);
    static void convert_cache_to_instruction(BuilderState& state);
    static void convert_cache_to_scope_signature(BuilderState& state);
    static void clear_cache(BuilderState& state);
    static bool is_cache_empty(const BuilderState& state);
    
    // Scope management
    static size_t get_current_scope_index(const BuilderState& state);
    static Scope& get_current_scope(BuilderState& state);
    static size_t enter_new_scope(BuilderState& state, Scope::Type type);
    static void exit_current_scope(BuilderState& state);
    
    // Scope type detection from cached signature tokens
    static Scope::Type determine_scope_type_from_cache(const std::vector<Token>& cache);
    static bool is_function_signature_pattern(const std::vector<Token>& tokens);
    static bool is_class_declaration_pattern(const std::vector<Token>& tokens);
    static bool is_conditional_pattern(const std::vector<Token>& tokens);
    static bool is_loop_pattern(const std::vector<Token>& tokens);
    static bool is_try_pattern(const std::vector<Token>& tokens);
    
    // Token stream utilities
    static bool is_boundary_token(const Token& token);
    static bool is_scope_opening(const Token& token);
    static bool is_scope_closing(const Token& token);
    static bool is_instruction_ending(const Token& token);
    
    // Pattern matching helpers
    static bool tokens_start_with(const std::vector<Token>& tokens, TokenKind kind);
    static bool tokens_contain(const std::vector<Token>& tokens, TokenKind kind);
    static size_t find_token_kind(const std::vector<Token>& tokens, TokenKind kind);
    
    // Validation
    static VoidResult validate_final_state(const BuilderState& state);
    
    // Debug utilities
    static void log_cache_state(const BuilderState& state, const std::string& context);
    static void log_scope_creation(size_t scope_index, Scope::Type type, size_t parent_index);
};

} // namespace cprime