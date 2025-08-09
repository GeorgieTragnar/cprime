#pragma once

#include "scope_destructor_tracker.h"
#include <string>
#include <vector>
#include <stdexcept>

namespace cprime {

/**
 * DeferValidationError - Exception thrown when defer usage is invalid.
 */
class DeferValidationError : public std::runtime_error {
public:
    explicit DeferValidationError(const std::string& message) 
        : std::runtime_error(message) {}
};

/**
 * DeferValidator - Validates defer statement usage according to CPrime rules.
 * 
 * CPrime defer rules:
 * 1. Simple cases (no conditionals): Always allowed
 * 2. Conditional defer referencing parent scope variables: Requires return statement in scope
 * 3. Conditional defer with only local variables: Always allowed
 * 4. Complex nested conditionals: TODO - currently generates error
 */
class DeferValidator {
public:
    /**
     * Validate a defer statement in the current scope context.
     * 
     * @param deferred_var The variable being deferred
     * @param current_scope The scope where defer is declared
     * @param parent_scopes Stack of parent scopes for validation
     * @throws DeferValidationError if defer usage is invalid
     */
    static void validate_defer_statement(
        const std::string& deferred_var,
        const ScopeDestructorTracker& current_scope,
        const std::vector<ScopeDestructorTracker>& parent_scopes
    );
    
    /**
     * Check if a variable is from a parent scope.
     * 
     * @param var_name Variable to check
     * @param current_scope Current scope tracker
     * @param parent_scopes Stack of parent scopes
     * @return true if variable is declared in a parent scope
     */
    static bool is_parent_scope_variable(
        const std::string& var_name,
        const ScopeDestructorTracker& current_scope,
        const std::vector<ScopeDestructorTracker>& parent_scopes
    );
    
    /**
     * Validate conditional defer pattern.
     * Ensures conditional scopes with parent variable defers have return statements.
     */
    static void validate_conditional_defer_pattern(
        const std::string& deferred_var,
        const ScopeDestructorTracker& current_scope,
        const std::vector<ScopeDestructorTracker>& parent_scopes
    );

private:
    /**
     * Generate error message for invalid conditional defer.
     */
    static std::string generate_conditional_defer_error_message(
        const std::string& deferred_var,
        bool is_parent_var,
        bool has_return
    );
};

} // namespace cprime