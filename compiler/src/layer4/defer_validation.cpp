#include "defer_validation.h"

namespace cprime {

void DeferValidator::validate_defer_statement(
    const std::string& deferred_var,
    const ScopeDestructorTracker& current_scope,
    const std::vector<ScopeDestructorTracker>& parent_scopes) {
    
    // Rule 1: Simple cases (no conditionals) are always allowed
    if (!current_scope.is_conditional()) {
        return; // Simple defer - always valid
    }
    
    // Rule 2: Conditional defer validation
    validate_conditional_defer_pattern(deferred_var, current_scope, parent_scopes);
}

bool DeferValidator::is_parent_scope_variable(
    const std::string& var_name,
    const ScopeDestructorTracker& current_scope,
    const std::vector<ScopeDestructorTracker>& parent_scopes) {
    
    // Check if variable is NOT in current scope (implies parent scope)
    if (current_scope.contains_variable(var_name)) {
        return false; // Variable is local to current scope
    }
    
    // Check if variable exists in any parent scope
    for (const auto& parent_scope : parent_scopes) {
        if (parent_scope.contains_variable(var_name)) {
            return true; // Found in parent scope
        }
    }
    
    // Variable not found anywhere - this would be a separate compilation error
    return false;
}

void DeferValidator::validate_conditional_defer_pattern(
    const std::string& deferred_var,
    const ScopeDestructorTracker& current_scope,
    const std::vector<ScopeDestructorTracker>& parent_scopes) {
    
    bool is_parent_var = is_parent_scope_variable(deferred_var, current_scope, parent_scopes);
    bool has_return = current_scope.has_return();
    
    // Rule: Conditional defer referencing parent scope variable requires return
    if (is_parent_var && !has_return) {
        throw DeferValidationError(
            generate_conditional_defer_error_message(deferred_var, is_parent_var, has_return)
        );
    }
    
    // Local variable defers in conditional scopes are always allowed
    // (they will be cleaned up at scope end regardless)
}

std::string DeferValidator::generate_conditional_defer_error_message(
    const std::string& deferred_var,
    bool is_parent_var,
    bool has_return) {
    
    if (is_parent_var && !has_return) {
        return "Error: defer statement in conditional scope references parent scope variable '" 
               + deferred_var + "' but scope has no return statement. "
               + "Conditional defer of parent scope variables requires a return statement "
               + "to ensure deterministic cleanup ordering.";
    }
    
    return "Error: invalid defer pattern for variable '" + deferred_var + "'";
}

} // namespace cprime