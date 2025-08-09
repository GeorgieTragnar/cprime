#include "contextual_token_kind.h"

namespace cprime {

const char* contextual_token_kind_to_string(ContextualTokenKind kind) {
    switch (kind) {
        // Basic tokens
        case ContextualTokenKind::IDENTIFIER: return "IDENTIFIER";
        case ContextualTokenKind::COMMENT: return "COMMENT";
        case ContextualTokenKind::WHITESPACE: return "WHITESPACE";
        case ContextualTokenKind::EOF_TOKEN: return "EOF_TOKEN";
        
        // Literals
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
        
        // Context-sensitive interpretations
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
        
        // Future features
        case ContextualTokenKind::CAPABILITY_GRANT: return "CAPABILITY_GRANT";
        case ContextualTokenKind::COROUTINE_YIELD: return "COROUTINE_YIELD";
        case ContextualTokenKind::ASYNC_AWAIT: return "ASYNC_AWAIT";
        case ContextualTokenKind::MEMORY_REGION: return "MEMORY_REGION";
        case ContextualTokenKind::COMPILE_TIME_EVAL: return "COMPILE_TIME_EVAL";
        
        // Error handling
        case ContextualTokenKind::CONTEXTUAL_TODO: return "CONTEXTUAL_TODO";
        case ContextualTokenKind::CONTEXTUAL_ERROR: return "CONTEXTUAL_ERROR";
        case ContextualTokenKind::CONTEXTUAL_UNKNOWN: return "CONTEXTUAL_UNKNOWN";
        
        // Operators (abbreviated for brevity)
        case ContextualTokenKind::PLUS: return "PLUS";
        case ContextualTokenKind::MINUS: return "MINUS";
        case ContextualTokenKind::LEFT_PAREN: return "LEFT_PAREN";
        case ContextualTokenKind::RIGHT_PAREN: return "RIGHT_PAREN";
        // ... (add more as needed)
        
        default: return "UNKNOWN_CONTEXTUAL_TOKEN_KIND";
    }
}

bool is_contextual_literal(ContextualTokenKind kind) {
    return kind >= ContextualTokenKind::INT_LITERAL && kind <= ContextualTokenKind::NULLPTR_LITERAL;
}

bool is_contextual_operator(ContextualTokenKind kind) {
    return kind >= ContextualTokenKind::PLUS && kind <= ContextualTokenKind::ELLIPSIS;
}

bool is_contextual_keyword(ContextualTokenKind kind) {
    return (kind >= ContextualTokenKind::IF && kind <= ContextualTokenKind::NAMESPACE) ||
           (kind >= ContextualTokenKind::RUNTIME_ACCESS_RIGHT && kind <= ContextualTokenKind::ASYNC_FUNCTION_DECLARATION);
}

bool is_contextual_type_declaration(ContextualTokenKind kind) {
    return kind == ContextualTokenKind::DATA_CLASS ||
           kind == ContextualTokenKind::FUNCTIONAL_CLASS ||
           kind == ContextualTokenKind::DANGER_CLASS ||
           kind == ContextualTokenKind::STRUCT_DECLARATION ||
           kind == ContextualTokenKind::UNION_DECLARATION ||
           kind == ContextualTokenKind::INTERFACE_DECLARATION ||
           kind == ContextualTokenKind::PLEX_DECLARATION;
}

bool is_context_sensitive(ContextualTokenKind kind) {
    return kind >= ContextualTokenKind::RUNTIME_ACCESS_RIGHT && 
           kind <= ContextualTokenKind::GENERIC_TYPE_PARAMETER;
}

} // namespace cprime