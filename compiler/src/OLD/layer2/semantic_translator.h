#pragma once

#include "../common/tokens.h"
#include "../common/structural_types.h"
#include "../common/token_utils.h"
#include "../layer1/context_stack.h"
#include "../common/string_table.h"
#include <stack>
#include <memory>

namespace cprime {

/**
 * Structure Builder - Layer 2 of the N-layer architecture.
 * Pure structural organization: converts raw token vector into hierarchical scopes.
 * NO CONTEXTUALIZATION - only bracket matching and scope detection.
 * 
 * Algorithm:
 * 1. Cache tokens until boundary ('{', '}', ';')
 * 2. On ';': Cache → Instruction tokens, clear cache  
 * 3. On '{': Cache → Scope signature, determine scope type, enter scope, clear cache
 * 4. On '}': Validate empty cache (error if not), exit scope
 */
class StructureBuilder {
public:
    explicit StructureBuilder(const std::vector<RawToken>& raw_tokens, StringTable& string_table);
    
    // Main structure building method
    StructuredTokens build_structure();
    
    // Error handling
    struct StructuralError {
        std::string message;
        size_t token_position;
        size_t line;
        size_t column;
        
        StructuralError(const std::string& msg, size_t pos, size_t line, size_t col)
            : message(msg), token_position(pos), line(line), column(col) {}
    };
    
    const std::vector<StructuralError>& get_errors() const { return errors; }
    bool has_errors() const { return !errors.empty(); }
    
private:
    const std::vector<RawToken>& raw_tokens_;
    StringTable& string_table_;
    size_t current_position_;
    std::vector<StructuralError> errors;
    
    // Structure building state
    StructuredTokens result;
    std::stack<size_t> scope_index_stack;       // Current scope chain
    std::vector<RawToken> token_cache;          // Accumulate tokens until boundary
    
    // Core algorithm implementation
    void process_token_cache_and_boundary();
    
    // Boundary handlers - implement cache-and-boundary methodology  
    void handle_semicolon();                    // Cache → Instruction
    void handle_left_brace();                   // Cache → Scope signature, enter scope
    void handle_right_brace();                  // Validate empty cache, exit scope
    
    // Scope type detection from cached signature tokens
    Scope::Type determine_scope_type_from_cache() const;
    bool is_named_scope_pattern() const;
    bool is_conditional_scope_pattern() const;
    bool is_loop_scope_pattern() const;
    bool is_try_scope_pattern() const;
    
    // Scope pattern matching helpers (structural patterns only)
    bool cache_starts_with_keyword(TokenKind keyword) const;
    bool cache_contains_pattern(const std::vector<TokenKind>& pattern) const;
    size_t find_token_in_cache(TokenKind kind, size_t start_offset = 0) const;
    
    // Cache management
    void add_instruction_to_current_scope();
    void clear_cache();
    bool is_cache_empty() const { return token_cache.empty(); }
    
    // Scope management  
    void enter_new_scope(Scope::Type type);
    void enter_new_scope(Scope::Type type, std::vector<RawToken> signature);
    void exit_current_scope();
    size_t get_current_scope_index() const;
    
    // Token stream navigation
    const RawToken& current_raw_token() const;
    const RawToken& peek_raw_token(size_t offset = 1) const;
    void advance_raw_token();
    bool is_at_end() const;
    
    // Error reporting
    void error(const std::string& message);
    void error_at_position(const std::string& message, size_t pos, size_t line, size_t col);
    void error_missing_semicolon();
    
    // Debug helpers
    void debug_print_cache() const;
    void debug_print_scope_stack() const;
    std::string scope_type_to_string(Scope::Type type) const;
};


} // namespace cprime