#include "semantic_translator.h"
#include "../common/token_utils.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <cassert>

namespace cprime {

// SemanticTranslator implementation - pure enum transformations
SemanticTranslator::SemanticTranslator(RawTokenStream raw_tokens, StringTable& string_table)
    : raw_tokens(std::move(raw_tokens)), string_table_(string_table), position(0) {
    context_resolver = std::make_unique<ContextResolver>(context_stack);
}

std::vector<ContextualToken> SemanticTranslator::translate() {
    contextual_tokens.clear();
    errors.clear();
    position = 0;
    
    // Reset context stack to initial state
    context_stack.clear();
    
    while (!is_at_end()) {
        try {
            ContextualToken contextual_token = translate_next_token();
            
            // Update context based on translated token - enum-based only
            enter_context_from_contextual_token(contextual_token);
            
            contextual_tokens.push_back(std::move(contextual_token));
            
        } catch (const std::exception& e) {
            error("Translation error: " + std::string(e.what()));
            advance_raw_token(); // Skip problematic token
        }
    }
    
    return contextual_tokens;
}

ContextualTokenStream SemanticTranslator::translate_to_stream() {
    return ContextualTokenStream(translate());
}

ContextualToken SemanticTranslator::translate_next_token() {
    const RawToken& raw_token = current_raw_token();
    
    // Determine contextual token kind through pure enum transformation
    ContextualTokenKind contextual_kind = ContextualTokenKind::CONTEXTUAL_TODO;
    
    // Handle context-sensitive keywords using enum-only logic
    if (raw_token.kind == TokenKind::RUNTIME) {
        contextual_kind = resolve_runtime_context(raw_token);
    } else if (raw_token.kind == TokenKind::DEFER) {
        contextual_kind = resolve_defer_context(raw_token);
    } else if (raw_token.kind == TokenKind::CLASS) {
        contextual_kind = resolve_class_context(raw_token);
    } else if (raw_token.kind == TokenKind::UNION) {
        contextual_kind = resolve_union_context(raw_token);
    } else if (raw_token.kind == TokenKind::INTERFACE) {
        contextual_kind = resolve_interface_context(raw_token);
    } else {
        // Direct mapping for non-context-sensitive tokens
        contextual_kind = map_token_kind_to_contextual(raw_token.kind);
    }
    
    // Update parsing context based on enum transformation
    update_parse_context(raw_token.kind, contextual_kind);
    
    // Create contextual token with enum-based resolution
    advance_raw_token();
    return ContextualToken(raw_token, contextual_kind);
}

ContextualTokenKind SemanticTranslator::resolve_runtime_context(const RawToken& /* token */) {
    // Pure enum-based context resolution for RUNTIME keyword
    // Check next token for pattern matching without string operations
    
    if (peek_raw_token().kind == TokenKind::IDENTIFIER) {
        // Could be "runtime exposes" for access rights
        // Note: In pure enum mode, we can't check if next identifier is "exposes"
        // We rely on Layer 1 to have correctly identified "exposes" as IDENTIFIER
        // This is a limitation of the enum-only approach for context-sensitive identifiers
        
        // For now, assume runtime + identifier = runtime access right pattern
        if (is_in_type_expression_context()) {
            return ContextualTokenKind::RUNTIME_TYPE_PARAMETER;
        } else {
            return ContextualTokenKind::RUNTIME_ACCESS_RIGHT;
        }
    } else if (is_in_type_expression_context()) {
        return ContextualTokenKind::RUNTIME_TYPE_PARAMETER;
    } else {
        return ContextualTokenKind::RUNTIME_VARIABLE_DECL;
    }
}

ContextualTokenKind SemanticTranslator::resolve_defer_context(const RawToken& /* token */) {
    // Pure enum-based context resolution for DEFER keyword
    // Check context and next tokens for pattern matching
    
    auto interpretation = context_resolver->resolve_defer_keyword();
    
    switch (interpretation) {
        case ContextResolver::KeywordInterpretation::DeferRaii:
            return ContextualTokenKind::DEFER_RAII;
        case ContextResolver::KeywordInterpretation::DeferCoroutine:
            return ContextualTokenKind::DEFER_COROUTINE;
        default:
            // Default to RAII defer - most common case
            return ContextualTokenKind::DEFER_RAII;
    }
}

ContextualTokenKind SemanticTranslator::resolve_exposes_context(const RawToken& /* token */) {
    // Pure enum-based context resolution for EXPOSES keyword
    // Note: EXPOSES is identified as IDENTIFIER in Layer 1, this method handles the mapping
    
    if (is_in_runtime_context()) {
        return ContextualTokenKind::EXPOSES_RUNTIME;
    } else {
        return ContextualTokenKind::EXPOSES_COMPILE_TIME;
    }
}

ContextualTokenKind SemanticTranslator::resolve_class_context(const RawToken& /* token */) {
    // Pure enum-based context resolution for CLASS keyword
    // Determine class type based on current context
    
    const ParseContext* ctx = context_stack.current();
    if (ctx && ctx->has_attribute("class_type")) {
        const std::string& class_type = ctx->get_attribute("class_type");
        if (class_type == "functional") {
            return ContextualTokenKind::FUNCTIONAL_CLASS;
        } else if (class_type == "danger") {
            return ContextualTokenKind::DANGER_CLASS;
        }
    }
    
    // Default to data class
    return ContextualTokenKind::DATA_CLASS;
}

ContextualTokenKind SemanticTranslator::resolve_union_context(const RawToken& /* token */) {
    // Pure enum-based context resolution for UNION keyword
    // Check if next token is RUNTIME for runtime union detection
    
    if (peek_raw_token().kind == TokenKind::RUNTIME || is_in_runtime_context()) {
        return ContextualTokenKind::RUNTIME_UNION_DECLARATION;
    } else {
        return ContextualTokenKind::UNION_DECLARATION;
    }
}

ContextualTokenKind SemanticTranslator::resolve_interface_context(const RawToken& /* token */) {
    // Pure enum-based context resolution for INTERFACE keyword
    return ContextualTokenKind::INTERFACE_DECLARATION;
}

ContextualTokenKind SemanticTranslator::resolve_function_context(const RawToken& /* token */) {
    // Pure enum-based context resolution for function keywords
    // Note: "fn" and "async" are IDENTIFIERs in Layer 1
    
    // Check if this is preceded by async keyword pattern
    // This would need lookahead/lookbehind which complicates enum-only approach
    // For now, default to regular function declaration
    return ContextualTokenKind::FUNCTION_DECLARATION;
}

ContextualTokenKind SemanticTranslator::map_token_kind_to_contextual(TokenKind kind) {
    // Direct 1:1 mapping for non-context-sensitive tokens
    switch (kind) {
        case TokenKind::IDENTIFIER: return ContextualTokenKind::IDENTIFIER;
        case TokenKind::COMMENT: return ContextualTokenKind::COMMENT;
        case TokenKind::WHITESPACE: return ContextualTokenKind::WHITESPACE;
        case TokenKind::EOF_TOKEN: return ContextualTokenKind::EOF_TOKEN;
        
        // Literals - direct mapping
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
        
        // Operators - direct mapping
        case TokenKind::PLUS: return ContextualTokenKind::PLUS;
        case TokenKind::MINUS: return ContextualTokenKind::MINUS;
        case TokenKind::LEFT_PAREN: return ContextualTokenKind::LEFT_PAREN;
        case TokenKind::RIGHT_PAREN: return ContextualTokenKind::RIGHT_PAREN;
        case TokenKind::LEFT_BRACE: return ContextualTokenKind::LEFT_BRACE;
        case TokenKind::RIGHT_BRACE: return ContextualTokenKind::RIGHT_BRACE;
        case TokenKind::LEFT_BRACKET: return ContextualTokenKind::LEFT_BRACKET;
        case TokenKind::RIGHT_BRACKET: return ContextualTokenKind::RIGHT_BRACKET;
        case TokenKind::SEMICOLON: return ContextualTokenKind::SEMICOLON;
        case TokenKind::COMMA: return ContextualTokenKind::COMMA;
        case TokenKind::COLON: return ContextualTokenKind::COLON;
        
        // Keywords - direct mapping for non-context-sensitive ones
        case TokenKind::IF: return ContextualTokenKind::IF;
        case TokenKind::ELSE: return ContextualTokenKind::ELSE;
        case TokenKind::WHILE: return ContextualTokenKind::WHILE;
        case TokenKind::FOR: return ContextualTokenKind::FOR;
        case TokenKind::RETURN: return ContextualTokenKind::RETURN;
        case TokenKind::VOID: return ContextualTokenKind::VOID;
        case TokenKind::BOOL: return ContextualTokenKind::BOOL;
        case TokenKind::INT: return ContextualTokenKind::INT;
        case TokenKind::CONST: return ContextualTokenKind::CONST;
        case TokenKind::MUT: return ContextualTokenKind::MUT;
        case TokenKind::STATIC: return ContextualTokenKind::STATIC;
        case TokenKind::VOLATILE: return ContextualTokenKind::VOLATILE;
        
        // Default to TODO for unhandled tokens
        default: return ContextualTokenKind::CONTEXTUAL_TODO;
    }
}

void SemanticTranslator::update_parse_context(TokenKind /* kind */, ContextualTokenKind contextual_kind) {
    // Update parsing context based on enum transformations only
    
    switch (contextual_kind) {
        case ContextualTokenKind::DATA_CLASS:
        case ContextualTokenKind::FUNCTIONAL_CLASS:
        case ContextualTokenKind::DANGER_CLASS:
            // Push class context - using contextual kind to determine class type
            if (contextual_kind == ContextualTokenKind::FUNCTIONAL_CLASS) {
                context_stack.push(ParseContext(ParseContextType::ClassDefinition, {{"class_type", "functional"}}));
            } else if (contextual_kind == ContextualTokenKind::DANGER_CLASS) {
                context_stack.push(ParseContext(ParseContextType::ClassDefinition, {{"class_type", "danger"}}));
            } else {
                context_stack.push(ParseContext(ParseContextType::ClassDefinition, {{"class_type", "data"}}));
            }
            break;
            
        case ContextualTokenKind::UNION_DECLARATION:
        case ContextualTokenKind::RUNTIME_UNION_DECLARATION:
            context_stack.push(ParseContext(ParseContextType::UnionDefinition, {}));
            break;
            
        case ContextualTokenKind::INTERFACE_DECLARATION:
            context_stack.push(ParseContext(ParseContextType::InterfaceDefinition, {}));
            break;
            
        case ContextualTokenKind::FUNCTION_DECLARATION:
        case ContextualTokenKind::ASYNC_FUNCTION_DECLARATION:
            context_stack.push(ParseContext(ParseContextType::FunctionBody, {}));
            break;
            
        case ContextualTokenKind::LEFT_BRACE:
            // Block start - context should already be set by preceding construct
            break;
            
        case ContextualTokenKind::RIGHT_BRACE:
            // Block end - pop context
            if (context_stack.depth() > 1) {
                context_stack.pop();
            }
            break;
            
        default:
            // No context change needed for other tokens
            break;
    }
}

bool SemanticTranslator::is_in_runtime_context() const {
    return context_stack.current_context_is_runtime();
}

bool SemanticTranslator::is_in_type_expression_context() const {
    const ParseContext* ctx = context_stack.current();
    return ctx && ctx->type == ParseContextType::TypeExpression;
}

bool SemanticTranslator::is_in_class_declaration_context() const {
    const ParseContext* ctx = context_stack.current();
    return ctx && ctx->type == ParseContextType::ClassDefinition;
}

void SemanticTranslator::enter_context_from_contextual_token(const ContextualToken& /* token */) {
    // Context changes are handled in update_parse_context during translation
    // This method is kept for interface compatibility but delegates to enum-based logic
}

bool SemanticTranslator::peek_for_token_sequence(const std::vector<TokenKind>& sequence) {
    for (size_t i = 0; i < sequence.size(); ++i) {
        if (raw_tokens.position() + i >= raw_tokens.size()) {
            return false; // Not enough tokens remaining
        }
        if (raw_tokens.peek(i).kind != sequence[i]) {
            return false;
        }
    }
    return true;
}

bool SemanticTranslator::peek_for_runtime_modifier() {
    return peek_raw_token().kind == TokenKind::RUNTIME;
}

bool SemanticTranslator::is_start_of_access_rights_declaration() const {
    // Pattern: IDENTIFIER("exposes") followed by IDENTIFIER
    // Note: In pure enum mode, "exposes" is IDENTIFIER, so we can't easily distinguish
    // This is a fundamental limitation of the enum-only approach for context-sensitive identifiers
    
    const RawToken& current = current_raw_token();
    if (current.kind != TokenKind::IDENTIFIER) {
        return false;
    }
    
    // Check if next token is also identifier (access right name)
    return peek_raw_token().kind == TokenKind::IDENTIFIER;
}

bool SemanticTranslator::is_start_of_union_declaration() const {
    return current_raw_token().kind == TokenKind::UNION;
}

bool SemanticTranslator::is_start_of_class_declaration() const {
    return current_raw_token().kind == TokenKind::CLASS;
}

bool SemanticTranslator::is_start_of_function_declaration() const {
    // Pattern: IDENTIFIER("fn") or IDENTIFIER("async") followed by IDENTIFIER("fn")
    // Note: In enum mode, both "fn" and "async" are IDENTIFIERs
    
    const RawToken& current = current_raw_token();
    if (current.kind != TokenKind::IDENTIFIER) {
        return false;
    }
    
    // This is a fundamental limitation - we can't easily distinguish "fn" from other identifiers
    // without string operations. For now, assume any IDENTIFIER followed by IDENTIFIER could be function
    return peek_raw_token().kind == TokenKind::IDENTIFIER;
}

void SemanticTranslator::exit_context_on_block_end() {
    // Pop context when exiting a block
    if (context_stack.depth() > 1) { // Keep at least top-level context
        context_stack.pop();
    }
}

const RawToken& SemanticTranslator::current_raw_token() const {
    return raw_tokens.current();
}

const RawToken& SemanticTranslator::peek_raw_token(size_t offset) const {
    return raw_tokens.peek(offset);
}

void SemanticTranslator::advance_raw_token() {
    raw_tokens.advance();
}

bool SemanticTranslator::is_at_end() const {
    return raw_tokens.is_at_end();
}

void SemanticTranslator::error(const std::string& message) {
    const RawToken& token = current_raw_token();
    error_at_token(message, token);
}

void SemanticTranslator::error_at_token(const std::string& message, const RawToken& token) {
    errors.emplace_back(message, token.line, token.column, context_stack.get_context_path_string());
}

} // namespace cprime