#include "structural_types.h"
#include "token_utils.h"
#include "string_table.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace cprime {

// ========================================================================
// Template instantiation for common types
// ========================================================================

// Explicit template instantiation for RawToken and ContextualToken
template struct StructuredTokens<RawToken>;
template struct StructuredTokens<ContextualToken>;
template struct Scope<RawToken>;
template struct Scope<ContextualToken>;

// ========================================================================
// StructuredTokens Navigation Helpers
// ========================================================================

template<typename TokenType>
std::vector<size_t> StructuredTokens<TokenType>::get_child_scope_indices(size_t parent_idx) const {
    if (parent_idx >= scopes.size()) {
        return {};
    }
    
    std::vector<size_t> child_indices;
    
    // For RawToken scopes, we need to scan for child scopes manually
    // (scope indices not encoded yet)
    for (size_t i = 0; i < scopes.size(); ++i) {
        if (scopes[i].parent_index == parent_idx) {
            child_indices.push_back(i);
        }
    }
    
    return child_indices;
}

template<typename TokenType>
size_t StructuredTokens<TokenType>::calculate_nesting_depth(size_t scope_idx) const {
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

template<typename TokenType>
std::string StructuredTokens<TokenType>::to_debug_string() const {
    std::ostringstream oss;
    oss << "StructuredTokens<" << typeid(TokenType).name() << "> {\n";
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
                case Scope<TokenType>::TopLevel: oss << "TopLevel"; break;
                case Scope<TokenType>::NamedFunction: oss << "NamedFunction"; break;
                case Scope<TokenType>::NamedClass: oss << "NamedClass"; break;
                case Scope<TokenType>::ConditionalScope: oss << "ConditionalScope"; break;
                case Scope<TokenType>::LoopScope: oss << "LoopScope"; break;
                case Scope<TokenType>::TryScope: oss << "TryScope"; break;
                case Scope<TokenType>::NakedScope: oss << "NakedScope"; break;
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

template<typename TokenType>
void StructuredTokens<TokenType>::print_structure() const {
    std::cout << to_debug_string() << std::endl;
}

// ========================================================================
// Specialized implementations for ContextualToken
// ========================================================================

// Specialized method for ContextualToken to handle scope indices in content
template<>
std::vector<size_t> StructuredTokens<ContextualToken>::get_child_scope_indices(size_t parent_idx) const {
    if (parent_idx >= scopes.size()) {
        return {};
    }
    
    std::vector<size_t> child_indices;
    const auto& parent_scope = scopes[parent_idx];
    
    // For ContextualToken, scan content for scope index markers
    for (const auto& token : parent_scope.content) {
        if (scope_encoding::is_scope_index(token.get_contextual_kind())) {
            size_t child_idx = scope_encoding::extract_scope_index(token.get_contextual_kind());
            child_indices.push_back(child_idx);
        }
    }
    
    return child_indices;
}

} // namespace cprime