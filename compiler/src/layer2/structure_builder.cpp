#include "structure_builder.h"

namespace cprime {

VoidResult StructureBuilder::build_scope_structure(CompilationContext& context) {
    // Validate Layer 1 completed
    if (context.scopes.empty() || !context.get_root_scope().is_layer_completed(1)) {
        return failure<bool>("Layer 1 must be completed before Layer 2");
    }
    
    BuilderState state(context);
    
    // Flatten token streams into single processing stream
    flatten_token_streams(state);
    
    if (state.current_token_stream.empty()) {
        // Empty input is valid - just mark layer complete
        context.get_root_scope().mark_layer_completed(2);
        context.current_processing_layer = 2;
        return success();
    }
    
    // Initialize with root scope on stack
    state.scope_stack.push(0);
    
    // Process all tokens using cache-and-boundary methodology
    auto result = process_all_tokens(state);
    if (!result.success()) {
        return result;
    }
    
    // Validate final state
    result = validate_final_state(state);
    if (!result.success()) {
        return result;
    }
    
    // Mark all scopes as Layer 2 complete
    for (auto& scope : context.scopes) {
        scope.mark_layer_completed(2);
    }
    context.current_processing_layer = 2;
    
    return success();
}

void StructureBuilder::flatten_token_streams(BuilderState& state) {
    const auto& root_scope = state.context.get_root_scope();
    state.current_token_stream.clear();
    
    // Combine all token streams into single stream
    // This simplifies processing by giving us a linear token sequence
    for (const auto& [stream_id, tokens] : root_scope.token_streams) {
        for (const auto& token : tokens) {
            state.current_token_stream.push_back(token);
        }
    }
}

VoidResult StructureBuilder::process_all_tokens(BuilderState& state) {
    state.current_token_index = 0;
    
    while (state.current_token_index < state.current_token_stream.size()) {
        const Token& token = state.current_token_stream[state.current_token_index];
        
        // Skip EOF tokens
        if (token.kind == TokenKind::EOF_TOKEN) {
            state.current_token_index++;
            continue;
        }
        
        process_token(state, token);
        state.current_token_index++;
    }
    
    // Handle any remaining cached tokens as final instruction
    if (!is_cache_empty(state)) {
        convert_cache_to_instruction(state);
    }
    
    return success();
}

void StructureBuilder::process_token(BuilderState& state, const Token& token) {
    if (is_boundary_token(token)) {
        // Handle boundary tokens according to cache-and-boundary methodology
        if (is_instruction_ending(token)) {
            handle_semicolon(state);
        } else if (is_scope_opening(token)) {
            handle_left_brace(state);
        } else if (is_scope_closing(token)) {
            handle_right_brace(state);
        }
    } else {
        // Regular token - add to cache
        add_token_to_cache(state, token);
    }
}

void StructureBuilder::handle_semicolon(BuilderState& state) {
    // Cache → Instruction tokens, clear cache
    if (!is_cache_empty(state)) {
        convert_cache_to_instruction(state);
        clear_cache(state);
    }
}

void StructureBuilder::handle_left_brace(BuilderState& state) {
    // Cache → Scope signature, determine scope type, enter scope, clear cache
    Scope::Type scope_type = determine_scope_type_from_cache(state.token_cache);
    
    // Convert cache to signature for new scope
    convert_cache_to_scope_signature(state);
    
    // Enter new scope
    size_t new_scope_index = enter_new_scope(state, scope_type);
    
    // Clear cache for new scope
    clear_cache(state);
    
    log_scope_creation(new_scope_index, scope_type, get_current_scope_index(state));
}

void StructureBuilder::handle_right_brace(BuilderState& state) {
    // Validate empty cache (error if not), exit scope
    if (!is_cache_empty(state)) {
        // Convert remaining cache to instruction before closing scope
        convert_cache_to_instruction(state);
        clear_cache(state);
    }
    
    exit_current_scope(state);
}

void StructureBuilder::add_token_to_cache(BuilderState& state, const Token& token) {
    state.token_cache.push_back(token);
}

void StructureBuilder::convert_cache_to_instruction(BuilderState& state) {
    if (is_cache_empty(state)) return;
    
    Scope& current_scope = get_current_scope(state);
    
    // Add cached tokens as new instruction group
    current_scope.instruction_groups.push_back(state.token_cache);
}

void StructureBuilder::convert_cache_to_scope_signature(BuilderState& state) {
    if (is_cache_empty(state)) return;
    
    // The signature will be set on the NEW scope that we're about to create
    // For now, we'll store it temporarily and set it in enter_new_scope
}

void StructureBuilder::clear_cache(BuilderState& state) {
    state.token_cache.clear();
}

bool StructureBuilder::is_cache_empty(const BuilderState& state) {
    return state.token_cache.empty();
}

size_t StructureBuilder::get_current_scope_index(const BuilderState& state) {
    if (state.scope_stack.empty()) {
        return 0; // Root scope
    }
    return state.scope_stack.top();
}

Scope& StructureBuilder::get_current_scope(BuilderState& state) {
    size_t index = get_current_scope_index(state);
    return state.context.scopes[index];
}

size_t StructureBuilder::enter_new_scope(BuilderState& state, Scope::Type type) {
    size_t parent_index = get_current_scope_index(state);
    
    // Create new scope
    size_t new_scope_index = state.context.add_child_scope(parent_index, type);
    
    // Set signature from cache
    if (!is_cache_empty(state)) {
        state.context.scopes[new_scope_index].signature_tokens = state.token_cache;
    }
    
    // Push new scope onto stack
    state.scope_stack.push(new_scope_index);
    
    return new_scope_index;
}

void StructureBuilder::exit_current_scope(BuilderState& state) {
    if (!state.scope_stack.empty()) {
        state.scope_stack.pop();
    }
}

Scope::Type StructureBuilder::determine_scope_type_from_cache(const std::vector<Token>& cache) {
    if (cache.empty()) {
        return Scope::NakedScope; // Bare {} blocks
    }
    
    if (is_function_signature_pattern(cache)) {
        return Scope::NamedFunction;
    }
    
    if (is_class_declaration_pattern(cache)) {
        return Scope::NamedClass;
    }
    
    if (is_conditional_pattern(cache)) {
        return Scope::ConditionalScope;
    }
    
    if (is_loop_pattern(cache)) {
        return Scope::LoopScope;
    }
    
    if (is_try_pattern(cache)) {
        return Scope::TryScope;
    }
    
    return Scope::NakedScope; // Default fallback
}

bool StructureBuilder::is_function_signature_pattern(const std::vector<Token>& tokens) {
    // Look for patterns like: "function name" or "fn name" or "identifier("
    if (tokens.empty()) return false;
    
    // Check for explicit function keyword
    if (tokens_start_with(tokens, TokenKind::FUNCTION)) {
        return true;
    }
    
    // Check for identifier followed by parentheses (function call/definition pattern)
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].kind == TokenKind::IDENTIFIER && 
            i + 1 < tokens.size() && 
            tokens[i + 1].kind == TokenKind::LEFT_PAREN) {
            return true;
        }
    }
    
    return false;
}

