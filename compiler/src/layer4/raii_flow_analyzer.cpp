#include "raii_flow_analyzer.h"
#include <stdexcept>

namespace cprime {

StructuredTokens RAIIFlowAnalyzer::process(const StructuredTokens& input) {
    validate_input(input);
    
    // Make a copy to modify
    StructuredTokens result = input;
    
    // Process each function scope in the structured tokens
    for (size_t scope_index = 0; scope_index < result.scopes.size(); ++scope_index) {
        if (result.scopes[scope_index].type == Scope::NamedFunction) {
            reset_analysis_state();
            analyze_function_scope(result, scope_index);
        }
    }
    
    return result;
}

void RAIIFlowAnalyzer::analyze_function_scope(StructuredTokens& structured_tokens, size_t function_scope_index) {
    // Initialize root scope tracker for this function
    scope_stack_.clear();
    scope_stack_.emplace_back(); // Root function scope tracker
    variable_to_scope_map_.clear();
    
    // Process the function scope and all its nested scopes
    auto& function_scope = structured_tokens.scopes[function_scope_index];
    process_scope_content(structured_tokens, function_scope_index, scope_stack_.back());
    
    // Inject cleanup sequences for this function
    inject_return_point_cleanup(structured_tokens, function_scope_index, scope_stack_.back());
    inject_scope_exit_cleanup(structured_tokens, function_scope_index, scope_stack_.back());
}

void RAIIFlowAnalyzer::process_scope_content(StructuredTokens& structured_tokens,
                                            size_t scope_index,
                                            ScopeDestructorTracker& scope_tracker) {
    auto& scope = structured_tokens.scopes[scope_index];
    auto& content = scope.content;
    
    for (size_t i = 0; i < content.size(); ++i) {
        auto token_kind = static_cast<ContextualTokenKind>(content[i]);
        
        // Check for variable declarations
        if (is_variable_declaration_at(content, i)) {
            std::string var_name = extract_variable_declaration(content, i);
            scope_tracker.add_variable(var_name);
            variable_to_scope_map_[var_name] = scope_stack_.size() - 1; // Current scope depth
        }
        
        // Check for defer statements
        else if (token_kind == ContextualTokenKind::DEFER_RAII) {
            std::string deferred_var = extract_defer_statement(content, i);
            
            // Validate the defer statement
            try {
                DeferValidator::validate_defer_statement(deferred_var, scope_tracker, scope_stack_);
                scope_tracker.defer_variable(deferred_var);
            } catch (const DeferValidationError& e) {
                // Add error to structured tokens
                structured_tokens.add_error(e.what(), i, scope_index);
            }
        }
        
        // Check for return statements
        else if (token_kind == ContextualTokenKind::RETURN) {
            scope_tracker.mark_return_statement();
        }
        
        // Process nested scopes recursively
        // TODO: Add nested scope processing when scope markers are implemented
    }
}

std::string RAIIFlowAnalyzer::extract_variable_declaration(const std::vector<uint32_t>& tokens, size_t& position) {
    // Simplified variable extraction - assumes pattern: [type] [identifier]
    // In a real implementation, this would need to handle complex type expressions
    
    // Look for the identifier token following the type
    for (size_t i = position + 1; i < tokens.size(); ++i) {
        auto token_kind = static_cast<ContextualTokenKind>(tokens[i]);
        if (token_kind == ContextualTokenKind::IDENTIFIER) {
            // In a real implementation, we would extract the actual string from the string table
            // For now, return a placeholder based on position
            return "var_" + std::to_string(position);
        }
    }
    
    return "unknown_var";
}

std::string RAIIFlowAnalyzer::extract_defer_statement(const std::vector<uint32_t>& tokens, size_t& position) {
    // Simplified defer extraction - assumes pattern: defer [cleanup_call]
    // Look for the identifier being deferred
    
    for (size_t i = position + 1; i < tokens.size(); ++i) {
        auto token_kind = static_cast<ContextualTokenKind>(tokens[i]);
        if (token_kind == ContextualTokenKind::IDENTIFIER) {
            // In a real implementation, we would parse the full cleanup expression
            // For now, return a placeholder
            return "deferred_var_" + std::to_string(position);
        }
    }
    
    return "unknown_deferred_var";
}

bool RAIIFlowAnalyzer::is_variable_declaration_at(const std::vector<uint32_t>& tokens, size_t position) {
    if (position >= tokens.size()) return false;
    
    auto token_kind = static_cast<ContextualTokenKind>(tokens[position]);
    
    // Check for type tokens that indicate variable declaration
    // This is simplified - real implementation would need more sophisticated parsing
    return token_kind == ContextualTokenKind::INT ||
           token_kind == ContextualTokenKind::FLOAT ||
           token_kind == ContextualTokenKind::DOUBLE ||
           token_kind == ContextualTokenKind::BOOL ||
           token_kind == ContextualTokenKind::AUTO ||
           token_kind == ContextualTokenKind::TYPE_IDENTIFIER;
}

bool RAIIFlowAnalyzer::is_defer_statement_at(const std::vector<uint32_t>& tokens, size_t position) {
    if (position >= tokens.size()) return false;
    
    auto token_kind = static_cast<ContextualTokenKind>(tokens[position]);
    return token_kind == ContextualTokenKind::DEFER_RAII;
}

bool RAIIFlowAnalyzer::is_return_statement_at(const std::vector<uint32_t>& tokens, size_t position) {
    if (position >= tokens.size()) return false;
    
    auto token_kind = static_cast<ContextualTokenKind>(tokens[position]);
    return token_kind == ContextualTokenKind::RETURN;
}

std::vector<uint32_t> RAIIFlowAnalyzer::generate_cleanup_tokens(const std::deque<std::string>& destruction_order) {
    std::vector<uint32_t> cleanup_tokens;
    
    // Generate destructor calls for each variable in destruction order
    for (const auto& var_name : destruction_order) {
        auto destructor_tokens = generate_destructor_call_tokens(var_name);
        cleanup_tokens.insert(cleanup_tokens.end(), destructor_tokens.begin(), destructor_tokens.end());
    }
    
    return cleanup_tokens;
}

std::vector<uint32_t> RAIIFlowAnalyzer::generate_destructor_call_tokens(const std::string& var_name) {
    // Generate a simplified destructor call sequence
    // Real implementation would generate proper function call tokens
    std::vector<uint32_t> tokens;
    
    // Simplified: generate identifier + function call pattern
    tokens.push_back(static_cast<uint32_t>(ContextualTokenKind::IDENTIFIER));  // Destructor function
    tokens.push_back(static_cast<uint32_t>(ContextualTokenKind::LEFT_PAREN));  // (
    tokens.push_back(static_cast<uint32_t>(ContextualTokenKind::IDENTIFIER));  // Variable name
    tokens.push_back(static_cast<uint32_t>(ContextualTokenKind::RIGHT_PAREN)); // )
    tokens.push_back(static_cast<uint32_t>(ContextualTokenKind::SEMICOLON));   // ;
    
    return tokens;
}

void RAIIFlowAnalyzer::inject_return_point_cleanup(StructuredTokens& structured_tokens,
                                                   size_t scope_index,
                                                   const ScopeDestructorTracker& scope_tracker) {
    auto& scope = structured_tokens.scopes[scope_index];
    auto return_positions = find_return_positions(scope.content);
    
    // Generate cleanup token sequence
    auto cleanup_tokens = generate_cleanup_tokens(scope_tracker.get_return_destruction_order());
    
    if (cleanup_tokens.empty()) {
        return; // No cleanup needed
    }
    
    // Insert cleanup before each return statement (process in reverse order to maintain positions)
    for (auto it = return_positions.rbegin(); it != return_positions.rend(); ++it) {
        insert_tokens_at_position(scope.content, *it, cleanup_tokens);
    }
}

void RAIIFlowAnalyzer::inject_scope_exit_cleanup(StructuredTokens& structured_tokens,
                                                 size_t scope_index,
                                                 const ScopeDestructorTracker& scope_tracker) {
    auto& scope = structured_tokens.scopes[scope_index];
    
    // Generate cleanup token sequence for scope exit
    auto cleanup_tokens = generate_cleanup_tokens(scope_tracker.get_scope_end_destruction_order());
    
    if (cleanup_tokens.empty()) {
        return; // No cleanup needed
    }
    
    // Insert cleanup at the end of scope content (before implicit scope end)
    insert_tokens_at_position(scope.content, scope.content.size(), cleanup_tokens);
}

std::vector<size_t> RAIIFlowAnalyzer::find_return_positions(const std::vector<uint32_t>& content) {
    std::vector<size_t> positions;
    
    for (size_t i = 0; i < content.size(); ++i) {
        if (is_return_statement_at(content, i)) {
            positions.push_back(i);
        }
    }
    
    return positions;
}

void RAIIFlowAnalyzer::insert_tokens_at_position(std::vector<uint32_t>& content,
                                                 size_t position,
                                                 const std::vector<uint32_t>& tokens_to_insert) {
    content.insert(content.begin() + position, tokens_to_insert.begin(), tokens_to_insert.end());
}

bool RAIIFlowAnalyzer::is_conditional_scope_type(Scope::Type scope_type) {
    return scope_type == Scope::ConditionalScope ||
           scope_type == Scope::LoopScope ||
           scope_type == Scope::TryScope;
}

void RAIIFlowAnalyzer::reset_analysis_state() {
    scope_stack_.clear();
    variable_to_scope_map_.clear();
}

void RAIIFlowAnalyzer::validate_input(const StructuredTokens& input) {
    if (!input.is_contextualized()) {
        throw std::runtime_error("RAIIFlowAnalyzer requires contextualized StructuredTokens from Layer 3");
    }
    
    if (input.has_errors()) {
        throw std::runtime_error("RAIIFlowAnalyzer cannot process StructuredTokens with errors");
    }
}

} // namespace cprime