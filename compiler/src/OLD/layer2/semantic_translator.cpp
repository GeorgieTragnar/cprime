#include "semantic_translator.h"
#include "../common/token_utils.h"
#include "../common/logger.h"
#include "../common/logger_components.h"
#include "../common/debug_utils.h"
#include <sstream>

namespace cprime {

// ========================================================================
// StructureBuilder Implementation
// ========================================================================

StructureBuilder::StructureBuilder(const std::vector<RawToken>& raw_tokens, StringTable& string_table)
    : raw_tokens_(raw_tokens), string_table_(string_table), current_position_(0) {
    // Initialize with root scope
    scope_index_stack.push(StructuredTokens::ROOT_SCOPE_INDEX);
    
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER2);
    logger->debug("StructureBuilder initialized with {} tokens", raw_tokens_.size());
}

StructuredTokens StructureBuilder::build_structure() {
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER2);
    logger->info("Building structure from {} raw tokens", raw_tokens_.size());
    
    current_position_ = 0;
    errors.clear();
    
    // Process all tokens using cache-and-boundary methodology
    while (!is_at_end()) {
        const RawToken& token = current_raw_token();
        
        logger->trace("Processing token: {} at position {}", 
                     debug_utils::token_kind_to_string(token.kind), current_position_);
        
        // Check for boundary tokens
        if (token.kind == TokenKind::SEMICOLON) {
            handle_semicolon();
        } else if (token.kind == TokenKind::LEFT_BRACE) {
            handle_left_brace();
        } else if (token.kind == TokenKind::RIGHT_BRACE) {
            handle_right_brace();
        } else {
            // Regular token - add to cache
            token_cache.push_back(token);
        }
        
        advance_raw_token();
    }
    
    // Validate final state - cache should be empty, only root scope should remain
    if (!is_cache_empty()) {
        error("Unexpected end of file - missing semicolon after final statement");
    }
    
    if (scope_index_stack.size() != 1 || scope_index_stack.top() != StructuredTokens::ROOT_SCOPE_INDEX) {
        error("Unexpected end of file - unclosed scope braces");
    }
    
    logger->info("Structure building complete. {} scopes created, {} errors", 
                result.total_scopes, errors.size());
    
    return std::move(result);
}

// ========================================================================
// Boundary Handler Implementation - Cache-and-Boundary Methodology
// ========================================================================

void StructureBuilder::handle_semicolon() {
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER2);
    logger->trace("Handling semicolon - converting cache to instruction");
    
    if (is_cache_empty()) {
        error("Empty statement - semicolon without preceding tokens");
        return;
    }
    
    add_instruction_to_current_scope();
    clear_cache();
}

void StructureBuilder::handle_left_brace() {
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER2);
    logger->trace("Handling left brace - converting cache to scope signature");
    
    // Cache becomes scope signature (can be empty for naked scopes)
    Scope::Type scope_type = determine_scope_type_from_cache();
    
    if (is_cache_empty()) {
        // Naked scope
        enter_new_scope(scope_type);
    } else {
        // Named scope with signature
        std::vector<RawToken> signature(token_cache);
        enter_new_scope(scope_type, std::move(signature));
    }
    
    clear_cache();
}

void StructureBuilder::handle_right_brace() {
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER2);
    logger->trace("Handling right brace - exiting current scope");
    
    if (!is_cache_empty()) {
        error_missing_semicolon();
        return;
    }
    
    exit_current_scope();
}

// ========================================================================
// Scope Type Detection - Structural Patterns Only
// ========================================================================

Scope::Type StructureBuilder::determine_scope_type_from_cache() const {
    if (is_cache_empty()) {
        return Scope::NakedScope;
    }
    
    // Check for named scope patterns
    if (is_named_scope_pattern()) {
        // Distinguish between function and class/struct
        if (cache_contains_pattern({TokenKind::LEFT_PAREN})) {
            return Scope::NamedFunction;
        } else {
            return Scope::NamedClass;
        }
    }
    
    // Check for control flow patterns
    if (is_conditional_scope_pattern()) {
        return Scope::ConditionalScope;
    }
    
    if (is_loop_scope_pattern()) {
        return Scope::LoopScope;
    }
    
    if (is_try_scope_pattern()) {
        return Scope::TryScope;
    }
    
    // Default to naked scope for unrecognized patterns
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER2);
    logger->debug("Unrecognized scope pattern, defaulting to NakedScope");
    return Scope::NakedScope;
}

bool StructureBuilder::is_named_scope_pattern() const {
    // Look for class/struct/function patterns
    return cache_starts_with_keyword(TokenKind::CLASS) ||
           cache_starts_with_keyword(TokenKind::STRUCT) ||
           cache_starts_with_keyword(TokenKind::UNION) ||
           cache_starts_with_keyword(TokenKind::INTERFACE) ||
           cache_contains_pattern({TokenKind::IDENTIFIER, TokenKind::LEFT_PAREN});  // function pattern
}

bool StructureBuilder::is_conditional_scope_pattern() const {
    return cache_starts_with_keyword(TokenKind::IF) ||
           cache_starts_with_keyword(TokenKind::ELSE) ||
           cache_starts_with_keyword(TokenKind::SWITCH) ||
           cache_starts_with_keyword(TokenKind::CASE);
}

bool StructureBuilder::is_loop_scope_pattern() const {
    return cache_starts_with_keyword(TokenKind::FOR) ||
           cache_starts_with_keyword(TokenKind::WHILE);
}

bool StructureBuilder::is_try_scope_pattern() const {
    return cache_starts_with_keyword(TokenKind::TRY) ||
           cache_starts_with_keyword(TokenKind::CATCH);
}

// ========================================================================
// Pattern Matching Helpers
// ========================================================================

