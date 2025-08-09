#pragma once

#include "../common/structural_types.h"
#include "../common/tokens.h"
#include "../common/string_table.h"
#include <vector>

namespace cprime {

/**
 * Layer 3 - Contextualizer: Pure enum transformation layer.
 * Takes structured tokens from Layer 2 and contextualizes them in-place.
 * Transforms TokenKind → ContextualTokenKind using zero-copy methodology.
 * 
 * Key Features:
 * - In-place contextualization with contextualized flag
 * - Zero memory copying - same storage, different enum interpretation
 * - Pure enum transformations with no string operations
 * - GPU-friendly with embarrassingly parallel structure
 * - Stable indices for concurrent processing
 */
class Contextualizer {
public:
    explicit Contextualizer(StringTable& string_table);
    
    // Main contextualization method - transforms in place
    void contextualize(StructuredTokens& structured_tokens);
    
    // Error reporting
    struct ContextualizationError {
        std::string message;
        size_t scope_index;
        size_t token_index;
        bool in_signature;
        
        ContextualizationError(const std::string& msg, size_t scope_idx, size_t token_idx, bool sig)
            : message(msg), scope_index(scope_idx), token_index(token_idx), in_signature(sig) {}
    };
    
    const std::vector<ContextualizationError>& get_errors() const { return errors; }
    bool has_errors() const { return !errors.empty(); }
    
private:
    StringTable& string_table_;
    std::vector<ContextualizationError> errors;
    
    // Core contextualization methods
    void contextualize_all_scopes(StructuredTokens& structured_tokens);
    void contextualize_scope(StructuredTokens& structured_tokens, size_t scope_index);
    void contextualize_token_sequence(StructuredTokens& structured_tokens, size_t scope_index, bool is_signature);
    
    // Context tracking for scope-aware contextualization
    struct ContextualizationContext {
        Scope::Type current_scope_type;
        size_t scope_index;
        size_t token_index_in_sequence;
        bool in_signature;
        bool in_type_expression;
        bool in_access_right_context;
    };
    
    // Token-by-token contextualization
    ContextualTokenKind contextualize_single_token(TokenKind kind, const ContextualizationContext& context);
    
    // Context-sensitive keyword resolution
    ContextualTokenKind resolve_runtime_keyword(TokenKind kind, const ContextualizationContext& context);
    ContextualTokenKind resolve_defer_keyword(TokenKind kind, const ContextualizationContext& context);
    ContextualTokenKind resolve_class_keyword(TokenKind kind, const ContextualizationContext& context);
    ContextualTokenKind resolve_identifier(TokenKind kind, const ContextualizationContext& context);
    
    // Direct mapping for non-context-sensitive tokens
    ContextualTokenKind map_direct(TokenKind kind);
    
    // Pattern recognition helpers
    bool is_function_signature_pattern(const StructuredTokens& structured, size_t scope_index);
    bool is_access_right_declaration_pattern(const StructuredTokens& structured, size_t scope_index);
    bool has_runtime_modifier_in_signature(const StructuredTokens& structured, size_t scope_index);
    
    // Error reporting
    void error(const std::string& message, size_t scope_idx, size_t token_idx, bool in_signature = false);
};

} // namespace cprime