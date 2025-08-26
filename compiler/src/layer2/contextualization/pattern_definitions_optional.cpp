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
    
    // Log registry state (if method exists)
    // registry.log_registry_state(); // Comment out for now
    
    LOG_INFO("✅ Builtin optional/reusable patterns initialized: {} optional, {} repeatable",
             registry.get_all_optional_keys().size(), registry.get_all_repeatable_keys().size());
}

// Optional Assignment Pattern: [= expression]
void OptionalPatternDefinitions::create_optional_assignment_pattern(ReusablePatternRegistry& registry) {
    std::vector<PatternElement> elements = {
        PatternElement(EToken::ASSIGN, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::EXPRESSION)
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

} // namespace cprime::layer2_contextualization