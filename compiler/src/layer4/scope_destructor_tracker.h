#pragma once

#include "destruction_order_tracker.h"
#include <deque>
#include <string>
#include <cstdint>

namespace cprime {

/**
 * ScopeDestructorTracker - Dual deque system for tracking destruction order in scopes.
 * 
 * Implements the dual vector inheritance strategy:
 * - return_destruction_order: Used at return points, inherits from parent scope
 * - scope_end_destruction_order: Used at scope boundaries, includes local cleanup
 * 
 * Both deques are updated simultaneously during defer operations to maintain consistency.
 * Child scopes inherit parent's return destruction order on creation.
 */
class ScopeDestructorTracker {
public:
    /**
     * Constructor for child scope - inherits parent's return destruction order.
     */
    ScopeDestructorTracker(const std::deque<std::string>& parent_return_order, 
                          bool is_conditional_scope = false);
    
    /**
     * Constructor for root scope.
     */
    ScopeDestructorTracker();
    
    /**
     * Add a variable declared in this scope.
     * Updates both destruction order trackers.
     */
    void add_variable(const std::string& var_name);
    
    /**
     * Defer a variable's destruction.
     * Applies bump-to-front to both destruction order trackers.
     */
    void defer_variable(const std::string& var_name);
    
    /**
     * Mark that this scope contains a return statement.
     * Used for conditional defer validation.
     */
    void mark_return_statement();
    
    /**
     * Get destruction order for return points.
     * Used when generating cleanup for return statements.
     */
    const std::deque<std::string>& get_return_destruction_order() const;
    
    /**
     * Get destruction order for scope end.
     * Used when generating cleanup for natural scope exit.
     */
    const std::deque<std::string>& get_scope_end_destruction_order() const;
    
    /**
     * Check if this scope is conditional (if/else/switch/loop).
     */
    bool is_conditional() const;
    
    /**
     * Check if this scope has a return statement.
     */
    bool has_return() const;
    
    /**
     * Check if a variable is tracked in this scope.
     */
    bool contains_variable(const std::string& var_name) const;
    
    /**
     * Get the parent scope index (for validation purposes).
     */
    size_t get_parent_scope_index() const;
    
    /**
     * Set the parent scope index.
     */
    void set_parent_scope_index(size_t parent_index);

private:
    // Dual deque system for different exit point types
    DestructionOrderTracker return_destruction_order_;    // For return statements
    DestructionOrderTracker scope_end_destruction_order_; // For scope boundaries
    
    // Scope metadata
    bool is_conditional_;           // Is this a conditional scope?
    bool has_return_statement_;     // Does this scope have a return?
    size_t parent_scope_index_;     // Index of parent scope (for validation)
};

} // namespace cprime