#pragma once

#include <deque>
#include <string>
#include <algorithm>

namespace cprime {

/**
 * DestructionOrderTracker - Efficient tracking of variable destruction order with defer support.
 * 
 * Uses std::deque for O(1) push_front operations needed for defer "bump-to-front" semantics.
 * Variables are destroyed in front-to-back order (front destructs first).
 * 
 * Key operations:
 * - add_variable(): Normal variable declaration (added to back)
 * - defer_variable(): Removes variable from current position and bumps to front
 * - get_destruction_sequence(): Returns front-to-back destruction order
 */
class DestructionOrderTracker {
public:
    /**
     * Add a variable to the destruction order (normal RAII construction order).
     * Variable is added to the back of the destruction queue.
     */
    void add_variable(const std::string& var_name);
    
    /**
     * Defer a variable's destruction (bump-to-front operation).
     * Removes the variable from its current position and moves it to the front
     * of the destruction queue, making it destruct first.
     */
    void defer_variable(const std::string& var_name);
    
    /**
     * Get the current destruction sequence.
     * Returns variables in front-to-back order (front destructs first).
     */
    const std::deque<std::string>& get_destruction_sequence() const;
    
    /**
     * Check if a variable is tracked in this destruction order.
     */
    bool contains_variable(const std::string& var_name) const;
    
    /**
     * Get the number of variables tracked.
     */
    size_t size() const;
    
    /**
     * Clear all variables from the destruction order.
     */
    void clear();
    
    /**
     * Copy constructor - needed for scope inheritance.
     */
    DestructionOrderTracker(const DestructionOrderTracker& other) = default;
    
    /**
     * Default constructor.
     */
    DestructionOrderTracker() = default;
    
    /**
     * Copy from another tracker (for scope inheritance).
     */
    DestructionOrderTracker(const std::deque<std::string>& initial_order);

private:
    std::deque<std::string> destruction_order;
    
    /**
     * Helper method to remove variable from current position and push to front.
     */
    void remove_and_push_front(const std::string& var_name);
};

} // namespace cprime