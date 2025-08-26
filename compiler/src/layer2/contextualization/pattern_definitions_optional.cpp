#include "pattern_definitions_optional.h"
#include "../../commons/logger.h"

namespace cprime::layer2_contextualization {

// Initialize all optional/reusable patterns into the registry
void OptionalPatternDefinitions::initialize_builtin_optional_patterns(ReusablePatternRegistry& registry) {
    auto logger = cprime::LoggerFactory::get_logger("optional_pattern_definitions");
    LOG_INFO("🏗️ Initializing builtin optional/reusable patterns");
    
    // Create optional patterns
    create_optional_assignment_pattern(registry);
    create_optional_type_modifier_pattern(registry);
    create_optional_access_modifier_pattern(registry);
    create_optional_whitespace_pattern(registry);
    
    // Create repeatable patterns
    create_repeatable_namespace_pattern(registry);
    create_repeatable_parameter_list_pattern(registry);
    create_repeatable_template_args_pattern(registry);
    
    // Create expression patterns
    create_base_expression_pattern(registry);
    create_mandatory_expression_pattern(registry);
    create_optional_parenthesized_pattern(registry);
    create_optional_binary_operator_pattern(registry);
    create_optional_unary_operator_pattern(registry);
    
    // Log registry state (if method exists)
    // registry.log_registry_state(); // Comment out for now
    
    LOG_INFO("✅ Builtin optional/reusable patterns initialized: {} optional, {} repeatable",
             registry.get_all_optional_keys().size(), registry.get_all_repeatable_keys().size());
}

// Optional Assignment Pattern: [= expression]
void OptionalPatternDefinitions::create_optional_assignment_pattern(ReusablePatternRegistry& registry) {
    std::vector<PatternElement> elements = {
        PatternElement(EToken::ASSIGN, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternKey::MANDATORY_EXPRESSION, EContextualToken::INVALID)  // Use full expression support
    };
    Pattern assignment_pattern("optional_assignment", elements);
    registry.register_optional_pattern(PatternKey::OPTIONAL_ASSIGNMENT, assignment_pattern, 
                                      "Optional assignment: = expression");
}

// Optional Type Modifier Pattern: [const|volatile|static]
void OptionalPatternDefinitions::create_optional_type_modifier_pattern(ReusablePatternRegistry& registry) {
    std::vector<PatternElement> elements = {
        PatternElement({EToken::CONST, EToken::VOLATILE, EToken::STATIC}, EContextualToken::TYPE_REFERENCE)
    };
    Pattern modifier_pattern("optional_type_modifier", elements);
    registry.register_optional_pattern(PatternKey::OPTIONAL_TYPE_MODIFIER, modifier_pattern,
                                      "Optional type modifier: const|volatile|static");
}

// Optional Access Modifier Pattern: [public|private|protected] or multiple type modifiers
void OptionalPatternDefinitions::create_optional_access_modifier_pattern(ReusablePatternRegistry& registry) {
    std::vector<PatternElement> elements = {
        PatternElement({EToken::CONST, EToken::STATIC, EToken::VOLATILE}, EContextualToken::TYPE_REFERENCE)
    };
    Pattern multi_modifier_pattern("optional_access_modifier", elements);
    registry.register_optional_pattern(PatternKey::OPTIONAL_ACCESS_MODIFIER, multi_modifier_pattern,
                                      "Optional multiple type modifiers");
}

// Optional Whitespace Pattern: [whitespace]*
void OptionalPatternDefinitions::create_optional_whitespace_pattern(ReusablePatternRegistry& registry) {
    // Note: This is handled specially by the pattern matching engine
    // We register it for completeness but it's implemented as OPTIONAL_WHITESPACE element type
    std::vector<PatternElement> elements = {
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID)
    };
    Pattern whitespace_pattern("optional_whitespace_pattern", elements);
    registry.register_optional_pattern(PatternKey::OPTIONAL_WHITESPACE_PATTERN, whitespace_pattern,
                                      "Optional whitespace pattern");
}

// Repeatable Namespace Pattern: (::identifier)+
void OptionalPatternDefinitions::create_repeatable_namespace_pattern(ReusablePatternRegistry& registry) {
    std::vector<PatternElement> elements = {
        PatternElement(EToken::COLON, EContextualToken::SCOPE_REFERENCE),
        PatternElement(EToken::COLON, EContextualToken::SCOPE_REFERENCE),
        PatternElement(EToken::IDENTIFIER, EContextualToken::SCOPE_REFERENCE)
    };
    Pattern namespace_pattern("repeatable_namespace", elements);
    registry.register_repeatable_pattern(PatternKey::REPEATABLE_NAMESPACE, namespace_pattern,
                                        "Repeatable namespace resolution: ::identifier");
}

// Repeatable Parameter List Pattern: (parameter,)*
void OptionalPatternDefinitions::create_repeatable_parameter_list_pattern(ReusablePatternRegistry& registry) {
    std::vector<PatternElement> elements = {
        PatternElement({EToken::INT32_T, EToken::FLOAT, EToken::DOUBLE, EToken::BOOL, EToken::CHAR, EToken::VOID, EToken::IDENTIFIER}, EContextualToken::TYPE_REFERENCE),
        PatternElement(PatternElementType::REQUIRED_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::IDENTIFIER, EContextualToken::VARIABLE_DECLARATION),
        PatternElement(EToken::COMMA, EContextualToken::OPERATOR)
    };
    Pattern parameter_pattern("repeatable_parameter_list", elements);
    registry.register_repeatable_pattern(PatternKey::REPEATABLE_PARAMETER_LIST, parameter_pattern,
                                        "Repeatable parameter list: type identifier,");
}

