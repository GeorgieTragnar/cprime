#include "debug_utils.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>

namespace cprime::debug_utils {

// ========================================================================
// Token String Representations (Layer 1 Debug Functions)
// ========================================================================

const char* token_kind_to_string(TokenKind kind) {
    switch (kind) {
        // Basic tokens
        case TokenKind::IDENTIFIER: return "IDENTIFIER";
        case TokenKind::COMMENT: return "COMMENT";
        case TokenKind::WHITESPACE: return "WHITESPACE";
        case TokenKind::EOF_TOKEN: return "EOF_TOKEN";
        
        // Literals
        case TokenKind::TRUE_LITERAL: return "TRUE_LITERAL";
        case TokenKind::FALSE_LITERAL: return "FALSE_LITERAL";
        case TokenKind::INT_LITERAL: return "INT_LITERAL";
        case TokenKind::UINT_LITERAL: return "UINT_LITERAL";
        case TokenKind::LONG_LITERAL: return "LONG_LITERAL";
        case TokenKind::ULONG_LITERAL: return "ULONG_LITERAL";
        case TokenKind::LONG_LONG_LITERAL: return "LONG_LONG_LITERAL";
        case TokenKind::ULONG_LONG_LITERAL: return "ULONG_LONG_LITERAL";
        case TokenKind::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenKind::DOUBLE_LITERAL: return "DOUBLE_LITERAL";
        case TokenKind::LONG_DOUBLE_LITERAL: return "LONG_DOUBLE_LITERAL";
        case TokenKind::CHAR_LITERAL: return "CHAR_LITERAL";
        case TokenKind::WCHAR_LITERAL: return "WCHAR_LITERAL";
        case TokenKind::CHAR16_LITERAL: return "CHAR16_LITERAL";
        case TokenKind::CHAR32_LITERAL: return "CHAR32_LITERAL";
        case TokenKind::STRING_LITERAL: return "STRING_LITERAL";
        case TokenKind::WSTRING_LITERAL: return "WSTRING_LITERAL";
        case TokenKind::STRING16_LITERAL: return "STRING16_LITERAL";
        case TokenKind::STRING32_LITERAL: return "STRING32_LITERAL";
        case TokenKind::STRING8_LITERAL: return "STRING8_LITERAL";
        case TokenKind::RAW_STRING_LITERAL: return "RAW_STRING_LITERAL";
        case TokenKind::NULLPTR_LITERAL: return "NULLPTR_LITERAL";
        
        // Keywords
        case TokenKind::CLASS: return "CLASS";
        case TokenKind::STRUCT: return "STRUCT";
        case TokenKind::UNION: return "UNION";
        case TokenKind::INTERFACE: return "INTERFACE";
        case TokenKind::PLEX: return "PLEX";
        case TokenKind::RUNTIME: return "RUNTIME";
        case TokenKind::DEFER: return "DEFER";
        case TokenKind::IF: return "IF";
        case TokenKind::ELSE: return "ELSE";
        case TokenKind::WHILE: return "WHILE";
        case TokenKind::FOR: return "FOR";
        case TokenKind::CASE: return "CASE";
        case TokenKind::SWITCH: return "SWITCH";
        case TokenKind::DEFAULT: return "DEFAULT";
        case TokenKind::BREAK: return "BREAK";
        case TokenKind::CONTINUE: return "CONTINUE";
        case TokenKind::RETURN: return "RETURN";
        case TokenKind::GOTO: return "GOTO";
        case TokenKind::THROW: return "THROW";
        case TokenKind::TRY: return "TRY";
        case TokenKind::CATCH: return "CATCH";
        case TokenKind::AUTO: return "AUTO";
        case TokenKind::VOID: return "VOID";
        case TokenKind::BOOL: return "BOOL";
        case TokenKind::CHAR: return "CHAR";
        case TokenKind::WCHAR_T: return "WCHAR_T";
        case TokenKind::INT: return "INT";
        case TokenKind::SHORT: return "SHORT";
        case TokenKind::LONG: return "LONG";
        case TokenKind::SIGNED: return "SIGNED";
        case TokenKind::UNSIGNED: return "UNSIGNED";
        case TokenKind::FLOAT: return "FLOAT";
        case TokenKind::DOUBLE: return "DOUBLE";
        case TokenKind::INT8_T: return "INT8_T";
        case TokenKind::INT16_T: return "INT16_T";
        case TokenKind::INT32_T: return "INT32_T";
        case TokenKind::INT64_T: return "INT64_T";
        case TokenKind::UINT8_T: return "UINT8_T";
        case TokenKind::UINT16_T: return "UINT16_T";
        case TokenKind::UINT32_T: return "UINT32_T";
        case TokenKind::UINT64_T: return "UINT64_T";
        case TokenKind::CHAR8_T: return "CHAR8_T";
        case TokenKind::CHAR16_T: return "CHAR16_T";
        case TokenKind::CHAR32_T: return "CHAR32_T";
        case TokenKind::CONST: return "CONST";
        case TokenKind::MUT: return "MUT";
        case TokenKind::STATIC: return "STATIC";
        case TokenKind::EXTERN: return "EXTERN";
        case TokenKind::REGISTER: return "REGISTER";
        case TokenKind::THREAD_LOCAL: return "THREAD_LOCAL";
        case TokenKind::VOLATILE: return "VOLATILE";
        case TokenKind::CONSTEXPR: return "CONSTEXPR";
        case TokenKind::CONSTEVAL: return "CONSTEVAL";
        case TokenKind::CONSTINIT: return "CONSTINIT";
        case TokenKind::NOEXCEPT: return "NOEXCEPT";
        case TokenKind::INLINE: return "INLINE";
        case TokenKind::NEW: return "NEW";
        case TokenKind::DELETE: return "DELETE";
        case TokenKind::DANGER: return "DANGER";
        case TokenKind::PUBLIC: return "PUBLIC";
        case TokenKind::PRIVATE: return "PRIVATE";
        case TokenKind::PROTECTED: return "PROTECTED";
        case TokenKind::FRIEND: return "FRIEND";
        case TokenKind::SIZEOF: return "SIZEOF";
        case TokenKind::ALIGNOF: return "ALIGNOF";
        case TokenKind::ALIGNAS: return "ALIGNAS";
        case TokenKind::DECLTYPE: return "DECLTYPE";
        case TokenKind::TYPEOF: return "TYPEOF";
        case TokenKind::TYPEID: return "TYPEID";
        case TokenKind::TEMPLATE: return "TEMPLATE";
        case TokenKind::TYPENAME: return "TYPENAME";
        case TokenKind::USING: return "USING";
        case TokenKind::NAMESPACE: return "NAMESPACE";
        
        // Operators
        case TokenKind::PLUS: return "PLUS";
        case TokenKind::MINUS: return "MINUS";
        case TokenKind::MULTIPLY: return "MULTIPLY";
        case TokenKind::DIVIDE: return "DIVIDE";
        case TokenKind::MODULO: return "MODULO";
        case TokenKind::ASSIGN: return "ASSIGN";
        case TokenKind::PLUS_ASSIGN: return "PLUS_ASSIGN";
        case TokenKind::MINUS_ASSIGN: return "MINUS_ASSIGN";
        case TokenKind::MULTIPLY_ASSIGN: return "MULTIPLY_ASSIGN";
        case TokenKind::DIVIDE_ASSIGN: return "DIVIDE_ASSIGN";
        case TokenKind::MODULO_ASSIGN: return "MODULO_ASSIGN";
        case TokenKind::INCREMENT: return "INCREMENT";
        case TokenKind::DECREMENT: return "DECREMENT";
        case TokenKind::EQUAL_EQUAL: return "EQUAL_EQUAL";
        case TokenKind::NOT_EQUAL: return "NOT_EQUAL";
        case TokenKind::LESS_THAN: return "LESS_THAN";
        case TokenKind::GREATER_THAN: return "GREATER_THAN";
        case TokenKind::LESS_EQUAL: return "LESS_EQUAL";
        case TokenKind::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenKind::SPACESHIP: return "SPACESHIP";
        case TokenKind::LOGICAL_AND: return "LOGICAL_AND";
        case TokenKind::LOGICAL_OR: return "LOGICAL_OR";
        case TokenKind::LOGICAL_NOT: return "LOGICAL_NOT";
        case TokenKind::BIT_AND: return "BIT_AND";
        case TokenKind::BIT_OR: return "BIT_OR";
        case TokenKind::BIT_XOR: return "BIT_XOR";
        case TokenKind::BIT_NOT: return "BIT_NOT";
        case TokenKind::LEFT_SHIFT: return "LEFT_SHIFT";
        case TokenKind::RIGHT_SHIFT: return "RIGHT_SHIFT";
        case TokenKind::BIT_AND_ASSIGN: return "BIT_AND_ASSIGN";
        case TokenKind::BIT_OR_ASSIGN: return "BIT_OR_ASSIGN";
        case TokenKind::BIT_XOR_ASSIGN: return "BIT_XOR_ASSIGN";
        case TokenKind::LEFT_SHIFT_ASSIGN: return "LEFT_SHIFT_ASSIGN";
        case TokenKind::RIGHT_SHIFT_ASSIGN: return "RIGHT_SHIFT_ASSIGN";
        case TokenKind::DOT: return "DOT";
        case TokenKind::ARROW: return "ARROW";
        case TokenKind::SCOPE_RESOLUTION: return "SCOPE_RESOLUTION";
        case TokenKind::DOT_STAR: return "DOT_STAR";
        case TokenKind::ARROW_STAR: return "ARROW_STAR";
        case TokenKind::LEFT_PAREN: return "LEFT_PAREN";
        case TokenKind::RIGHT_PAREN: return "RIGHT_PAREN";
        case TokenKind::LEFT_BRACE: return "LEFT_BRACE";
        case TokenKind::RIGHT_BRACE: return "RIGHT_BRACE";
        case TokenKind::LEFT_BRACKET: return "LEFT_BRACKET";
        case TokenKind::RIGHT_BRACKET: return "RIGHT_BRACKET";
        case TokenKind::SEMICOLON: return "SEMICOLON";
        case TokenKind::COMMA: return "COMMA";
        case TokenKind::COLON: return "COLON";
        case TokenKind::QUESTION: return "QUESTION";
        case TokenKind::ELLIPSIS: return "ELLIPSIS";
        
        default:
            return "UNKNOWN_TOKEN_KIND";
    }
}

const char* contextual_token_kind_to_string(ContextualTokenKind kind) {
    switch (kind) {
        // Basic tokens (direct mapping)
        case ContextualTokenKind::IDENTIFIER: return "IDENTIFIER";
        case ContextualTokenKind::COMMENT: return "COMMENT";
        case ContextualTokenKind::WHITESPACE: return "WHITESPACE";
        case ContextualTokenKind::EOF_TOKEN: return "EOF_TOKEN";
        
        // Literals (direct mapping)
        case ContextualTokenKind::INT_LITERAL: return "INT_LITERAL";
        case ContextualTokenKind::UINT_LITERAL: return "UINT_LITERAL";
        case ContextualTokenKind::LONG_LITERAL: return "LONG_LITERAL";
        case ContextualTokenKind::ULONG_LITERAL: return "ULONG_LITERAL";
        case ContextualTokenKind::LONG_LONG_LITERAL: return "LONG_LONG_LITERAL";
        case ContextualTokenKind::ULONG_LONG_LITERAL: return "ULONG_LONG_LITERAL";
        case ContextualTokenKind::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case ContextualTokenKind::DOUBLE_LITERAL: return "DOUBLE_LITERAL";
        case ContextualTokenKind::LONG_DOUBLE_LITERAL: return "LONG_DOUBLE_LITERAL";
        case ContextualTokenKind::CHAR_LITERAL: return "CHAR_LITERAL";
        case ContextualTokenKind::WCHAR_LITERAL: return "WCHAR_LITERAL";
        case ContextualTokenKind::CHAR16_LITERAL: return "CHAR16_LITERAL";
        case ContextualTokenKind::CHAR32_LITERAL: return "CHAR32_LITERAL";
        case ContextualTokenKind::STRING_LITERAL: return "STRING_LITERAL";
        case ContextualTokenKind::WSTRING_LITERAL: return "WSTRING_LITERAL";
        case ContextualTokenKind::STRING16_LITERAL: return "STRING16_LITERAL";
        case ContextualTokenKind::STRING32_LITERAL: return "STRING32_LITERAL";
        case ContextualTokenKind::STRING8_LITERAL: return "STRING8_LITERAL";
        case ContextualTokenKind::RAW_STRING_LITERAL: return "RAW_STRING_LITERAL";
        case ContextualTokenKind::TRUE_LITERAL: return "TRUE_LITERAL";
        case ContextualTokenKind::FALSE_LITERAL: return "FALSE_LITERAL";
        case ContextualTokenKind::NULLPTR_LITERAL: return "NULLPTR_LITERAL";
        
        // Context-sensitive keyword resolutions (new in Layer 3)
        case ContextualTokenKind::RUNTIME_ACCESS_RIGHT: return "RUNTIME_ACCESS_RIGHT";
        case ContextualTokenKind::RUNTIME_TYPE_PARAMETER: return "RUNTIME_TYPE_PARAMETER";
        case ContextualTokenKind::RUNTIME_VARIABLE_DECL: return "RUNTIME_VARIABLE_DECL";
        case ContextualTokenKind::RUNTIME_UNION_DECLARATION: return "RUNTIME_UNION_DECLARATION";
        case ContextualTokenKind::RUNTIME_COROUTINE: return "RUNTIME_COROUTINE";
        
        case ContextualTokenKind::DEFER_RAII: return "DEFER_RAII";
        case ContextualTokenKind::DEFER_COROUTINE: return "DEFER_COROUTINE";
        case ContextualTokenKind::DEFER_SCOPE_GUARD: return "DEFER_SCOPE_GUARD";
        
        case ContextualTokenKind::DATA_CLASS: return "DATA_CLASS";
        case ContextualTokenKind::FUNCTIONAL_CLASS: return "FUNCTIONAL_CLASS";
        case ContextualTokenKind::DANGER_CLASS: return "DANGER_CLASS";
        case ContextualTokenKind::STRUCT_DECLARATION: return "STRUCT_DECLARATION";
        case ContextualTokenKind::UNION_DECLARATION: return "UNION_DECLARATION";
        case ContextualTokenKind::INTERFACE_DECLARATION: return "INTERFACE_DECLARATION";
        case ContextualTokenKind::PLEX_DECLARATION: return "PLEX_DECLARATION";
        
        case ContextualTokenKind::EXPOSES_COMPILE_TIME: return "EXPOSES_COMPILE_TIME";
        case ContextualTokenKind::EXPOSES_RUNTIME: return "EXPOSES_RUNTIME";
        case ContextualTokenKind::FUNCTION_DECLARATION: return "FUNCTION_DECLARATION";
        case ContextualTokenKind::ASYNC_FUNCTION_DECLARATION: return "ASYNC_FUNCTION_DECLARATION";
        
        case ContextualTokenKind::ACCESS_RIGHT_DECLARATION: return "ACCESS_RIGHT_DECLARATION";
        case ContextualTokenKind::ACCESS_RIGHT_USAGE: return "ACCESS_RIGHT_USAGE";
        case ContextualTokenKind::TYPE_IDENTIFIER: return "TYPE_IDENTIFIER";
        case ContextualTokenKind::GENERIC_TYPE_PARAMETER: return "GENERIC_TYPE_PARAMETER";
        
        // Advanced contextual tokens
        case ContextualTokenKind::CAPABILITY_GRANT: return "CAPABILITY_GRANT";
        case ContextualTokenKind::COROUTINE_YIELD: return "COROUTINE_YIELD";
        case ContextualTokenKind::ASYNC_AWAIT: return "ASYNC_AWAIT";
        case ContextualTokenKind::MEMORY_REGION: return "MEMORY_REGION";
        case ContextualTokenKind::COMPILE_TIME_EVAL: return "COMPILE_TIME_EVAL";
        
        // Error and placeholder tokens
        case ContextualTokenKind::CONTEXTUAL_TODO: return "CONTEXTUAL_TODO";
        case ContextualTokenKind::CONTEXTUAL_ERROR: return "CONTEXTUAL_ERROR";
        case ContextualTokenKind::CONTEXTUAL_UNKNOWN: return "CONTEXTUAL_UNKNOWN";
        
        // Operators and punctuation (direct mapping - abbreviated list)
        case ContextualTokenKind::PLUS: return "PLUS";
        case ContextualTokenKind::MINUS: return "MINUS";
        case ContextualTokenKind::MULTIPLY: return "MULTIPLY";
        case ContextualTokenKind::DIVIDE: return "DIVIDE";
        case ContextualTokenKind::MODULO: return "MODULO";
        case ContextualTokenKind::ASSIGN: return "ASSIGN";
        case ContextualTokenKind::LEFT_PAREN: return "LEFT_PAREN";
        case ContextualTokenKind::RIGHT_PAREN: return "RIGHT_PAREN";
        case ContextualTokenKind::LEFT_BRACE: return "LEFT_BRACE";
        case ContextualTokenKind::RIGHT_BRACE: return "RIGHT_BRACE";
        case ContextualTokenKind::LEFT_BRACKET: return "LEFT_BRACKET";
        case ContextualTokenKind::RIGHT_BRACKET: return "RIGHT_BRACKET";
        case ContextualTokenKind::SEMICOLON: return "SEMICOLON";
        case ContextualTokenKind::COMMA: return "COMMA";
        case ContextualTokenKind::COLON: return "COLON";
        case ContextualTokenKind::QUESTION: return "QUESTION";
        case ContextualTokenKind::DOT: return "DOT";
        case ContextualTokenKind::ARROW: return "ARROW";
        case ContextualTokenKind::ELLIPSIS: return "ELLIPSIS";
        
        // Standard keywords (direct mapping)
        case ContextualTokenKind::IF: return "IF";
        case ContextualTokenKind::ELSE: return "ELSE";
        case ContextualTokenKind::WHILE: return "WHILE";
        case ContextualTokenKind::FOR: return "FOR";
        case ContextualTokenKind::RETURN: return "RETURN";
        case ContextualTokenKind::BREAK: return "BREAK";
        case ContextualTokenKind::CONTINUE: return "CONTINUE";
        case ContextualTokenKind::VOID: return "VOID";
        case ContextualTokenKind::BOOL: return "BOOL";
        case ContextualTokenKind::CHAR: return "CHAR";
        case ContextualTokenKind::INT: return "INT";
        case ContextualTokenKind::FLOAT: return "FLOAT";
        case ContextualTokenKind::DOUBLE: return "DOUBLE";
        case ContextualTokenKind::CONST: return "CONST";
        case ContextualTokenKind::MUT: return "MUT";
        case ContextualTokenKind::STATIC: return "STATIC";
        case ContextualTokenKind::VOLATILE: return "VOLATILE";
        case ContextualTokenKind::PUBLIC: return "PUBLIC";
        case ContextualTokenKind::PRIVATE: return "PRIVATE";
        case ContextualTokenKind::PROTECTED: return "PROTECTED";
        
        default:
            return "UNKNOWN_CONTEXTUAL_TOKEN_KIND";
    }
}

std::string raw_token_to_string(const RawToken& token, const StringTable& string_table) {
    std::ostringstream ss;
    ss << token_kind_to_string(token.kind) << " ";
    
    if (token.has_string_value() && token.has_valid_string_index()) {
        ss << ", \"" << string_table.get_string(token.string_index) << "\"";
    } else if (token.has_literal_value()) {
        ss << ", " << internal::format_literal_value(token);
    }
    
    ss << " (" << token.line << ":" << token.column << ")";
    return ss.str();
}

// ========================================================================
// Token Sequence Analysis (Layer 1 Debug Functions)
// ========================================================================

std::string tokens_to_string(const std::vector<RawToken>& tokens, const StringTable& string_table) {
    std::ostringstream ss;
    for (size_t i = 0; i < tokens.size(); ++i) {
        ss << "[" << std::setw(3) << i << "] " << raw_token_to_string(tokens[i], string_table);
        if (i < tokens.size() - 1) ss << "\n";
    }
    return ss.str();
}

void print_tokens(const std::vector<RawToken>& tokens, const StringTable& string_table) {
    std::cout << "=== RAW TOKENS (" << tokens.size() << ") ===" << std::endl;
    std::cout << tokens_to_string(tokens, string_table) << std::endl;
    std::cout << "=== END TOKENS ===" << std::endl;
}

void analyze_token_distribution(const std::vector<RawToken>& tokens) {
    std::unordered_map<TokenKind, size_t> distribution;
    
    for (const auto& token : tokens) {
        distribution[token.kind]++;
    }
    
    std::cout << "=== TOKEN DISTRIBUTION ===" << std::endl;
    for (const auto& pair : distribution) {
        std::cout << std::setw(20) << token_kind_to_string(pair.first) 
                  << ": " << std::setw(6) << pair.second << std::endl;
    }
    std::cout << "=== END DISTRIBUTION ===" << std::endl;
}

void print_token_statistics(const std::vector<RawToken>& tokens, const StringTable& string_table) {
    std::cout << "=== TOKEN STATISTICS ===" << std::endl;
    std::cout << "Total tokens: " << tokens.size() << std::endl;
    std::cout << "String table size: " << string_table.size() << std::endl;
    
    size_t keywords = 0, operators = 0, literals = 0, identifiers = 0;
    for (const auto& token : tokens) {
        if (token.is_keyword()) keywords++;
        else if (token.is_operator()) operators++;
        else if (token.is_literal()) literals++;
        else if (token.kind == TokenKind::IDENTIFIER) identifiers++;
    }
    
    std::cout << "Keywords: " << keywords << std::endl;
    std::cout << "Operators: " << operators << std::endl;
    std::cout << "Literals: " << literals << std::endl;
    std::cout << "Identifiers: " << identifiers << std::endl;
    std::cout << "=== END STATISTICS ===" << std::endl;
}

// ========================================================================
// Structured Token Analysis (Layer 2 Debug Functions)
// ========================================================================

std::string structured_tokens_to_debug_string(const StructuredTokens& structured) {
    std::ostringstream ss;
    
    ss << "=== STRUCTURED TOKENS ===\n";
    ss << "Contextualized: " << (structured.is_contextualized() ? "true" : "false") << "\n";
    ss << "Total scopes: " << structured.scopes.size() << "\n";
    ss << "Max nesting depth: " << structured.max_nesting_depth << "\n\n";
    
    for (size_t i = 0; i < structured.scopes.size(); ++i) {
        const auto& scope = structured.scopes[i];
        ss << "Scope[" << i << "]: " << scope_type_to_string(scope.type) << "\n";
        ss << "  Parent: " << (scope.parent_index != StructuredTokens::INVALID_PARENT_INDEX ? 
                             std::to_string(scope.parent_index) : "ROOT") << "\n";
        ss << "  Stream ID: " << scope.raw_token_stream_id << "\n";
        ss << "  Signature tokens: " << scope.signature_tokens.size() << "\n";
        ss << "  Content tokens: " << scope.content.size() << "\n";
        
        if (i < structured.scopes.size() - 1) ss << "\n";
    }
    
    ss << "=== END STRUCTURED TOKENS ===";
    return ss.str();
}

void print_structured_tokens(const StructuredTokens& structured) {
    std::cout << structured_tokens_to_debug_string(structured) << std::endl;
}

void print_scope_hierarchy(const StructuredTokens& structured) {
    std::cout << "=== SCOPE HIERARCHY ===" << std::endl;
    
    // Find root scopes (those with INVALID_PARENT_INDEX parent)
    for (size_t i = 0; i < structured.scopes.size(); ++i) {
        if (structured.scopes[i].parent_index == StructuredTokens::INVALID_PARENT_INDEX) {
            internal::print_scope_tree(structured, i, 0);
        }
    }
    
    std::cout << "=== END HIERARCHY ===" << std::endl;
}

void analyze_scope_distribution(const StructuredTokens& structured) {
    std::unordered_map<Scope::Type, size_t> distribution;
    
    for (const auto& scope : structured.scopes) {
        distribution[scope.type]++;
    }
    
    std::cout << "=== SCOPE DISTRIBUTION ===" << std::endl;
    for (const auto& pair : distribution) {
        std::cout << std::setw(20) << scope_type_to_string(pair.first) 
                  << ": " << std::setw(6) << pair.second << std::endl;
    }
    std::cout << "=== END DISTRIBUTION ===" << std::endl;
}

std::string scope_to_string(const Scope& scope, size_t scope_index, 
                           const StringTable& string_table, bool contextualized) {
    std::ostringstream ss;
    
    ss << "Scope[" << scope_index << "]: " << scope_type_to_string(scope.type) << "\n";
    ss << "  Parent: " << (scope.parent_index != StructuredTokens::INVALID_PARENT_INDEX ? 
                          std::to_string(scope.parent_index) : "ROOT") << "\n";
    
    if (!scope.signature_tokens.empty()) {
        ss << "  Signature: " << internal::format_token_sequence(scope.signature_tokens, string_table, contextualized) << "\n";
    }
    
    if (!scope.content.empty()) {
        ss << "  Content: " << internal::format_token_sequence(scope.content, string_table, contextualized) << "\n";
    }
    
    return ss.str();
}

std::string scope_type_to_string(Scope::Type type) {
    switch (type) {
        case Scope::TopLevel: return "TopLevel";
        case Scope::NamedClass: return "NamedClass";
        case Scope::NamedFunction: return "NamedFunction";
        case Scope::ConditionalScope: return "ConditionalScope";
        case Scope::LoopScope: return "LoopScope";
        case Scope::TryScope: return "TryScope";
        case Scope::NakedScope: return "NakedScope";
        default: return "Unknown";
    }
}

// ========================================================================
// Contextualization Analysis (Layer 3 Debug Functions)
// ========================================================================

void print_contextualization_report(const StructuredTokens& structured) {
    std::cout << "=== CONTEXTUALIZATION REPORT ===" << std::endl;
    std::cout << "Contextualized: " << (structured.is_contextualized() ? "YES" : "NO") << std::endl;
    std::cout << "Total scopes: " << structured.scopes.size() << std::endl;
    
    if (!structured.is_contextualized()) {
        std::cout << "NOTE: Tokens are in raw TokenKind format" << std::endl;
        std::cout << "=== END REPORT ===" << std::endl;
        return;
    }
    
    // Count context-sensitive tokens
    size_t context_sensitive_count = 0;
    for (const auto& scope : structured.scopes) {
        for (uint32_t token_value : scope.signature_tokens) {
            ContextualTokenKind kind = static_cast<ContextualTokenKind>(token_value);
            if (internal::is_context_sensitive_token(kind)) {
                context_sensitive_count++;
            }
        }
        for (uint32_t token_value : scope.content) {
            ContextualTokenKind kind = static_cast<ContextualTokenKind>(token_value);
            if (internal::is_context_sensitive_token(kind)) {
                context_sensitive_count++;
            }
        }
    }
    
    std::cout << "Context-sensitive tokens: " << context_sensitive_count << std::endl;
    std::cout << "=== END REPORT ===" << std::endl;
}

void analyze_contextualization_changes(const StructuredTokens& before, const StructuredTokens& after) {
    std::cout << "=== CONTEXTUALIZATION CHANGES ===" << std::endl;
    
    if (before.is_contextualized() || !after.is_contextualized()) {
        std::cout << "ERROR: Expected before=raw, after=contextualized" << std::endl;
        std::cout << "=== END CHANGES ===" << std::endl;
        return;
    }
    
    std::cout << "Changes will be tracked in future implementation" << std::endl;
    std::cout << "=== END CHANGES ===" << std::endl;
}

void print_context_sensitive_tokens(const StructuredTokens& structured) {
    std::cout << "=== CONTEXT-SENSITIVE TOKENS ===" << std::endl;
    
    if (!structured.is_contextualized()) {
        std::cout << "ERROR: StructuredTokens not contextualized" << std::endl;
        std::cout << "=== END CONTEXT-SENSITIVE ===" << std::endl;
        return;
    }
    
    for (size_t scope_idx = 0; scope_idx < structured.scopes.size(); ++scope_idx) {
        const auto& scope = structured.scopes[scope_idx];
        bool found_any = false;
        
        for (uint32_t token_value : scope.signature_tokens) {
            ContextualTokenKind kind = static_cast<ContextualTokenKind>(token_value);
            if (internal::is_context_sensitive_token(kind)) {
                if (!found_any) {
                    std::cout << "Scope[" << scope_idx << "] signature: ";
                    found_any = true;
                }
                std::cout << contextual_token_kind_to_string(kind) << " ";
            }
        }
        
        for (uint32_t token_value : scope.content) {
            ContextualTokenKind kind = static_cast<ContextualTokenKind>(token_value);
            if (internal::is_context_sensitive_token(kind)) {
                if (!found_any) {
                    std::cout << "Scope[" << scope_idx << "] content: ";
                    found_any = true;
                }
                std::cout << contextual_token_kind_to_string(kind) << " ";
            }
        }
        
        if (found_any) {
            std::cout << std::endl;
        }
    }
    
    std::cout << "=== END CONTEXT-SENSITIVE ===" << std::endl;
}

void analyze_context_resolution_patterns(const StructuredTokens& structured) {
    std::cout << "=== CONTEXT RESOLUTION PATTERNS ===" << std::endl;
    std::cout << "Pattern analysis not yet implemented" << std::endl;
    std::cout << "=== END PATTERNS ===" << std::endl;
}

void print_contextualization_errors(const std::vector<std::string>& errors) {
    std::cout << "=== CONTEXTUALIZATION ERRORS ===" << std::endl;
    if (errors.empty()) {
        std::cout << "No errors found" << std::endl;
    } else {
        for (size_t i = 0; i < errors.size(); ++i) {
            std::cout << "[" << i + 1 << "] " << errors[i] << std::endl;
        }
    }
    std::cout << "=== END ERRORS ===" << std::endl;
}

// ========================================================================
// Private Helper Functions
// ========================================================================

namespace internal {

std::string format_token_position(const RawToken& token) {
    return std::to_string(token.line) + ":" + std::to_string(token.column);
}

std::string format_literal_value(const RawToken& token) {
    // For now, just return a placeholder - full literal formatting can be added later
    return "[literal]";
}

std::string format_token_sequence(const std::vector<uint32_t>& tokens, 
                                 const StringTable& string_table, bool contextualized) {
    if (tokens.empty()) return "[]";
    
    std::ostringstream ss;
    ss << "[";
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (contextualized) {
            // Use contextualized token string function
            ContextualTokenKind kind = static_cast<ContextualTokenKind>(tokens[i]);
            ss << contextual_token_kind_to_string(kind);
        } else {
            // Use raw token string function
            TokenKind kind = static_cast<TokenKind>(tokens[i]);
            ss << token_kind_to_string(kind);
        }
        
        if (i < tokens.size() - 1) ss << ", ";
    }
    
