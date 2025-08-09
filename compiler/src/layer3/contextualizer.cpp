#include "contextualizer.h"
#include "../common/logger.h"
#include "../common/logger_components.h"
#include "../common/token_utils.h"
#include "../common/debug_utils.h"

namespace cprime {

Contextualizer::Contextualizer(StringTable& string_table) 
    : string_table_(string_table) {
}

void Contextualizer::contextualize(StructuredTokens& structured_tokens) {
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER3);
    logger->info("Starting contextualization of {} scopes", structured_tokens.scopes.size());
    
    if (structured_tokens.is_contextualized()) {
        logger->warn("StructuredTokens already contextualized, skipping");
        return;
    }
    
    errors.clear();
    
    // Contextualize all scopes in order
    contextualize_all_scopes(structured_tokens);
    
    // Set contextualized flag - enables ContextualTokenKind interpretation
    structured_tokens.set_contextualized();
    
    logger->info("Contextualization complete. {} errors encountered", errors.size());
}

void Contextualizer::contextualize_all_scopes(StructuredTokens& structured_tokens) {
    // Process all scopes - embarrassingly parallel (future GPU processing)
    for (size_t scope_idx = 0; scope_idx < structured_tokens.scopes.size(); ++scope_idx) {
        contextualize_scope(structured_tokens, scope_idx);
    }
}

void Contextualizer::contextualize_scope(StructuredTokens& structured_tokens, size_t scope_index) {
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER3);
    logger->trace("Contextualizing scope {}", scope_index);
    
    if (scope_index >= structured_tokens.scopes.size()) {
        error("Invalid scope index", scope_index, 0);
        return;
    }
    
    // Contextualize signature tokens (for named scopes)
    if (!structured_tokens.scopes[scope_index].signature_tokens.empty()) {
        contextualize_token_sequence(structured_tokens, scope_index, true);
    }
    
    // Contextualize content tokens
    if (!structured_tokens.scopes[scope_index].content.empty()) {
        contextualize_token_sequence(structured_tokens, scope_index, false);
    }
}

void Contextualizer::contextualize_token_sequence(StructuredTokens& structured_tokens, size_t scope_index, bool is_signature) {
    const auto& scope = structured_tokens.scopes[scope_index];
    auto& token_sequence = is_signature ? 
        structured_tokens.scopes[scope_index].signature_tokens : 
        structured_tokens.scopes[scope_index].content;
    
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER3);
    logger->trace("Contextualizing {} tokens in scope {} ({})", 
                 token_sequence.size(), scope_index, is_signature ? "signature" : "content");
    
    // Process each token with full context awareness
    for (size_t token_idx = 0; token_idx < token_sequence.size(); ++token_idx) {
        uint32_t raw_token_value = token_sequence[token_idx];
        TokenKind original_kind = static_cast<TokenKind>(raw_token_value);
        
        // Build contextualization context
        ContextualizationContext context;
        context.current_scope_type = scope.type;
        context.scope_index = scope_index;
        context.token_index_in_sequence = token_idx;
        context.in_signature = is_signature;
        context.in_type_expression = false;  // TODO: Implement type expression detection
        context.in_access_right_context = false;  // TODO: Implement access right detection
        
        // Contextualize the token
        ContextualTokenKind contextual_kind = contextualize_single_token(original_kind, context);
        
        // Store the contextualized enum value back
        token_sequence[token_idx] = static_cast<uint32_t>(contextual_kind);
        
        logger->trace("Token {} at scope {}[{}]: {} → {}", 
                     token_idx, scope_index, is_signature ? "sig" : "cont",
                     debug_utils::token_kind_to_string(original_kind), 
                     static_cast<int>(contextual_kind));
    }
}

ContextualTokenKind Contextualizer::contextualize_single_token(TokenKind kind, const ContextualizationContext& context) {
    // Handle context-sensitive keywords
    switch (kind) {
        case TokenKind::RUNTIME:
            return resolve_runtime_keyword(kind, context);
            
        case TokenKind::DEFER:
            return resolve_defer_keyword(kind, context);
            
        case TokenKind::CLASS:
            return resolve_class_keyword(kind, context);
            
        case TokenKind::STRUCT:
            return ContextualTokenKind::STRUCT_DECLARATION;
            
        case TokenKind::UNION:
            // Check for runtime union pattern
            return context.in_access_right_context ? 
                ContextualTokenKind::RUNTIME_UNION_DECLARATION :
                ContextualTokenKind::UNION_DECLARATION;
                
        case TokenKind::INTERFACE:
            return ContextualTokenKind::INTERFACE_DECLARATION;
            
        case TokenKind::PLEX:
            return ContextualTokenKind::PLEX_DECLARATION;
            
        case TokenKind::IDENTIFIER:
            return resolve_identifier(kind, context);
            
        default:
            // Direct mapping for non-context-sensitive tokens
            return map_direct(kind);
    }
}

