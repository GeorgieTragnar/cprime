#pragma once

#include "token_types.h"
#include "tokens.h"
#include <vector>
#include <string>
#include <cstdint>

namespace cprime {

/**
 * Templated structural organization for tokens.
 * Supports both RawToken (Layer 2 output) and ContextualToken (Layer 3 output).
 */
template<typename TokenType>
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
    size_t parent_index;                    // Index into scopes vector (-1 for root/top-level)
    size_t raw_token_stream_id;             // For GPU batching locality
    
    // For named scopes (functions, classes) - signature tokens before the '{'
    std::vector<TokenType> signature_tokens;
    
    // Content preserves exact ordering of instructions and child scopes
    // Instructions are stored directly as token vectors
    // Child scopes are referenced by index using special scope marker tokens
    std::vector<TokenType> content;
    
    // Constructors
    Scope(Type type, size_t parent_idx, size_t stream_id = 0)
        : type(type), parent_index(parent_idx), raw_token_stream_id(stream_id) {}
    
    Scope(Type type, size_t parent_idx, std::vector<TokenType> signature, size_t stream_id = 0)
        : type(type), parent_index(parent_idx), raw_token_stream_id(stream_id), 
          signature_tokens(std::move(signature)) {}
    
    // Convenience methods
    bool is_named() const { return type == NamedFunction || type == NamedClass; }
    bool is_conditional() const { return type == ConditionalScope; }
    bool is_loop() const { return type == LoopScope; }
    bool is_top_level() const { return type == TopLevel; }
};

/**
 * StructuredTokens - Layer 2 output containing hierarchically organized tokens.
 * Uses flat vector with index-based hierarchy for efficiency and GPU compatibility.
 */
template<typename TokenType>
struct StructuredTokens {
    // All scopes in growth-only vector for stable indices
    std::vector<Scope<TokenType>> scopes;
    
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
        scopes.emplace_back(Scope<TokenType>::TopLevel, INVALID_PARENT_INDEX);
        total_scopes = 1;
    }
    
    // Convenience accessors
    bool has_errors() const { return !errors.empty(); }
    
    Scope<TokenType>& root_scope() { return scopes[ROOT_SCOPE_INDEX]; }
    const Scope<TokenType>& root_scope() const { return scopes[ROOT_SCOPE_INDEX]; }
    
    // Add new scope and return its index
    size_t add_scope(typename Scope<TokenType>::Type type, size_t parent_idx, size_t stream_id = 0) {
        size_t new_index = scopes.size();
        scopes.emplace_back(type, parent_idx, stream_id);
        total_scopes++;
        return new_index;
    }
    
    size_t add_scope(typename Scope<TokenType>::Type type, size_t parent_idx, 
                     std::vector<TokenType> signature, size_t stream_id = 0) {
        size_t new_index = scopes.size();
        scopes.emplace_back(type, parent_idx, std::move(signature), stream_id);
        total_scopes++;
        return new_index;
    }
    
    // Error reporting
    void add_error(const std::string& message, size_t token_pos, size_t scope_idx) {
        errors.emplace_back(message, token_pos, scope_idx);
    }
    
    // Navigation helpers
    std::vector<size_t> get_child_scope_indices(size_t parent_idx) const;
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

// Type aliases for common instantiations
using RawStructuredTokens = StructuredTokens<RawToken>;
using ContextualStructuredTokens = StructuredTokens<ContextualToken>;

} // namespace cprime