    ss << "]";
    return ss.str();
}

void print_scope_tree(const StructuredTokens& structured, size_t scope_idx, int indent_level) {
    if (scope_idx >= structured.scopes.size()) return;
    
    const auto& scope = structured.scopes[scope_idx];
    
    // Print indentation
    for (int i = 0; i < indent_level; ++i) {
        std::cout << "  ";
    }
    
    // Print scope info
    std::cout << "├─ Scope[" << scope_idx << "]: " << scope_type_to_string(scope.type);
    std::cout << " (sig:" << scope.signature_tokens.size() << ", cont:" << scope.content.size() << ")";
    std::cout << std::endl;
    
    // Print children recursively
    std::vector<size_t> child_indices = structured.get_child_scope_indices(scope_idx);
    for (size_t child_idx : child_indices) {
        print_scope_tree(structured, child_idx, indent_level + 1);
    }
}

bool is_context_sensitive_token(ContextualTokenKind kind) {
    static const std::unordered_set<ContextualTokenKind> context_sensitive_tokens = {
        // Runtime keyword variations
        ContextualTokenKind::RUNTIME_ACCESS_RIGHT,
        ContextualTokenKind::RUNTIME_TYPE_PARAMETER,
        ContextualTokenKind::RUNTIME_VARIABLE_DECL,
        ContextualTokenKind::RUNTIME_UNION_DECLARATION,
        ContextualTokenKind::RUNTIME_COROUTINE,
        
        // Defer keyword variations
        ContextualTokenKind::DEFER_RAII,
        ContextualTokenKind::DEFER_COROUTINE,
        ContextualTokenKind::DEFER_SCOPE_GUARD,
        
        // Class type variations
        ContextualTokenKind::DATA_CLASS,
        ContextualTokenKind::FUNCTIONAL_CLASS,
        ContextualTokenKind::DANGER_CLASS,
        
        // Access right variations
        ContextualTokenKind::EXPOSES_COMPILE_TIME,
        ContextualTokenKind::EXPOSES_RUNTIME,
        
        // Function variations
        ContextualTokenKind::FUNCTION_DECLARATION,
        ContextualTokenKind::ASYNC_FUNCTION_DECLARATION,
        
        // Advanced contextual tokens
        ContextualTokenKind::ACCESS_RIGHT_DECLARATION,
        ContextualTokenKind::ACCESS_RIGHT_USAGE,
        ContextualTokenKind::TYPE_IDENTIFIER,
        ContextualTokenKind::GENERIC_TYPE_PARAMETER,
        ContextualTokenKind::CAPABILITY_GRANT,
        ContextualTokenKind::COROUTINE_YIELD,
        ContextualTokenKind::ASYNC_AWAIT,
        ContextualTokenKind::MEMORY_REGION,
        ContextualTokenKind::COMPILE_TIME_EVAL
    };
    
    return context_sensitive_tokens.count(kind) > 0;
}

std::string format_contextualization_change(TokenKind original, ContextualTokenKind contextual) {
    // This will be implemented when we track changes
    return "CHANGE_TRACKING_NOT_IMPLEMENTED";
}

} // namespace internal

} // namespace cprime::debug_utils