bool StructureBuilder::is_class_declaration_pattern(const std::vector<Token>& tokens) {
    return tokens_start_with(tokens, TokenKind::CLASS) ||
           tokens_start_with(tokens, TokenKind::STRUCT) ||
           tokens_start_with(tokens, TokenKind::INTERFACE);
}

bool StructureBuilder::is_conditional_pattern(const std::vector<Token>& tokens) {
    return tokens_start_with(tokens, TokenKind::IF) ||
           tokens_start_with(tokens, TokenKind::ELSE);
}

bool StructureBuilder::is_loop_pattern(const std::vector<Token>& tokens) {
    return tokens_start_with(tokens, TokenKind::WHILE) ||
           tokens_start_with(tokens, TokenKind::FOR);
}

bool StructureBuilder::is_try_pattern(const std::vector<Token>& tokens) {
    return tokens_start_with(tokens, TokenKind::TRY) ||
           tokens_start_with(tokens, TokenKind::CATCH) ||
           tokens_start_with(tokens, TokenKind::FINALLY);
}

bool StructureBuilder::is_boundary_token(const Token& token) {
    return is_scope_opening(token) || is_scope_closing(token) || is_instruction_ending(token);
}

bool StructureBuilder::is_scope_opening(const Token& token) {
    return token.kind == TokenKind::LEFT_BRACE;
}

bool StructureBuilder::is_scope_closing(const Token& token) {
    return token.kind == TokenKind::RIGHT_BRACE;
}

bool StructureBuilder::is_instruction_ending(const Token& token) {
    return token.kind == TokenKind::SEMICOLON;
}

bool StructureBuilder::tokens_start_with(const std::vector<Token>& tokens, TokenKind kind) {
    return !tokens.empty() && tokens[0].kind == kind;
}

bool StructureBuilder::tokens_contain(const std::vector<Token>& tokens, TokenKind kind) {
    return find_token_kind(tokens, kind) != SIZE_MAX;
}

size_t StructureBuilder::find_token_kind(const std::vector<Token>& tokens, TokenKind kind) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].kind == kind) {
            return i;
        }
    }
    return SIZE_MAX;
}

VoidResult StructureBuilder::validate_final_state(const BuilderState& state) {
    // Should end up back at root scope
    if (state.scope_stack.size() != 1 || state.scope_stack.top() != 0) {
        return failure<bool>("Unbalanced scope braces - ended with scope stack size: " + 
                            std::to_string(state.scope_stack.size()));
    }
    
    // Cache should be empty
    if (!is_cache_empty(state)) {
        return failure<bool>("Token cache not empty at end of processing");
    }
    
    return success();
}

void StructureBuilder::log_cache_state(const BuilderState& state, const std::string& context) {
    auto logger = CPRIME_LOGGER("LAYER2");
    CPRIME_LOG_DEBUG(logger, "Cache state at {}: {} tokens", context, state.token_cache.size());
}

void StructureBuilder::log_scope_creation(size_t scope_index, Scope::Type type, size_t parent_index) {
    auto logger = CPRIME_LOGGER("LAYER2");
    CPRIME_LOG_DEBUG(logger, "Created scope [{}] type={} parent={}", 
                     scope_index, static_cast<int>(type), parent_index);
}

} // namespace cprime