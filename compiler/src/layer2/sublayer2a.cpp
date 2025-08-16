#include "layer2.h"
#include "../commons/enum/token.h"
#include <cassert>

namespace cprime {

namespace layer2_sublayers {

std::vector<Scope> sublayer2a(const std::map<std::string, std::vector<RawToken>>& streams,
                              const StringTable& string_table,
                              ExecAliasRegistry& exec_registry) {
    using namespace layer2_internal;
    
    (void)string_table; // Suppress unused warning - no semantic analysis in Sublayer 2A
    
    ScopeBuilder builder(exec_registry);
    
    // Initialize global scope - we start parsing inside the global scope body
    builder.scopes.emplace_back();
    builder.scopes[0]._header = Instruction{}; // Empty header for global
    builder.scopes[0]._footer = Instruction{}; // Will be set at end  
    builder.scopes[0]._parentScopeIndex = 0;   // Global is its own parent
    builder.scopes[0]._scopeType = ScopeType::BLOCK; // All scopes are just BLOCK in Sublayer 2A
    builder.current_scope_index = 0;
    
    // Process each stream
    for (const auto& [stream_name, raw_tokens] : streams) {
        uint32_t stringstream_id = 0; // TODO: Map stream_name to ID
        builder.token_cache.current_stringstream_id = stringstream_id;
        
        // Process tokens using cache-and-boundary methodology
        for (uint32_t token_index = 0; token_index < raw_tokens.size(); ++token_index) {
            const auto& raw_token = raw_tokens[token_index];
            
            // Check for structural boundary tokens
            if (raw_token._token == EToken::SEMICOLON) {
                // Create instruction from cached tokens and add semicolon
                if (!builder.token_cache.empty()) {
                    builder.token_cache.add_token(raw_token, token_index);
                    auto instruction = builder.token_cache.create_instruction();
                    builder.add_instruction(instruction);
                    builder.token_cache.clear();
                } else {
                    // Standalone semicolon - create empty instruction
                    builder.token_cache.add_token(raw_token, token_index);
                    auto instruction = builder.token_cache.create_instruction();
                    builder.add_instruction(instruction);
                    builder.token_cache.clear();
                }
            }
            else if (raw_token._token == EToken::LEFT_BRACE) {
                // Create header instruction from cache, enter new scope
                auto header_instruction = builder.token_cache.create_instruction();
                
                // Check if this is an exec scope by examining first token in header
                bool is_exec_scope = false;
                if (!header_instruction._tokens.empty() && 
                    header_instruction._tokens[0]._token == EToken::EXEC) {
                    is_exec_scope = true;
                }
                
                // Enter scope (always BLOCK type in Sublayer 2A)
                builder.enter_scope(ScopeType::BLOCK, header_instruction);
                
                // If this is an exec scope, register it
                if (is_exec_scope) {
                    uint32_t new_scope_index = static_cast<uint32_t>(builder.scopes.size() - 1);
                    builder.exec_registry.register_scope_index(new_scope_index);
                    
                    // Check if any token is EXEC_ALIAS and register alias mapping
                    for (size_t i = 0; i < header_instruction._tokens.size(); ++i) {
                        if (header_instruction._tokens[i]._token == EToken::EXEC_ALIAS) {
                            // Get the ExecAliasIndex from the raw token
                            uint32_t raw_token_index = header_instruction._tokens[i]._tokenIndex;
                            if (raw_token_index < raw_tokens.size()) {
                                const auto& raw_token_ref = raw_tokens[raw_token_index];
                                if (std::holds_alternative<ExecAliasIndex>(raw_token_ref._literal_value)) {
                                    ExecAliasIndex alias_idx = std::get<ExecAliasIndex>(raw_token_ref._literal_value);
                                    builder.exec_registry.register_scope_index_to_exec_alias(alias_idx, new_scope_index);
                                }
                            }
                            break; // Found EXEC_ALIAS, stop looking
                        }
                    }
                }
                
                builder.token_cache.clear();
            }
            else if (raw_token._token == EToken::RIGHT_BRACE) {
                // Process any remaining tokens as footer, exit scope
                if (!builder.token_cache.empty()) {
                    auto footer_instruction = builder.token_cache.create_instruction();
                    builder.exit_scope(footer_instruction);
                } else {
                    builder.exit_scope(Instruction{});
                }
                builder.token_cache.clear();
            }
            else {
                // Regular token - add to cache
                builder.token_cache.add_token(raw_token, token_index);
            }
        }
        
        // Process any remaining cached tokens at end of stream
        if (!builder.token_cache.empty()) {
            auto instruction = builder.token_cache.create_instruction();
            builder.add_instruction(instruction);
            builder.token_cache.clear();
        }
    }
    
    return std::move(builder.scopes);
}

} // namespace layer2_sublayers

namespace layer2_internal {

void TokenCache::add_token(const RawToken& raw_token, uint32_t token_index) {
    Token token;
    token._stringstreamId = current_stringstream_id;
    token._tokenIndex = token_index;
    token._token = raw_token._token;
    cached_tokens.push_back(token);
}

void TokenCache::clear() {
    cached_tokens.clear();
}

bool TokenCache::empty() const {
    return cached_tokens.empty();
}

Instruction TokenCache::create_instruction() {
    Instruction instruction;
    instruction._tokens = cached_tokens;
    // _contextualTokens and _contexts remain empty for now
    return instruction;
}

void ScopeBuilder::enter_scope(ScopeType type, const Instruction& header) {
    // Create new scope
    Scope new_scope;
    new_scope._header = header;
    new_scope._footer = Instruction{}; // Will be set on exit
    new_scope._parentScopeIndex = current_scope_index;
    new_scope._scopeType = type;
    
    uint32_t new_scope_index = static_cast<uint32_t>(scopes.size());
    scopes.push_back(new_scope);
    
    // Update current scope to point to new child scope
    current_scope_index = new_scope_index;
}

void ScopeBuilder::exit_scope(const Instruction& footer) {
    // Set footer for current scope
    scopes[current_scope_index]._footer = footer;
    
    // Return to parent scope
    current_scope_index = scopes[current_scope_index]._parentScopeIndex;
}

void ScopeBuilder::add_instruction(const Instruction& instruction) {
    // For now, just store instructions in the variant
    // TODO: Implement proper instruction management
    scopes[current_scope_index]._instructions = instruction;
}

} // namespace layer2_internal

} // namespace cprime