bool StructureBuilder::cache_starts_with_keyword(TokenKind keyword) const {
    return !token_cache.empty() && token_cache[0].kind == keyword;
}

bool StructureBuilder::cache_contains_pattern(const std::vector<TokenKind>& pattern) const {
    if (pattern.empty() || token_cache.size() < pattern.size()) {
        return false;
    }
    
    // Simple substring search
    for (size_t i = 0; i <= token_cache.size() - pattern.size(); ++i) {
        bool match = true;
        for (size_t j = 0; j < pattern.size(); ++j) {
            if (token_cache[i + j].kind != pattern[j]) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    
    return false;
}

size_t StructureBuilder::find_token_in_cache(TokenKind kind, size_t start_offset) const {
    for (size_t i = start_offset; i < token_cache.size(); ++i) {
        if (token_cache[i].kind == kind) {
            return i;
        }
    }
    return token_cache.size();  // Not found
}

// ========================================================================
// Cache Management
// ========================================================================

void StructureBuilder::add_instruction_to_current_scope() {
    size_t current_scope_idx = get_current_scope_index();
    
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER2);
    logger->trace("Adding instruction with {} tokens to scope {}", 
                 token_cache.size(), current_scope_idx);
    
    // Add each cached token to the current scope's content
    for (const RawToken& token : token_cache) {
        result.add_content_token(current_scope_idx, token.kind);
    }
}

void StructureBuilder::clear_cache() {
    token_cache.clear();
}

// ========================================================================
// Scope Management
// ========================================================================

void StructureBuilder::enter_new_scope(Scope::Type type) {
    size_t parent_idx = get_current_scope_index();
    size_t new_scope_idx = result.add_scope(type, parent_idx, 0);  // Stream ID not needed anymore
    
    scope_index_stack.push(new_scope_idx);
    
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER2);
    logger->debug("Entered new scope: {} (index {}), parent: {}", 
                 scope_type_to_string(type), new_scope_idx, parent_idx);
}

void StructureBuilder::enter_new_scope(Scope::Type type, std::vector<RawToken> signature) {
    size_t parent_idx = get_current_scope_index();
    
    // Convert RawToken signature to uint32_t for storage
    std::vector<uint32_t> signature_kinds;
    signature_kinds.reserve(signature.size());
    for (const RawToken& token : signature) {
        signature_kinds.push_back(static_cast<uint32_t>(token.kind));
    }
    
    size_t new_scope_idx = result.add_scope(type, parent_idx, std::move(signature_kinds), 0);  // Stream ID not needed anymore
    scope_index_stack.push(new_scope_idx);
    
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER2);
    logger->debug("Entered new named scope: {} (index {}), parent: {}, signature tokens: {}", 
                 scope_type_to_string(type), new_scope_idx, parent_idx, signature.size());
}

void StructureBuilder::exit_current_scope() {
    if (scope_index_stack.size() <= 1) {
        error("Unexpected closing brace - no scope to exit");
        return;
    }
    
    size_t exited_scope_idx = scope_index_stack.top();
    scope_index_stack.pop();
    
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER2);
    logger->debug("Exited scope: {} (index {})", 
                 scope_type_to_string(result.scopes[exited_scope_idx].type), exited_scope_idx);
}

size_t StructureBuilder::get_current_scope_index() const {
    return scope_index_stack.top();
}

// ========================================================================
// Token Stream Navigation
// ========================================================================

const RawToken& StructureBuilder::current_raw_token() const {
    return raw_tokens_[current_position_];
}

const RawToken& StructureBuilder::peek_raw_token(size_t offset) const {
    size_t peek_pos = current_position_ + offset;
    if (peek_pos >= raw_tokens_.size()) {
        static RawToken eof_token(TokenKind::EOF_TOKEN, 0, 0, UINT32_MAX);
        return eof_token;
    }
    return raw_tokens_[peek_pos];
}

void StructureBuilder::advance_raw_token() {
    if (current_position_ < raw_tokens_.size()) {
        current_position_++;
    }
}

bool StructureBuilder::is_at_end() const {
    return current_position_ >= raw_tokens_.size();
}

// ========================================================================
// Error Reporting
// ========================================================================

void StructureBuilder::error(const std::string& message) {
    const RawToken& token = current_raw_token();
    error_at_position(message, current_position_, token.line, token.column);
}

void StructureBuilder::error_at_position(const std::string& message, size_t pos, size_t line, size_t col) {
    errors.emplace_back(message, pos, line, col);
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER2);
    logger->error("Structural error at {}:{}: {}", line, col, message);
}

void StructureBuilder::error_missing_semicolon() {
    error("Missing semicolon - found tokens in cache when closing scope");
}

// ========================================================================
// Debug Helpers
// ========================================================================

void StructureBuilder::debug_print_cache() const {
    std::ostringstream oss;
    oss << "Cache[" << token_cache.size() << "]: ";
    for (const RawToken& token : token_cache) {
        oss << debug_utils::token_kind_to_string(token.kind) << " ";
    }
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER2);
    logger->debug("{}", oss.str());
}

void StructureBuilder::debug_print_scope_stack() const {
    std::ostringstream oss;
    oss << "Scope stack depth: " << scope_index_stack.size();
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER2);
    logger->debug("{}", oss.str());
}

std::string StructureBuilder::scope_type_to_string(Scope::Type type) const {
    switch (type) {
        case Scope::TopLevel: return "TopLevel";
        case Scope::NamedFunction: return "NamedFunction";
        case Scope::NamedClass: return "NamedClass";
        case Scope::ConditionalScope: return "ConditionalScope";
        case Scope::LoopScope: return "LoopScope";
        case Scope::TryScope: return "TryScope";
        case Scope::NakedScope: return "NakedScope";
        default: return "Unknown";
    }
}

} // namespace cprime