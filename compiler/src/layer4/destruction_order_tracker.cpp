#include "destruction_order_tracker.h"

namespace cprime {

DestructionOrderTracker::DestructionOrderTracker(const std::deque<std::string>& initial_order)
    : destruction_order(initial_order) {}

void DestructionOrderTracker::add_variable(const std::string& var_name) {
    // Add to back - normal RAII construction order
    destruction_order.push_back(var_name);
}

void DestructionOrderTracker::defer_variable(const std::string& var_name) {
    // Remove from current position and bump to front
    remove_and_push_front(var_name);
}

const std::deque<std::string>& DestructionOrderTracker::get_destruction_sequence() const {
    return destruction_order;
}

bool DestructionOrderTracker::contains_variable(const std::string& var_name) const {
    return std::find(destruction_order.begin(), destruction_order.end(), var_name) != destruction_order.end();
}

size_t DestructionOrderTracker::size() const {
    return destruction_order.size();
}

void DestructionOrderTracker::clear() {
    destruction_order.clear();
}

void DestructionOrderTracker::remove_and_push_front(const std::string& var_name) {
    // Find the variable in the current destruction order
    auto it = std::find(destruction_order.begin(), destruction_order.end(), var_name);
    
    if (it != destruction_order.end()) {
        // Remove from current position - O(n) but acceptable for defer operations
        destruction_order.erase(it);
    }
    
    // Bump to front - O(1) with deque
    destruction_order.push_front(var_name);
}

} // namespace cprime