#include "layer2.h"
#include "../commons/enum/token.h"
#include "../commons/dirty/string_table.h"
#include <cassert>
#include <iostream>

namespace cprime {

namespace layer2_sublayers {

std::vector<Scope> sublayer2a(const std::map<std::string, std::vector<RawToken>>& streams,
                              const StringTable& string_table,
                              ExecAliasRegistry& exec_registry) {
    using namespace layer2_internal;
    
    ScopeBuilder builder(exec_registry, streams, string_table);
    
    // Initialize global scope - we start parsing inside the global scope body
    builder.scopes.emplace_back();
    builder.scopes[0]._header = Instruction{}; // Empty header for global
    builder.scopes[0]._footer = Instruction{}; // Will be set at end  
    builder.scopes[0]._parentScopeIndex = 0;   // Global is its own parent
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
                // Check if we're collecting exec footer tokens
                if (builder.awaiting_exec_footer) {
                    // End of exec footer - complete the footer and exit exec scope
                    builder.token_cache.add_token(raw_token, token_index);
                    auto footer_instruction = builder.token_cache.create_instruction();
                    builder.exit_scope(footer_instruction);
                    builder.token_cache.clear();
                    builder.awaiting_exec_footer = false;
                } else {
                    // Regular semicolon handling
                    // For exec scopes, don't treat semicolons as instruction boundaries
                    // Accumulate ALL content between { and } for exec blocks
                    bool inside_exec_scope = false;
                    
                    // Check if current scope is an exec scope by examining its header
                    if (builder.current_scope_index > 0 && builder.current_scope_index < builder.scopes.size()) {
                        const auto& current_scope = builder.scopes[builder.current_scope_index];
                        for (const auto& token : current_scope._header._tokens) {
                            if (token._token == EToken::EXEC) {
                                inside_exec_scope = true;
                                break;
                            }
                        }
                    }
                    
                    if (inside_exec_scope) {
                        // Inside exec scope: just accumulate the semicolon, don't create instruction boundary
                        builder.token_cache.add_token(raw_token, token_index);
                    } else {
                        // Regular CPrime scope: treat semicolon as instruction boundary
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
                }
            }
            else if (raw_token._token == EToken::LEFT_BRACE) {
                // Create header instruction from cache, enter new scope
                auto header_instruction = builder.token_cache.create_instruction();
                
                // Check if this is an exec scope by examining all tokens in header
                bool is_exec_scope = false;
                for (const auto& token : header_instruction._tokens) {
                    if (token._token == EToken::EXEC) {
                        is_exec_scope = true;
                        // DEBUG: Exec scope detected successfully
                        break;
                    }
                }
                
                // Enter scope  
                builder.enter_scope(header_instruction);
                
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
                // Check if we're exiting an exec scope
                bool exiting_exec_scope = false;
                if (builder.current_scope_index > 0 && builder.current_scope_index < builder.scopes.size()) {
                    const auto& current_scope = builder.scopes[builder.current_scope_index];
                    for (const auto& token : current_scope._header._tokens) {
                        if (token._token == EToken::EXEC) {
                            exiting_exec_scope = true;
                            break;
                        }
                    }
                }
                
                if (exiting_exec_scope) {
                    if (!builder.token_cache.empty()) {
                        // For exec scopes: accumulated content becomes the main instruction body
                        auto exec_body_instruction = builder.token_cache.create_instruction();
                        builder.add_instruction(exec_body_instruction);
                        builder.token_cache.clear();
                    }
                    // Don't exit scope yet - continue collecting footer tokens until semicolon
                    builder.awaiting_exec_footer = true; // Flag to indicate we're collecting footer
                } else if (!builder.token_cache.empty()) {
                    // Regular scopes: remaining tokens become footer
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

// Helper function to get chunk content from Token via StringTable lookup
std::string get_chunk_content(const Token& token, const std::map<std::string, std::vector<RawToken>>& streams, const StringTable& string_table) {
    // Find the RawToken that this Token references
    for (const auto& [stream_name, raw_tokens] : streams) {
        if (token._stringstreamId == 0 && token._tokenIndex < raw_tokens.size()) { // TODO: Map stream names to IDs
            const RawToken& raw_token = raw_tokens[token._tokenIndex];
            if (raw_token._token == EToken::CHUNK) {
                return string_table.get_string(raw_token.chunk_content_index);
            }
        }
    }
    return ""; // Not found or not a chunk
}

// Namespace detection logic for hierarchical processing
std::string detect_namespace_creation(const Instruction& header, const std::map<std::string, std::vector<RawToken>>& streams, const StringTable& string_table) {
    // PHASE 4: Detect what namespace this scope creates by examining header patterns
    // Legal namespace-creating declarations:
    // 1. "namespace IdentifierName" - creates named namespace
    // 2. "class ClassName" - creates ClassName:: namespace  
    // 3. "struct StructName" - creates StructName:: namespace
    // 4. ":: IdentifierName" - namespace operator prepending (TODO: future)
    
    const auto& tokens = header._tokens;
    
    // Look for CHUNK + SPACE + CHUNK patterns (keyword + identifier)
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i]._token == EToken::CHUNK && 
            i + 2 < tokens.size() && 
            tokens[i + 1]._token == EToken::SPACE &&
            tokens[i + 2]._token == EToken::CHUNK) {
            
            // Get the actual content of both CHUNKs
            std::string first_chunk = get_chunk_content(tokens[i], streams, string_table);
            std::string second_chunk = get_chunk_content(tokens[i + 2], streams, string_table);
            
            // Check if first chunk is a namespace-creating keyword
            if (first_chunk == "namespace" || first_chunk == "class" || first_chunk == "struct") {
                // Return the identifier name from the second chunk
                return second_chunk;
            }
        }
    }
    
    return ""; // No namespace-creating pattern detected
}

void ScopeBuilder::enter_scope(const Instruction& header) {
    // Store current parent scope index before creating new scope
    uint32_t parent_scope_index = current_scope_index;
    
    // Create new scope
    Scope new_scope;
    new_scope._header = header;
    new_scope._footer = Instruction{}; // Will be set on exit
    new_scope._parentScopeIndex = parent_scope_index;
    
    // HIERARCHICAL NAMESPACE PROCESSING: Inherit namespace context from parent
    if (parent_scope_index < scopes.size()) {
        new_scope.namespace_context = scopes[parent_scope_index].namespace_context;
    }
    
    // Check if this scope creates a new namespace and update context
    std::string detected_namespace = detect_namespace_creation(header, streams, string_table);
    if (!detected_namespace.empty()) {
        new_scope.namespace_context.push_back(detected_namespace);
    }
    
    uint32_t new_scope_index = static_cast<uint32_t>(scopes.size());
    scopes.push_back(new_scope);
    
    // Add reference to the nested scope in parent's instructions vector
    add_nested_scope_reference(new_scope_index);
    
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
    // Add instruction to the vector of variants
    scopes[current_scope_index]._instructions.emplace_back(instruction);
}

void ScopeBuilder::add_nested_scope_reference(uint32_t nested_scope_index) {
    // Add nested scope index to parent scope's instructions vector
    // Note: current_scope_index is still pointing to parent when this is called
    scopes[current_scope_index]._instructions.emplace_back(nested_scope_index);
}

} // namespace layer2_internal

} // namespace cprime