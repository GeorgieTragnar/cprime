#pragma once

#include "tokens.h"
#include <vector>

namespace cprime {

/**
 * Raw token stream for convenient iteration and lookahead.
 */
class RawTokenStream {
public:
    explicit RawTokenStream(std::vector<RawToken> tokens);
    
    // Navigation
    const RawToken& current() const;
    const RawToken& peek(size_t offset = 1) const;
    const RawToken& previous() const;
    void advance();
    void rewind();
    bool is_at_end() const;
    
    // Position management
    size_t position() const { return pos; }
    void set_position(size_t new_pos);
    size_t size() const { return tokens.size(); }
    
    // Token access
    const std::vector<RawToken>& get_tokens() const { return tokens; }
    
private:
    std::vector<RawToken> tokens;
    size_t pos;
    
    // Bounds checking
    void ensure_valid_position() const;
};

/**
 * Contextual token stream for convenient iteration and processing.
 * Layer 3 interface for consuming context-enriched tokens.
 */
class ContextualTokenStream {
public:
    explicit ContextualTokenStream(std::vector<ContextualToken> tokens);
    
    // Navigation (same interface as RawTokenStream)
    const ContextualToken& current() const;
    const ContextualToken& peek(size_t offset = 1) const;
    const ContextualToken& previous() const;
    void advance();
    void rewind();
    bool is_at_end() const;
    
    // Position management
    size_t position() const { return pos; }
    void set_position(size_t new_pos);
    size_t size() const { return tokens.size(); }
    
    // Token access
    const std::vector<ContextualToken>& get_tokens() const { return tokens; }
    
    // Enum-based queries
    std::vector<ContextualToken> filter_by_contextual_kind(ContextualTokenKind kind) const;
    size_t count_by_contextual_kind(ContextualTokenKind kind) const;
    
private:
    std::vector<ContextualToken> tokens;
    size_t pos;
    
    // Bounds checking
    void ensure_valid_position() const;
};

} // namespace cprime