ContextualTokenKind Contextualizer::resolve_runtime_keyword(TokenKind /* kind */, const ContextualizationContext& context) {
    // Context-sensitive resolution for RUNTIME keyword
    
    if (context.in_signature) {
        // In signature context, likely type parameter or access right
        if (context.current_scope_type == Scope::NamedClass || context.current_scope_type == Scope::NamedFunction) {
            return ContextualTokenKind::RUNTIME_ACCESS_RIGHT;
        } else {
            return ContextualTokenKind::RUNTIME_TYPE_PARAMETER;
        }
    } else if (context.in_type_expression) {
        return ContextualTokenKind::RUNTIME_TYPE_PARAMETER;
    } else {
        // General runtime variable declaration
        return ContextualTokenKind::RUNTIME_VARIABLE_DECL;
    }
}

ContextualTokenKind Contextualizer::resolve_defer_keyword(TokenKind /* kind */, const ContextualizationContext& context) {
    // Context-sensitive resolution for DEFER keyword
    
    if (context.current_scope_type == Scope::NamedFunction) {
        // In function scope, likely RAII defer
        return ContextualTokenKind::DEFER_RAII;
    } else {
        // Default to RAII defer - most common case
        return ContextualTokenKind::DEFER_RAII;
    }
}

ContextualTokenKind Contextualizer::resolve_class_keyword(TokenKind /* kind */, const ContextualizationContext& context) {
    // Context-sensitive resolution for CLASS keyword
    // TODO: Implement class type detection based on modifiers
    
    // For now, default to data class
    // In the future, we would analyze preceding tokens for "functional" or "danger" modifiers
    return ContextualTokenKind::DATA_CLASS;
}

ContextualTokenKind Contextualizer::resolve_identifier(TokenKind /* kind */, const ContextualizationContext& context) {
    // Context-sensitive resolution for IDENTIFIER tokens
    
    if (context.in_signature && context.current_scope_type == Scope::NamedFunction) {
        // Could be function name or parameter
        if (context.token_index_in_sequence == 0) {
            return ContextualTokenKind::FUNCTION_DECLARATION;
        }
    }
    
    if (context.in_type_expression) {
        return ContextualTokenKind::TYPE_IDENTIFIER;
    }
    
    // TODO: Implement detection for special identifiers like "exposes", "fn", "async"
    // This requires string comparison which we want to minimize
    
    // Default to regular identifier
    return ContextualTokenKind::IDENTIFIER;
}