// Repeatable Template Args Pattern: (<type,>)*
void OptionalPatternDefinitions::create_repeatable_template_args_pattern(ReusablePatternRegistry& registry) {
    std::vector<PatternElement> elements = {
        PatternElement(EToken::LESS_THAN, EContextualToken::OPERATOR),
        PatternElement({EToken::INT32_T, EToken::FLOAT, EToken::DOUBLE, EToken::BOOL, EToken::CHAR, EToken::VOID, EToken::IDENTIFIER}, EContextualToken::TYPE_REFERENCE),
        PatternElement(EToken::COMMA, EContextualToken::OPERATOR),
        PatternElement(EToken::GREATER_THAN, EContextualToken::OPERATOR)
    };
    Pattern template_pattern("repeatable_template_args", elements);
    registry.register_repeatable_pattern(PatternKey::REPEATABLE_TEMPLATE_ARGS, template_pattern,
                                        "Repeatable template arguments: <type,>");
}

// Base Expression Pattern: literals and identifiers
void OptionalPatternDefinitions::create_base_expression_pattern(ReusablePatternRegistry& registry) {
    std::vector<PatternElement> elements = {
        // Support all literal types and identifiers as base expressions
        PatternElement({
            EToken::IDENTIFIER,
            EToken::INT_LITERAL,
            EToken::UINT_LITERAL, 
            EToken::LONG_LITERAL,
            EToken::ULONG_LITERAL,
            EToken::LONG_LONG_LITERAL,
            EToken::ULONG_LONG_LITERAL,
            EToken::FLOAT_LITERAL,
            EToken::DOUBLE_LITERAL,
            EToken::LONG_DOUBLE_LITERAL,
            EToken::CHAR_LITERAL,
            EToken::STRING_LITERAL,
            EToken::TRUE_LITERAL,
            EToken::FALSE_LITERAL,
            EToken::NULLPTR_LITERAL
        }, EContextualToken::EXPRESSION)
    };
    Pattern base_expression_pattern("base_expression", elements);
    registry.register_optional_pattern(PatternKey::BASE_EXPRESSION, base_expression_pattern,
                                      "Base expressions: literals and identifiers");
}

// Mandatory Expression Pattern: recursive expression composition
void OptionalPatternDefinitions::create_mandatory_expression_pattern(ReusablePatternRegistry& registry) {
    // This is a composite pattern that will be handled by the pattern matcher
    // It delegates to BASE_EXPRESSION with optional PARENTHESIZED, BINARY_OPERATOR, and UNARY_OPERATOR
    std::vector<PatternElement> elements = {
        PatternElement(PatternKey::BASE_EXPRESSION, EContextualToken::INVALID)
    };
    Pattern mandatory_expression_pattern("mandatory_expression", elements);
    registry.register_optional_pattern(PatternKey::MANDATORY_EXPRESSION, mandatory_expression_pattern,
                                      "Mandatory expression with recursive composition");
}

// Optional Parenthesized Expression Pattern: ( expression )
void OptionalPatternDefinitions::create_optional_parenthesized_pattern(ReusablePatternRegistry& registry) {
    std::vector<PatternElement> elements = {
        PatternElement(EToken::LEFT_PAREN, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternKey::MANDATORY_EXPRESSION, EContextualToken::INVALID),  // Recursive expression
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::RIGHT_PAREN, EContextualToken::OPERATOR)
    };
    Pattern parenthesized_pattern("optional_parenthesized", elements);
    registry.register_optional_pattern(PatternKey::OPTIONAL_PARENTHESIZED, parenthesized_pattern,
                                      "Optional parenthesized expression: ( expression )");
}

// Optional Binary Operator Pattern: expression OP expression
void OptionalPatternDefinitions::create_optional_binary_operator_pattern(ReusablePatternRegistry& registry) {
    std::vector<PatternElement> elements = {
        PatternElement(PatternKey::MANDATORY_EXPRESSION, EContextualToken::INVALID),  // Left expression
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement({
            // Arithmetic operators
            EToken::PLUS, EToken::MINUS, EToken::MULTIPLY, EToken::DIVIDE, EToken::MODULO,
            // Comparison operators  
            EToken::EQUALS, EToken::NOT_EQUALS, 
            EToken::LESS_THAN, EToken::GREATER_THAN, EToken::LESS_EQUAL, EToken::GREATER_EQUAL,
            // Logical operators
            EToken::LOGICAL_AND, EToken::LOGICAL_OR
        }, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternKey::MANDATORY_EXPRESSION, EContextualToken::INVALID)  // Right expression
    };
    Pattern binary_operator_pattern("optional_binary_operator", elements);
    registry.register_optional_pattern(PatternKey::OPTIONAL_BINARY_OPERATOR, binary_operator_pattern,
                                      "Optional binary operator: expression OP expression");
}

// Optional Unary Operator Pattern: OP expression
void OptionalPatternDefinitions::create_optional_unary_operator_pattern(ReusablePatternRegistry& registry) {
    std::vector<PatternElement> elements = {
        PatternElement({
            EToken::LOGICAL_NOT,  // !
            EToken::PLUS,         // Unary +
            EToken::MINUS         // Unary -
            // TODO: Add increment/decrement when tokens are available
        }, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternKey::MANDATORY_EXPRESSION, EContextualToken::INVALID)  // Recursive expression
    };
    Pattern unary_operator_pattern("optional_unary_operator", elements);
    registry.register_optional_pattern(PatternKey::OPTIONAL_UNARY_OPERATOR, unary_operator_pattern,
                                      "Optional unary operator: OP expression");
}

} // namespace cprime::layer2_contextualization