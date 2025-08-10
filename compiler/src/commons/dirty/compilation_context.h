#pragma once

#include "../scope_types.h"
#include "string_table.h"
#include "../rawToken.h"
#include "error_collector.h"
#include <string>
#include <map>
#include <sstream>

namespace cprime {

/**
 * Clean CompilationContext - pure data structure owned by orchestrator.
 * Contains all compilation data with controlled access to string table.
 */
struct CompilationContext {
    // ===== LAYER 0: Input processing results =====
    std::map<std::string, std::stringstream> input_streams;
    
    // ===== STRING TABLE: Owned by orchestrator =====
    // Mutable access only for Layer 1, read-only for others
    StringTable string_table;
    
    // ===== LAYER 1: Raw tokenization results =====
    // Output from Layer 1: const map after Layer 1 completes
    std::map<std::string, std::vector<RawToken>> raw_token_streams;
    bool raw_tokenization_complete = false;
    
    // ===== MAIN DATA STRUCTURE: Flat vector of scopes =====
    // This is THE central data structure that layers 2+ operate on
    ScopeVector scopes;
    
    // ===== ERROR COLLECTION =====
    // Central error collection for all layers  
    ErrorCollector error_collector;
    
    // ===== PROCESSING STATE =====
    int current_processing_layer = 0;    // Which layer is currently being processed
    bool compilation_complete = false;   // True when all layers finished successfully
    
    // ===== SIMPLE DATA OPERATIONS =====
    
    /**
     * Clear all compilation data (for reuse or cleanup).
     */
    void clear() {
        input_streams.clear();
        string_table.clear();
        raw_token_streams.clear();
        scopes.clear();
        error_collector.clear();
        raw_tokenization_complete = false;
        current_processing_layer = 0;
        compilation_complete = false;
    }
    
    /**
     * Initialize root scope after Layer 1 completes.
     */
    void initialize_root_scope() {
        scopes.clear();
        
        // Create root/top-level scope
        Scope root_scope;
        root_scope.type = Scope::TopLevel;
        root_scope.parent_index = SIZE_MAX;  // Root has no parent
        
        scopes.push_back(root_scope);
    }
    
    /**
     * Add a child scope and return its index.
     */
    size_t add_child_scope(size_t parent_index, Scope::Type type) {
        Scope child_scope;
        child_scope.type = type;
        child_scope.parent_index = parent_index;
        
        size_t child_index = scopes.size();
        scopes.push_back(child_scope);
        
        // Update parent's child list
        if (parent_index < scopes.size()) {
            scopes[parent_index].child_indices.push_back(child_index);
        }
        
        return child_index;
    }
    
    /**
     * Get root scope (always at index 0).
     */
    Scope& get_root_scope() {
        return scopes[0];
    }
    
    const Scope& get_root_scope() const {
        return scopes[0];
    }
};

} // namespace cprime