ContextualTokenKind Contextualizer::map_direct(TokenKind kind) {
    // Direct 1:1 mapping for non-context-sensitive tokens
    switch (kind) {
        // Basic tokens
        case TokenKind::COMMENT: return ContextualTokenKind::COMMENT;
        case TokenKind::WHITESPACE: return ContextualTokenKind::WHITESPACE;
        case TokenKind::EOF_TOKEN: return ContextualTokenKind::EOF_TOKEN;
        
        // Literals
        case TokenKind::INT_LITERAL: return ContextualTokenKind::INT_LITERAL;
        case TokenKind::UINT_LITERAL: return ContextualTokenKind::UINT_LITERAL;
        case TokenKind::LONG_LITERAL: return ContextualTokenKind::LONG_LITERAL;
        case TokenKind::ULONG_LITERAL: return ContextualTokenKind::ULONG_LITERAL;
        case TokenKind::LONG_LONG_LITERAL: return ContextualTokenKind::LONG_LONG_LITERAL;
        case TokenKind::ULONG_LONG_LITERAL: return ContextualTokenKind::ULONG_LONG_LITERAL;
        case TokenKind::FLOAT_LITERAL: return ContextualTokenKind::FLOAT_LITERAL;
        case TokenKind::DOUBLE_LITERAL: return ContextualTokenKind::DOUBLE_LITERAL;
        case TokenKind::LONG_DOUBLE_LITERAL: return ContextualTokenKind::LONG_DOUBLE_LITERAL;
        case TokenKind::CHAR_LITERAL: return ContextualTokenKind::CHAR_LITERAL;
        case TokenKind::WCHAR_LITERAL: return ContextualTokenKind::WCHAR_LITERAL;
        case TokenKind::CHAR16_LITERAL: return ContextualTokenKind::CHAR16_LITERAL;
        case TokenKind::CHAR32_LITERAL: return ContextualTokenKind::CHAR32_LITERAL;
        case TokenKind::STRING_LITERAL: return ContextualTokenKind::STRING_LITERAL;
        case TokenKind::WSTRING_LITERAL: return ContextualTokenKind::WSTRING_LITERAL;
        case TokenKind::STRING16_LITERAL: return ContextualTokenKind::STRING16_LITERAL;
        case TokenKind::STRING32_LITERAL: return ContextualTokenKind::STRING32_LITERAL;
        case TokenKind::STRING8_LITERAL: return ContextualTokenKind::STRING8_LITERAL;
        case TokenKind::RAW_STRING_LITERAL: return ContextualTokenKind::RAW_STRING_LITERAL;
        case TokenKind::TRUE_LITERAL: return ContextualTokenKind::TRUE_LITERAL;
        case TokenKind::FALSE_LITERAL: return ContextualTokenKind::FALSE_LITERAL;
        case TokenKind::NULLPTR_LITERAL: return ContextualTokenKind::NULLPTR_LITERAL;
        
        // Operators and punctuation
        case TokenKind::PLUS: return ContextualTokenKind::PLUS;
        case TokenKind::MINUS: return ContextualTokenKind::MINUS;
        case TokenKind::MULTIPLY: return ContextualTokenKind::MULTIPLY;
        case TokenKind::DIVIDE: return ContextualTokenKind::DIVIDE;
        case TokenKind::MODULO: return ContextualTokenKind::MODULO;
        case TokenKind::ASSIGN: return ContextualTokenKind::ASSIGN;
        case TokenKind::LEFT_PAREN: return ContextualTokenKind::LEFT_PAREN;
        case TokenKind::RIGHT_PAREN: return ContextualTokenKind::RIGHT_PAREN;
        case TokenKind::LEFT_BRACE: return ContextualTokenKind::LEFT_BRACE;
        case TokenKind::RIGHT_BRACE: return ContextualTokenKind::RIGHT_BRACE;
        case TokenKind::LEFT_BRACKET: return ContextualTokenKind::LEFT_BRACKET;
        case TokenKind::RIGHT_BRACKET: return ContextualTokenKind::RIGHT_BRACKET;
        case TokenKind::SEMICOLON: return ContextualTokenKind::SEMICOLON;
        case TokenKind::COMMA: return ContextualTokenKind::COMMA;
        case TokenKind::COLON: return ContextualTokenKind::COLON;
        case TokenKind::QUESTION: return ContextualTokenKind::QUESTION;
        case TokenKind::DOT: return ContextualTokenKind::DOT;
        case TokenKind::ARROW: return ContextualTokenKind::ARROW;
        
        // Keywords with direct mapping
        case TokenKind::IF: return ContextualTokenKind::IF;
        case TokenKind::ELSE: return ContextualTokenKind::ELSE;
        case TokenKind::WHILE: return ContextualTokenKind::WHILE;
        case TokenKind::FOR: return ContextualTokenKind::FOR;
        case TokenKind::CASE: return ContextualTokenKind::CASE;
        case TokenKind::SWITCH: return ContextualTokenKind::SWITCH;
        case TokenKind::DEFAULT: return ContextualTokenKind::DEFAULT;
        case TokenKind::BREAK: return ContextualTokenKind::BREAK;
        case TokenKind::CONTINUE: return ContextualTokenKind::CONTINUE;
        case TokenKind::RETURN: return ContextualTokenKind::RETURN;
        case TokenKind::THROW: return ContextualTokenKind::THROW;
        case TokenKind::TRY: return ContextualTokenKind::TRY;
        case TokenKind::CATCH: return ContextualTokenKind::CATCH;
        case TokenKind::VOID: return ContextualTokenKind::VOID;
        case TokenKind::BOOL: return ContextualTokenKind::BOOL;
        case TokenKind::CHAR: return ContextualTokenKind::CHAR;
        case TokenKind::INT: return ContextualTokenKind::INT;
        case TokenKind::FLOAT: return ContextualTokenKind::FLOAT;
        case TokenKind::DOUBLE: return ContextualTokenKind::DOUBLE;
        case TokenKind::CONST: return ContextualTokenKind::CONST;
        case TokenKind::MUT: return ContextualTokenKind::MUT;
        case TokenKind::STATIC: return ContextualTokenKind::STATIC;
        case TokenKind::VOLATILE: return ContextualTokenKind::VOLATILE;
        case TokenKind::PUBLIC: return ContextualTokenKind::PUBLIC;
        case TokenKind::PRIVATE: return ContextualTokenKind::PRIVATE;
        case TokenKind::PROTECTED: return ContextualTokenKind::PROTECTED;
        
        // Default case for unhandled tokens
        default:
            auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER3);
            logger->warn("Unhandled TokenKind in direct mapping: {}", 
                        static_cast<int>(kind));
            return ContextualTokenKind::CONTEXTUAL_TODO;
    }
}

void Contextualizer::error(const std::string& message, size_t scope_idx, size_t token_idx, bool in_signature) {
    errors.emplace_back(message, scope_idx, token_idx, in_signature);
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER3);
    logger->error("Contextualization error in scope {} token {}: {}", 
                 scope_idx, token_idx, message);
}

} // namespace cprime