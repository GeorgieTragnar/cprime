#pragma once

#include "../layer1/raw_token.h"
#include "../layer1/context_stack.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace cprime {

/**
 * Context attributes for storing metadata about token resolution.
 * Used to pass context-specific information along with tokens.
 */
struct ContextAttributes {
    std::unordered_map<std::string, std::string> data;
    
    void set(const std::string& key, const std::string& value) {
        data[key] = value;
    }
    
    std::string get(const std::string& key, const std::string& default_value = "") const {
        auto it = data.find(key);
        return it != data.end() ? it->second : default_value;
    }
    
    bool has(const std::string& key) const {
        return data.find(key) != data.end();
    }
    
    bool empty() const { return data.empty(); }
};

/**
 * Context-enriched token - Layer 2 output.
 * Contains original raw token plus full context information.
 * This enables GPU-friendly, self-contained token processing.
 */
struct ContextualToken {
    // Original raw token (unchanged)
    RawToken raw_token;
    
    // Context information
    ParseContextType current_context;
    std::vector<ParseContextType> context_stack;  // Stack snapshot at token time
    std::string context_resolution;               // E.g., "RuntimeAccessRight", "DeferRaii"
    ContextAttributes attributes;                 // Context-specific metadata
    
    ContextualToken(const RawToken& raw_token, ParseContextType context)
        : raw_token(raw_token), current_context(context) {}
    
    // Convenience accessors (delegate to raw_token)
    RawTokenType type() const { return raw_token.type; }
    const std::string& value() const { return raw_token.value; }
    size_t line() const { return raw_token.line; }
    size_t column() const { return raw_token.column; }
    size_t position() const { return raw_token.position; }
    
    // Context queries
    bool is_resolved_as(const std::string& resolution) const {
        return context_resolution == resolution;
    }
    
    bool has_attribute(const std::string& key) const {
        return attributes.has(key);
    }
    
    std::string get_attribute(const std::string& key, const std::string& default_value = "") const {
        return attributes.get(key, default_value);
    }
    
    void set_attribute(const std::string& key, const std::string& value) {
        attributes.set(key, value);
    }
    
    // Utility methods (delegate to raw_token)
    bool is_keyword(const std::string& keyword) const {
        return raw_token.is_keyword(keyword);
    }
    
    bool is_identifier() const {
        return raw_token.is_identifier();
    }
    
    bool is_operator(const std::string& op) const {
        return raw_token.is_operator(op);
    }
    
    bool is_punctuation(const std::string& punct) const {
        return raw_token.is_punctuation(punct);
    }
    
    // Debug representation
    std::string to_string() const;
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
    
    // Context-specific queries
    std::vector<ContextualToken> filter_by_resolution(const std::string& resolution) const;
    std::vector<ContextualToken> filter_by_context(ParseContextType context) const;
    size_t count_by_resolution(const std::string& resolution) const;
    
private:
    std::vector<ContextualToken> tokens;
    size_t pos;
    
    // Bounds checking
    void ensure_valid_position() const;
};

} // namespace cprime