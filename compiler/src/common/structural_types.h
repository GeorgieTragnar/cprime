#pragma once

#include "token_types.h"
#include "tokens.h"
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>

namespace cprime {

/**
 * Non-templated structural organization for tokens.
 * Uses contextualized flag to track whether tokens are interpreted as TokenKind or ContextualTokenKind.
 * Same memory layout enables zero-copy contextualization and GPU-friendly processing.
 */
struct Scope {
    enum Type {
        TopLevel,              // File-level scope (root)
        NamedFunction,         // function foo(args) -> return_type { }
        NamedClass,            // class/struct/union declarations
        ConditionalScope,      // if (condition) { }, else { }, switch cases
        LoopScope,             // for/while/do-while loops  
        TryScope,              // try/catch/finally blocks
        NakedScope             // bare { } blocks
    };
    
    Type type;
    size_t parent_index;                    // Index into scopes vector (SIZE_MAX for root/top-level)
    size_t raw_token_stream_id;             // For GPU batching locality
    
    // Token storage as raw enum values - interpretation depends on StructuredTokens::contextualized flag
    // For named scopes (functions, classes) - signature tokens before the '{'
    std::vector<uint32_t> signature_tokens;
    
    // Content preserves exact ordering of instructions and child scopes
    // Instructions are stored directly as token enum values
    // Child scopes are referenced by index using special scope marker tokens
    std::vector<uint32_t> content;
    
    // Constructors
    Scope(Type type, size_t parent_idx, size_t stream_id = 0)
        : type(type), parent_index(parent_idx), raw_token_stream_id(stream_id) {}
    
    Scope(Type type, size_t parent_idx, std::vector<uint32_t> signature, size_t stream_id = 0)
        : type(type), parent_index(parent_idx), raw_token_stream_id(stream_id), 
          signature_tokens(std::move(signature)) {}
    
    // Convenience methods
    bool is_named() const { return type == NamedFunction || type == NamedClass; }
    bool is_conditional() const { return type == ConditionalScope; }
    bool is_loop() const { return type == LoopScope; }
    bool is_top_level() const { return type == TopLevel; }
};

/**
 * StructuredTokens - Layer 2/3 output containing hierarchically organized tokens.
 * Uses flat vector with index-based hierarchy for efficiency and GPU compatibility.
 * Single non-templated type with contextualized flag for zero-copy enum transformation.
 */
struct StructuredTokens {
    // Interpretation state - determines how to cast stored enum values
    bool contextualized = false;            // false = TokenKind, true = ContextualTokenKind
    
    // All scopes in growth-only vector for stable indices
    std::vector<Scope> scopes;
    
    // Root scope is always scopes[0] with parent_index = SIZE_MAX (invalid index)
    static constexpr size_t ROOT_SCOPE_INDEX = 0;
    static constexpr size_t INVALID_PARENT_INDEX = SIZE_MAX;
    
    // Error tracking during structure building
    struct StructuralError {
        std::string message;
        size_t token_position;              // Position where error occurred
        size_t scope_index;                 // Scope where error was detected
        
        StructuralError(const std::string& msg, size_t pos, size_t scope_idx)
            : message(msg), token_position(pos), scope_index(scope_idx) {}
    };
    std::vector<StructuralError> errors;
    
    // Statistics for debugging and optimization
    size_t total_scopes = 0;
    size_t max_nesting_depth = 0;
    
    // Constructor - initialize with root scope
    StructuredTokens() {
        scopes.emplace_back(Scope::TopLevel, INVALID_PARENT_INDEX);
        total_scopes = 1;
    }
    
    // Convenience accessors
    bool has_errors() const { return !errors.empty(); }
    
    Scope& root_scope() { return scopes[ROOT_SCOPE_INDEX]; }
    const Scope& root_scope() const { return scopes[ROOT_SCOPE_INDEX]; }
    
    // Add new scope and return its index
    size_t add_scope(Scope::Type type, size_t parent_idx, size_t stream_id = 0) {
        size_t new_index = scopes.size();
        scopes.emplace_back(type, parent_idx, stream_id);
        total_scopes++;
        return new_index;
    }
    
    size_t add_scope(Scope::Type type, size_t parent_idx, 
                     std::vector<uint32_t> signature, size_t stream_id = 0) {
        size_t new_index = scopes.size();
        scopes.emplace_back(type, parent_idx, std::move(signature), stream_id);
        total_scopes++;
        return new_index;
    }
    
    // Error reporting
    void add_error(const std::string& message, size_t token_pos, size_t scope_idx) {
        errors.emplace_back(message, token_pos, scope_idx);
    }
    
    // Type-safe token access based on contextualized flag
    TokenKind get_raw_token_kind(size_t scope_idx, size_t token_idx, bool from_signature = false) const;
    ContextualTokenKind get_contextual_token_kind(size_t scope_idx, size_t token_idx, bool from_signature = false) const;
    
    // Add tokens to scope (automatically stores as uint32_t)
    void add_content_token(size_t scope_idx, TokenKind kind);
    void add_content_token(size_t scope_idx, ContextualTokenKind kind);
    void add_signature_token(size_t scope_idx, TokenKind kind);
    void add_signature_token(size_t scope_idx, ContextualTokenKind kind);
    
    // Contextualization - transforms TokenKind → ContextualTokenKind in place
    void set_contextualized() { contextualized = true; }
    bool is_contextualized() const { return contextualized; }
    
    // Navigation helpers
    std::vector<size_t> get_child_scope_indices(size_t parent_idx) const;
    std::vector<size_t> get_child_scope_indices_from_content(size_t parent_idx) const;
    size_t calculate_nesting_depth(size_t scope_idx) const;
    
    // Debug output
    std::string to_debug_string() const;
    void print_structure() const;
};

/**
 * Scope index encoding for ContextualTokenKind.
 * Uses high bit patterns to encode scope references directly in the enum.
 */
namespace scope_encoding {
    // Reserve upper 31 bits for scope indices (bit 31 = scope marker)
    static constexpr uint32_t SCOPE_INDEX_MASK = 0x80000000;
    static constexpr uint32_t MAX_SCOPE_INDEX = 0x7FFFFFFF;  // ~2 billion scopes
    
    // Check if a ContextualTokenKind represents a scope index
    inline bool is_scope_index(ContextualTokenKind kind) {
        return (static_cast<uint32_t>(kind) & SCOPE_INDEX_MASK) != 0;
    }
    
    // Extract scope index from encoded ContextualTokenKind
    inline size_t extract_scope_index(ContextualTokenKind kind) {
        return static_cast<size_t>(static_cast<uint32_t>(kind) & ~SCOPE_INDEX_MASK);
    }
    
    // Create scope index ContextualTokenKind
    inline ContextualTokenKind encode_scope_index(size_t scope_index) {
        if (scope_index > MAX_SCOPE_INDEX) {
            throw std::runtime_error("Scope index too large for encoding");
        }
        return static_cast<ContextualTokenKind>(SCOPE_INDEX_MASK | static_cast<uint32_t>(scope_index));
    }
    
    // Create scope marker token
    inline ContextualToken create_scope_marker(size_t scope_index) {
        RawToken dummy_raw(TokenKind::EOF_TOKEN, 0, 0, 0);  // Placeholder raw token
        return ContextualToken(dummy_raw, encode_scope_index(scope_index));
    }
}

// No more type aliases needed - single unified type
// Layer 2 output: StructuredTokens with contextualized = false (TokenKind interpretation)
// Layer 3 output: StructuredTokens with contextualized = true (ContextualTokenKind interpretation)

} // namespace cprime