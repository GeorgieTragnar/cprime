#include "token_utils.h"
#include "tokens.h"
#include "string_table.h"
#include <sstream>

namespace cprime {

const char* token_kind_to_string(TokenKind kind) {
    switch (kind) {
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
        
        // Literals
        case TokenKind::TRUE_LITERAL: return "TRUE_LITERAL";
        case TokenKind::FALSE_LITERAL: return "FALSE_LITERAL";
        case TokenKind::NULLPTR_LITERAL: return "NULLPTR_LITERAL";
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
        
        // Dynamic tokens
        case TokenKind::IDENTIFIER: return "IDENTIFIER";
        case TokenKind::COMMENT: return "COMMENT";
        case TokenKind::WHITESPACE: return "WHITESPACE";
        case TokenKind::EOF_TOKEN: return "EOF_TOKEN";
        
        default: return "UNKNOWN_TOKEN_KIND";
    }
}

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

std::string RawToken::to_string(const StringTable& string_table) const {
    std::ostringstream oss;
    oss << token_kind_to_string(kind) << " ";
    
    if (has_string_value()) {
        oss << "\"" << get_string(string_table) << "\"";
    } else if (has_literal_value()) {
        oss << "[typed_literal]";
    }
    
    oss << " (" << line << ":" << column << ")";
    return oss.str();
}

std::string ContextualToken::to_string() const {
    std::ostringstream oss;
    oss << contextual_token_kind_to_string(contextual_kind);
    oss << " (" << raw_token.line << ":" << raw_token.column << ")";
    return oss.str();
}

} // namespace cprime