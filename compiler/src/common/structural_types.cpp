#include "structural_types.h"
#include "token_utils.h"
#include "string_table.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <functional>

namespace cprime {

// ========================================================================
// Type-safe token access methods
// ========================================================================

TokenKind StructuredTokens::get_raw_token_kind(size_t scope_idx, size_t token_idx, bool from_signature) const {
    assert(!contextualized && "Cannot access as TokenKind when contextualized flag is true");
    assert(scope_idx < scopes.size() && "Scope index out of bounds");
    
    const auto& token_vector = from_signature ? scopes[scope_idx].signature_tokens : scopes[scope_idx].content;
    assert(token_idx < token_vector.size() && "Token index out of bounds");
    
    return static_cast<TokenKind>(token_vector[token_idx]);
}

ContextualTokenKind StructuredTokens::get_contextual_token_kind(size_t scope_idx, size_t token_idx, bool from_signature) const {
    assert(contextualized && "Cannot access as ContextualTokenKind when contextualized flag is false");
    assert(scope_idx < scopes.size() && "Scope index out of bounds");
    
    const auto& token_vector = from_signature ? scopes[scope_idx].signature_tokens : scopes[scope_idx].content;
    assert(token_idx < token_vector.size() && "Token index out of bounds");
    
    return static_cast<ContextualTokenKind>(token_vector[token_idx]);
}

void StructuredTokens::add_content_token(size_t scope_idx, TokenKind kind) {
    assert(!contextualized && "Cannot add TokenKind when contextualized");
    assert(scope_idx < scopes.size() && "Scope index out of bounds");
    scopes[scope_idx].content.push_back(static_cast<uint32_t>(kind));
}

void StructuredTokens::add_content_token(size_t scope_idx, ContextualTokenKind kind) {
    assert(contextualized && "Cannot add ContextualTokenKind when not contextualized");
    assert(scope_idx < scopes.size() && "Scope index out of bounds");
    scopes[scope_idx].content.push_back(static_cast<uint32_t>(kind));
}

void StructuredTokens::add_signature_token(size_t scope_idx, TokenKind kind) {
    assert(!contextualized && "Cannot add TokenKind when contextualized");
    assert(scope_idx < scopes.size() && "Scope index out of bounds");
    scopes[scope_idx].signature_tokens.push_back(static_cast<uint32_t>(kind));
}

void StructuredTokens::add_signature_token(size_t scope_idx, ContextualTokenKind kind) {
    assert(contextualized && "Cannot add ContextualTokenKind when not contextualized");
    assert(scope_idx < scopes.size() && "Scope index out of bounds");
    scopes[scope_idx].signature_tokens.push_back(static_cast<uint32_t>(kind));
}

// ========================================================================
// StructuredTokens Navigation Helpers
// ========================================================================

std::vector<size_t> StructuredTokens::get_child_scope_indices(size_t parent_idx) const {
    if (parent_idx >= scopes.size()) {
        return {};
    }
    
    std::vector<size_t> child_indices;
    
    if (contextualized) {
        // When contextualized, check content for scope index markers
        return get_child_scope_indices_from_content(parent_idx);
    } else {
        // When not contextualized, scan all scopes for matching parent_index
        for (size_t i = 0; i < scopes.size(); ++i) {
            if (scopes[i].parent_index == parent_idx) {
                child_indices.push_back(i);
            }
        }
    }
    
    return child_indices;
}

size_t StructuredTokens::calculate_nesting_depth(size_t scope_idx) const {
    if (scope_idx >= scopes.size()) {
        return 0;
    }
    
    size_t depth = 0;
    size_t current_idx = scope_idx;
    
    while (current_idx != INVALID_PARENT_INDEX) {
        const auto& scope = scopes[current_idx];
        if (scope.parent_index == INVALID_PARENT_INDEX) {
            break;  // Reached root scope
        }
        depth++;
        current_idx = scope.parent_index;
    }
    
    return depth;
}

std::string StructuredTokens::to_debug_string() const {
    std::ostringstream oss;
    oss << "StructuredTokens { contextualized: " << (contextualized ? "true" : "false") << "\n";
    oss << "  Total Scopes: " << total_scopes << "\n";
    oss << "  Max Nesting Depth: " << max_nesting_depth << "\n";
    oss << "  Errors: " << errors.size() << "\n";
    
    // Print scope hierarchy
    std::function<void(size_t, int)> print_scope_recursive = 
        [&](size_t scope_idx, int indent) {
            if (scope_idx >= scopes.size()) return;
            
            std::string indent_str(indent * 2, ' ');
            const auto& scope = scopes[scope_idx];
            
            oss << indent_str << "Scope[" << scope_idx << "] { type: ";
            switch (scope.type) {
                case Scope::TopLevel: oss << "TopLevel"; break;
                case Scope::NamedFunction: oss << "NamedFunction"; break;
                case Scope::NamedClass: oss << "NamedClass"; break;
                case Scope::ConditionalScope: oss << "ConditionalScope"; break;
                case Scope::LoopScope: oss << "LoopScope"; break;
                case Scope::TryScope: oss << "TryScope"; break;
                case Scope::NakedScope: oss << "NakedScope"; break;
            }
            
            oss << ", parent: " << scope.parent_index;
            oss << ", stream_id: " << scope.raw_token_stream_id;
            oss << ", signature_tokens: " << scope.signature_tokens.size();
            oss << ", content_tokens: " << scope.content.size();
            oss << " }\n";
            
            // Print child scopes
            auto children = get_child_scope_indices(scope_idx);
            for (size_t child_idx : children) {
                print_scope_recursive(child_idx, indent + 1);
            }
        };
    
    print_scope_recursive(ROOT_SCOPE_INDEX, 1);
    
    // Print errors if any
    if (!errors.empty()) {
        oss << "\n  Errors:\n";
        for (const auto& error : errors) {
            oss << "    - " << error.message 
                << " (pos:" << error.token_position 
                << ", scope:" << error.scope_index << ")\n";
        }
    }
    
    oss << "}";
    return oss.str();
}

void StructuredTokens::print_structure() const {
    std::cout << to_debug_string() << std::endl;
}

// ========================================================================
// Specialized child scope detection based on contextualized flag
// ========================================================================

// When contextualized, look for scope index markers in content tokens
std::vector<size_t> StructuredTokens::get_child_scope_indices_from_content(size_t parent_idx) const {
    if (parent_idx >= scopes.size() || !contextualized) {
        return {};
    }
    
    std::vector<size_t> child_indices;
    const auto& parent_scope = scopes[parent_idx];
    
    // Scan content for scope index markers (only when contextualized)
    for (uint32_t token_value : parent_scope.content) {
        ContextualTokenKind kind = static_cast<ContextualTokenKind>(token_value);
        if (scope_encoding::is_scope_index(kind)) {
            size_t child_idx = scope_encoding::extract_scope_index(kind);
            child_indices.push_back(child_idx);
        }
    }
    
    return child_indices;
}

} // namespace cprime