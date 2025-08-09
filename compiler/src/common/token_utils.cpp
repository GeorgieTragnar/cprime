#include "token_utils.h"
#include "tokens.h"
#include "string_table.h"
#include <sstream>

namespace cprime {




bool is_literal(TokenKind kind) {
    return kind >= TokenKind::TRUE_LITERAL && kind <= TokenKind::RAW_STRING_LITERAL;
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