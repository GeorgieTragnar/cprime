#include "layer2.h"
#include "../commons/logger.h"
#include "../commons/dirty/string_table.h"
#include "../commons/enum/token.h"
#include <sstream>
#include <stdexcept>

namespace cprime::layer2_sublayers {

// Helper function to check if a string is a language keyword
bool is_keyword(const std::string& content) {
    // Core language keywords that should be tokenized as keywords
    if (content == "int" || content == "float" || content == "double" || 
        content == "bool" || content == "char" || content == "void" ||
        content == "func" || content == "function" ||
        content == "if" || content == "else" || content == "for" || content == "while" ||
        content == "return" || content == "break" || content == "continue" ||
        content == "exec" || content == "defer" ||
        content == "namespace" || content == "class" || content == "struct" ||
        content == "true" || content == "false") {
        return true;
    }
    return false;
}

// Helper function to get the appropriate EToken for a keyword
EToken get_keyword_token_type(const std::string& content) {
    if (content == "int") return EToken::INT32_T;
    if (content == "float") return EToken::FLOAT;
    if (content == "double") return EToken::DOUBLE;
    if (content == "bool") return EToken::BOOL;
    if (content == "char") return EToken::CHAR;
    if (content == "void") return EToken::VOID;
    if (content == "func" || content == "function") return EToken::FUNC;
    if (content == "if") return EToken::IF;
    if (content == "else") return EToken::ELSE;
    if (content == "for") return EToken::FOR;
    if (content == "while") return EToken::WHILE;
    if (content == "return") return EToken::RETURN;
    if (content == "exec") return EToken::EXEC;
    if (content == "defer") return EToken::DEFER;
    if (content == "true") return EToken::TRUE_LITERAL;
    if (content == "false") return EToken::FALSE_LITERAL;
    
    // TODO: Add more keywords as needed
    return EToken::IDENTIFIER; // Fallback
}

// Helper function to resolve CHUNK token with namespace context
EToken resolve_chunk_with_context(const std::string& chunk_content, 
                                  const std::vector<std::string>& namespace_context,
                                  const ExecAliasRegistry& exec_registry) {
    auto logger = cprime::LoggerFactory::get_logger("chunk_resolver");
    
    // 1. Check for keywords first (namespace-independent)
    if (is_keyword(chunk_content)) {
        EToken keyword_token = get_keyword_token_type(chunk_content);
        LOG_DEBUG("Resolved CHUNK '{}' to keyword: {}", chunk_content, static_cast<uint32_t>(keyword_token));
        return keyword_token;
    }
    
    // 2. Check for exec aliases in current namespace context
    // NOTE: Do not resolve to EXEC_ALIAS here - leave as IDENTIFIER
    // Exec alias resolution should happen during contextualization when RawToken data can be properly updated
    std::vector<std::string> found_namespace_path;
    if (exec_registry.lookup_alias_with_context(chunk_content, namespace_context, found_namespace_path)) {
        LOG_DEBUG("Found exec alias '{}' in namespace [{}], but keeping as IDENTIFIER for now", 
                 chunk_content, found_namespace_path.empty() ? "GLOBAL" : found_namespace_path.back());
        // Return IDENTIFIER - exec alias resolution happens in contextualization
    }
    
    // 3. Default to identifier
    LOG_DEBUG("Resolved CHUNK '{}' to IDENTIFIER", chunk_content);
    return EToken::IDENTIFIER;
}

// Helper function to resolve all CHUNK tokens in an instruction
void resolve_chunks_in_instruction(Instruction& instruction, 
                                   const std::vector<std::string>& namespace_context,
                                   const StringTable& string_table,
                                   std::map<std::string, std::vector<RawToken>>& streams,
                                   const ExecAliasRegistry& exec_registry,
                                   uint32_t current_scope_index) {
    auto logger = cprime::LoggerFactory::get_logger("chunk_resolver");
    
    for (auto& token : instruction._tokens) {
        if (token._token == EToken::CHUNK) {
            // Find the corresponding RawToken to get the chunk content
            for (auto& [stream_name, raw_tokens] : streams) {
                if (token._stringstreamId == 0 && token._tokenIndex < raw_tokens.size()) { // TODO: Map stream names to IDs
                    RawToken& raw_token = raw_tokens[token._tokenIndex];
                    if (raw_token._token == EToken::CHUNK) {
                        // Get chunk content from StringTable
                        std::string chunk_content = string_table.get_string(raw_token.chunk_content_index);
                        
                        // Check if this should be resolved to EXEC_ALIAS
                        std::vector<std::string> found_namespace_path;
                        if (exec_registry.lookup_alias_with_context(chunk_content, namespace_context, found_namespace_path)) {
                            // This CHUNK should become an EXEC_ALIAS
                            ExecAliasIndex alias_index = exec_registry.get_alias_index_with_context(chunk_content, namespace_context);
                            
                            // Validate that the alias exists and get its scope information
                            if (alias_index.value != UINT32_MAX) {
                                // Log cross-scope exec alias usage (this is normal and expected)
                                uint32_t registry_scope_index = exec_registry.get_scope_index_for_alias(alias_index);
                                LOG_INFO("✅ CHUNK→EXEC_ALIAS: '{}' with index {} defined in scope {} (called from scope {})", 
                                         chunk_content, alias_index.value, registry_scope_index, current_scope_index);
                                
                                LOG_INFO("✅ Namespace context: [{}]", 
                                         found_namespace_path.empty() ? "GLOBAL" : found_namespace_path.back());
                                
                                // Update both Token and RawToken
                                token._token = EToken::EXEC_ALIAS;
                                raw_token._token = EToken::EXEC_ALIAS;
                                raw_token._literal_value = alias_index;
                                
                                LOG_DEBUG("Updated RawToken at index {} with ExecAliasIndex {}", 
                                         token._tokenIndex, alias_index.value);
                            } else {
                                // Fallback to IDENTIFIER if alias lookup failed
                                LOG_WARN("Failed to get ExecAliasIndex for '{}', falling back to IDENTIFIER", chunk_content);
                                token._token = EToken::IDENTIFIER;
                                raw_token._token = EToken::IDENTIFIER;
                            }
                        } else {
                            // Regular resolution (keyword or identifier)
                            EToken resolved_token = resolve_chunk_with_context(chunk_content, namespace_context, exec_registry);
                            
                            // Update both Token and RawToken
                            token._token = resolved_token;
                            raw_token._token = resolved_token;
                            
                            LOG_INFO("✅ CHUNK→{}: '{}' at token index {} in scope {}", 
                                     (resolved_token == EToken::INT32_T) ? "KEYWORD" : "IDENTIFIER",
                                     chunk_content, token._tokenIndex, current_scope_index);
                        }
                        break;
                    }
                }
            }
        }
    }
}

// Helper function to resolve CHUNK tokens in a scope
void resolve_chunks_in_scope(Scope& scope, 
                             const std::vector<std::string>& namespace_context,
                             const StringTable& string_table,
                             std::map<std::string, std::vector<RawToken>>& streams,
                             const ExecAliasRegistry& exec_registry,
                             uint32_t current_scope_index) {
    auto logger = cprime::LoggerFactory::get_logger("chunk_resolver");
    
    LOG_DEBUG("Resolving CHUNK tokens in scope with namespace context: {}", namespace_context.size());
    
    // Resolve chunks in header
    resolve_chunks_in_instruction(scope._header, namespace_context, string_table, streams, exec_registry, current_scope_index);
    
    // Resolve chunks in all body instructions
    for (auto& instruction_variant : scope._instructions) {
        if (std::holds_alternative<Instruction>(instruction_variant)) {
            Instruction& instruction = std::get<Instruction>(instruction_variant);
            resolve_chunks_in_instruction(instruction, namespace_context, string_table, streams, exec_registry, current_scope_index);
        }
        // Note: uint32_t variants are scope references, no tokens to resolve
    }
    
    // Resolve chunks in footer (if it's an instruction)
    if (std::holds_alternative<Instruction>(scope._footer)) {
        Instruction& footer_instruction = std::get<Instruction>(scope._footer);
        resolve_chunks_in_instruction(footer_instruction, namespace_context, string_table, streams, exec_registry, current_scope_index);
    }
}

// Helper function to detect if a scope creates a new namespace
std::string detect_namespace_creation(const Instruction& header, 
                                     const StringTable& string_table,
                                     const std::map<std::string, std::vector<RawToken>>& streams) {
    // Check for namespace-creating patterns in header
    // This is similar to the logic in sublayer2a but works with resolved tokens
    const auto& tokens = header._tokens;
    
    if (tokens.size() >= 2) {
        // Look for patterns like "namespace NAME" or "class NAME" or "struct NAME"
        for (size_t i = 0; i < tokens.size() - 1; ++i) {
            if ((tokens[i]._token == EToken::IDENTIFIER && 
                 tokens[i + 1]._token == EToken::IDENTIFIER) ||
                (tokens[i]._token == EToken::IDENTIFIER && 
                 tokens[i + 1]._token == EToken::IDENTIFIER)) {
                
                // Get the content of the first token to check if it's a namespace keyword
                // We need to look up in raw tokens since this runs after resolution
                for (const auto& [stream_name, raw_tokens] : streams) {
                    if (tokens[i]._stringstreamId == 0 && tokens[i]._tokenIndex < raw_tokens.size()) {
                        const RawToken& raw_token = raw_tokens[tokens[i]._tokenIndex];
                        if (raw_token._token == EToken::CHUNK) {
                            std::string first_content = string_table.get_string(raw_token.chunk_content_index);
                            
                            if (first_content == "namespace" || first_content == "class" || first_content == "struct") {
                                // Get the second token content (the namespace name)
                                if (tokens[i + 1]._tokenIndex < raw_tokens.size()) {
                                    const RawToken& name_raw_token = raw_tokens[tokens[i + 1]._tokenIndex];
                                    if (name_raw_token._token == EToken::CHUNK) {
                                        std::string namespace_name = string_table.get_string(name_raw_token.chunk_content_index);
                                        return namespace_name;
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
    
    return ""; // No namespace creation detected
}

// Main hierarchical processing function
void process_scope_hierarchically(std::vector<Scope>& all_scopes, 
                                  uint32_t scope_index,
                                  std::vector<std::string>& current_namespace,
                                  const StringTable& string_table,
                                  std::map<std::string, std::vector<RawToken>>& streams,
                                  const ExecAliasRegistry& exec_registry) {
    auto logger = cprime::LoggerFactory::get_logger("chunk_resolver");
    
    if (scope_index >= all_scopes.size()) {
        return; // Invalid scope index
    }
    
    Scope& scope = all_scopes[scope_index];
    
    std::string namespace_str = current_namespace.empty() ? "GLOBAL" : current_namespace.back();
    LOG_INFO("🔍 Processing scope {} with namespace context: [{}] (depth={})", 
             scope_index, namespace_str, current_namespace.size());
    
    // 1. Copy current namespace context to this scope
    scope.namespace_context = current_namespace;
    
    // 2. Check if this scope creates a new namespace (this needs to be done BEFORE resolution)
    std::string new_namespace = detect_namespace_creation(scope._header, string_table, streams);
    bool creates_namespace = !new_namespace.empty();
    
    if (creates_namespace) {
        current_namespace.push_back(new_namespace);
        LOG_DEBUG("Scope {} creates namespace '{}', new context: [{}]", 
                 scope_index, new_namespace, current_namespace.back());
    }
    
    // 3. Resolve CHUNK tokens in this scope with current namespace context
    resolve_chunks_in_scope(scope, current_namespace, string_table, streams, exec_registry, scope_index);
    
    // 4. Process nested scopes with updated context
    for (const auto& instruction_variant : scope._instructions) {
        if (std::holds_alternative<uint32_t>(instruction_variant)) {
            uint32_t nested_scope_index = std::get<uint32_t>(instruction_variant);
            process_scope_hierarchically(all_scopes, nested_scope_index, current_namespace, 
                                       string_table, streams, exec_registry);
        }
    }
    
    // 5. Pop namespace if we added one
    if (creates_namespace) {
        current_namespace.pop_back();
        LOG_DEBUG("Popped namespace '{}' after processing scope {}", new_namespace, scope_index);
    }
}

// Sublayer 2C: Namespace-aware CHUNK token disambiguation
std::vector<Scope> sublayer2c(const std::vector<Scope>& input_scopes,
                              const StringTable& string_table,
                              std::map<std::string, std::vector<RawToken>>& streams,
                              ExecAliasRegistry& exec_registry) {
    
    // Create a mutable copy of the input scopes for processing
    std::vector<Scope> scopes = input_scopes;
    
    auto logger = cprime::LoggerFactory::get_logger("sublayer2c");
    
    LOG_INFO("=== Sublayer 2C: CHUNK Token Disambiguation ===");
    LOG_INFO("Processing {} scopes for namespace-aware token resolution", scopes.size());
    
    // Start hierarchical processing with empty namespace context (global scope)
    std::vector<std::string> global_namespace_context; // Empty = global scope
    
    // Process all top-level scopes (scopes with parentScopeIndex pointing to themselves or 0)
    for (size_t scope_index = 0; scope_index < scopes.size(); ++scope_index) {
        const Scope& scope = scopes[scope_index];
        
        // Process top-level scopes (parent is 0 or self-referencing)
        if (scope._parentScopeIndex == 0 || scope._parentScopeIndex == scope_index) {
            process_scope_hierarchically(scopes, static_cast<uint32_t>(scope_index), 
                                       global_namespace_context, string_table, streams, exec_registry);
        }
    }
    
    // Count remaining CHUNK tokens for validation
    uint32_t remaining_chunks = 0;
    for (const auto& scope : scopes) {
        // Count chunks in header
        for (const auto& token : scope._header._tokens) {
            if (token._token == EToken::CHUNK) remaining_chunks++;
        }
        
        // Count chunks in instructions
        for (const auto& instruction_variant : scope._instructions) {
            if (std::holds_alternative<Instruction>(instruction_variant)) {
                const Instruction& instruction = std::get<Instruction>(instruction_variant);
                for (const auto& token : instruction._tokens) {
                    if (token._token == EToken::CHUNK) remaining_chunks++;
                }
            }
        }
        
        // Count chunks in footer
        if (std::holds_alternative<Instruction>(scope._footer)) {
            const Instruction& footer = std::get<Instruction>(scope._footer);
            for (const auto& token : footer._tokens) {
                if (token._token == EToken::CHUNK) remaining_chunks++;
            }
        }
    }
    
    if (remaining_chunks > 0) {
        LOG_WARN("WARNING: {} CHUNK tokens remain unresolved after disambiguation", remaining_chunks);
    } else {
        LOG_INFO("All CHUNK tokens successfully resolved");
    }
    
    LOG_INFO("=== Sublayer 2C Complete ===");
    return scopes;
}

} // namespace cprime::layer2_sublayers