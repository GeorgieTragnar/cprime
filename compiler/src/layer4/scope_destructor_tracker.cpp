#include "scope_destructor_tracker.h"
#include <cstdint>

namespace cprime {

ScopeDestructorTracker::ScopeDestructorTracker(const std::deque<std::string>& parent_return_order, 
                                              bool is_conditional_scope)
    : return_destruction_order_(parent_return_order)
    , scope_end_destruction_order_(parent_return_order)  // Both inherit parent's return order
    , is_conditional_(is_conditional_scope)
    , has_return_statement_(false)
    , parent_scope_index_(0)  // Will be set by caller
{
}

ScopeDestructorTracker::ScopeDestructorTracker()
    : return_destruction_order_()
    , scope_end_destruction_order_()
    , is_conditional_(false)
    , has_return_statement_(false)
    , parent_scope_index_(SIZE_MAX)  // Root scope has no parent
{
}

void ScopeDestructorTracker::add_variable(const std::string& var_name) {
    // Add to both destruction order trackers simultaneously
    return_destruction_order_.add_variable(var_name);
    scope_end_destruction_order_.add_variable(var_name);
}

void ScopeDestructorTracker::defer_variable(const std::string& var_name) {
    // Apply defer (bump-to-front) to both destruction order trackers
    return_destruction_order_.defer_variable(var_name);
    scope_end_destruction_order_.defer_variable(var_name);
}

void ScopeDestructorTracker::mark_return_statement() {
    has_return_statement_ = true;
}

const std::deque<std::string>& ScopeDestructorTracker::get_return_destruction_order() const {
    return return_destruction_order_.get_destruction_sequence();
}

const std::deque<std::string>& ScopeDestructorTracker::get_scope_end_destruction_order() const {
    return scope_end_destruction_order_.get_destruction_sequence();
}

bool ScopeDestructorTracker::is_conditional() const {
    return is_conditional_;
}

bool ScopeDestructorTracker::has_return() const {
    return has_return_statement_;
}

bool ScopeDestructorTracker::contains_variable(const std::string& var_name) const {
    // Check either tracker (they should be consistent for variable presence)
    return return_destruction_order_.contains_variable(var_name);
}

size_t ScopeDestructorTracker::get_parent_scope_index() const {
    return parent_scope_index_;
}

void ScopeDestructorTracker::set_parent_scope_index(size_t parent_index) {
    parent_scope_index_ = parent_index;
}

} // namespace cprime