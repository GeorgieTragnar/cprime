#include "pattern_definitions_body.h"
#include "contextualization_pattern_matcher.h"
#include "../../commons/logger.h"

namespace cprime::layer2_contextualization {

// Initialize all body patterns into the matcher
void BodyPatternDefinitions::initialize_builtin_body_patterns(ContextualizationPatternMatcher& matcher) {
    auto logger = cprime::LoggerFactory::get_logger("body_pattern_definitions");
    LOG_INFO("🏗️ Initializing builtin body patterns");
    
    // Variable and assignment patterns
    create_variable_declaration_with_assignment_pattern(matcher);
    create_variable_declaration_without_assignment_pattern(matcher);
    create_complex_variable_declaration_pattern(matcher);
    create_assignment_statement_pattern(matcher);
    
    // Function and expression patterns
    create_function_call_pattern(matcher);
    create_expression_statement_pattern(matcher);
    
    // Control flow patterns
    create_if_statement_pattern(matcher);
    create_while_loop_pattern(matcher);
    create_for_loop_pattern(matcher);
    
    // Other patterns
    // NOTE: Comment patterns are not needed - comments are filtered out during preprocessing
    
    LOG_INFO("✅ Builtin body patterns initialized: {} patterns registered", 
             matcher.get_body_pattern_count());
}

// Body Pattern 1: Variable Declaration with Assignment
// Pattern: [OPTIONAL_WHITESPACE] TYPE REQUIRED_WHITESPACE IDENTIFIER [OPTIONAL_ASSIGNMENT] [OPTIONAL_WHITESPACE] ; [OPTIONAL_WHITESPACE]
void BodyPatternDefinitions::create_variable_declaration_with_assignment_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Type (primitive keywords or identifiers that resolve to types)
        PatternElement({EToken::INT32_T, EToken::FLOAT, EToken::DOUBLE, EToken::BOOL, EToken::CHAR, EToken::VOID, EToken::IDENTIFIER}, EContextualToken::TYPE_REFERENCE),
        PatternElement(PatternElementType::REQUIRED_WHITESPACE, EContextualToken::INVALID),
        // Variable name
        PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::VARIABLE_DECLARATION),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Use the optional assignment pattern which handles the full "= expression" part
        PatternElement(PatternKey::OPTIONAL_ASSIGNMENT, EContextualToken::INVALID),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // No semicolon needed - instructions are pre-split at semicolon boundaries
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern variable_assignment_pattern("variable_declaration_with_assignment", elements);
    matcher.register_body_pattern(variable_assignment_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("body_pattern_definitions");
    LOG_DEBUG("Registered body pattern: variable_declaration_with_assignment");
}

// Body Pattern 2: Variable Declaration without Assignment
// Pattern: [OPTIONAL_WHITESPACE] TYPE REQUIRED_WHITESPACE IDENTIFIER [OPTIONAL_WHITESPACE] ; [OPTIONAL_WHITESPACE]
void BodyPatternDefinitions::create_variable_declaration_without_assignment_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Type (primitive keywords or identifiers that resolve to types)
        PatternElement({EToken::INT32_T, EToken::FLOAT, EToken::DOUBLE, EToken::BOOL, EToken::CHAR, EToken::VOID, EToken::IDENTIFIER}, EContextualToken::TYPE_REFERENCE),
        PatternElement(PatternElementType::REQUIRED_WHITESPACE, EContextualToken::INVALID),
        // Variable name
        PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::VARIABLE_DECLARATION),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // No semicolon needed - instructions are pre-split at semicolon boundaries
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern variable_declaration_pattern("variable_declaration_without_assignment", elements);
    matcher.register_body_pattern(variable_declaration_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("body_pattern_definitions");
    LOG_DEBUG("Registered body pattern: variable_declaration_without_assignment");
}

// Body Pattern 3: Complex Variable Declaration (with modifiers and namespaces)
// Pattern: [OPTIONAL_WHITESPACE] [MODIFIERS] TYPE [::NAMESPACE::] IDENTIFIER [= EXPRESSION] [OPTIONAL_WHITESPACE] ; [OPTIONAL_WHITESPACE]
void BodyPatternDefinitions::create_complex_variable_declaration_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Optional type modifiers (const, static, etc.) - can use reusable patterns
        PatternElement({EToken::CONST, EToken::STATIC, EToken::VOLATILE}, EContextualToken::TYPE_REFERENCE),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Type with potential namespacing
        PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::TYPE_REFERENCE),
        PatternElement(PatternElementType::REQUIRED_WHITESPACE, EContextualToken::INVALID),
        // Variable name
        PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::VARIABLE_DECLARATION),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Optional assignment with full expression support
        PatternElement(PatternKey::OPTIONAL_ASSIGNMENT, EContextualToken::INVALID),
        // Statement terminator
        PatternElement(EToken::SEMICOLON, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern complex_variable_pattern("complex_variable_declaration", elements);
    matcher.register_body_pattern(complex_variable_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("body_pattern_definitions");
    LOG_DEBUG("Registered body pattern: complex_variable_declaration");
}

// Body Pattern 4: Assignment Statement
// Pattern: [OPTIONAL_WHITESPACE] IDENTIFIER = EXPRESSION [OPTIONAL_WHITESPACE] ; [OPTIONAL_WHITESPACE]
void BodyPatternDefinitions::create_assignment_statement_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Variable name (can be namespaced)
        PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::VARIABLE_DECLARATION),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Assignment operator
        PatternElement(EToken::ASSIGN, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Full expression support for the assigned value
        PatternElement(PatternKey::MANDATORY_EXPRESSION, EContextualToken::INVALID),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // No semicolon needed - instructions are pre-split at semicolon boundaries
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern assignment_statement_pattern("assignment_statement", elements);
    matcher.register_body_pattern(assignment_statement_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("body_pattern_definitions");
    LOG_DEBUG("Registered body pattern: assignment_statement");
}

// Body Pattern 5: Function Call
// Pattern: [OPTIONAL_WHITESPACE] NAMESPACED_IDENTIFIER ( [PARAMETERS] ) [OPTIONAL_WHITESPACE] ; [OPTIONAL_WHITESPACE]
void BodyPatternDefinitions::create_function_call_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Function name (can be namespaced)
        PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::FUNCTION_CALL),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Opening parenthesis
        PatternElement(EToken::LEFT_PAREN, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Parameters (simplified for now)
        PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::EXPRESSION),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Closing parenthesis
        PatternElement(EToken::RIGHT_PAREN, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Statement terminator
        PatternElement(EToken::SEMICOLON, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern function_call_pattern("function_call", elements);
    matcher.register_body_pattern(function_call_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("body_pattern_definitions");
    LOG_DEBUG("Registered body pattern: function_call");
}

// Body Pattern 6: Expression Statement
// Pattern: [OPTIONAL_WHITESPACE] EXPRESSION [OPTIONAL_WHITESPACE] ; [OPTIONAL_WHITESPACE]
void BodyPatternDefinitions::create_expression_statement_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Expression (identifier, literal, or complex expression)
        PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::EXPRESSION),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Statement terminator
        PatternElement(EToken::SEMICOLON, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern expression_statement_pattern("expression_statement", elements);
    matcher.register_body_pattern(expression_statement_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("body_pattern_definitions");
    LOG_DEBUG("Registered body pattern: expression_statement");
}

// Body Pattern 7: If Statement
// Pattern: [OPTIONAL_WHITESPACE] IF [OPTIONAL_WHITESPACE] ( EXPRESSION ) [OPTIONAL_WHITESPACE] { [OPTIONAL_WHITESPACE]
void BodyPatternDefinitions::create_if_statement_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::IF, EContextualToken::CONTROL_FLOW),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Condition in parentheses with full expression support
        PatternElement(EToken::LEFT_PAREN, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternKey::MANDATORY_EXPRESSION, EContextualToken::INVALID),  // Full expression support for conditions
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::RIGHT_PAREN, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Opening brace
        PatternElement(EToken::LEFT_BRACE, EContextualToken::SCOPE_REFERENCE),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern if_statement_pattern("if_statement", elements);
    matcher.register_body_pattern(if_statement_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("body_pattern_definitions");
    LOG_DEBUG("Registered body pattern: if_statement");
}

// Body Pattern 8: While Loop
// Pattern: [OPTIONAL_WHITESPACE] WHILE [OPTIONAL_WHITESPACE] ( EXPRESSION ) [OPTIONAL_WHITESPACE] { [OPTIONAL_WHITESPACE]
void BodyPatternDefinitions::create_while_loop_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::WHILE, EContextualToken::CONTROL_FLOW),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Condition in parentheses with full expression support
        PatternElement(EToken::LEFT_PAREN, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternKey::MANDATORY_EXPRESSION, EContextualToken::INVALID),  // Full expression support for conditions
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::RIGHT_PAREN, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Opening brace
        PatternElement(EToken::LEFT_BRACE, EContextualToken::SCOPE_REFERENCE),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern while_loop_pattern("while_loop", elements);
    matcher.register_body_pattern(while_loop_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("body_pattern_definitions");
    LOG_DEBUG("Registered body pattern: while_loop");
}

// Body Pattern 9: For Loop
// Pattern: [OPTIONAL_WHITESPACE] FOR [OPTIONAL_WHITESPACE] ( INIT ; CONDITION ; INCREMENT ) [OPTIONAL_WHITESPACE] { [OPTIONAL_WHITESPACE]
void BodyPatternDefinitions::create_for_loop_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::FOR, EContextualToken::CONTROL_FLOW),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Opening parenthesis
        PatternElement(EToken::LEFT_PAREN, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Initialization (simplified)
        PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::VARIABLE_DECLARATION),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::SEMICOLON, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Condition
        PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::EXPRESSION),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::SEMICOLON, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Increment
        PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::EXPRESSION),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::RIGHT_PAREN, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Opening brace
        PatternElement(EToken::LEFT_BRACE, EContextualToken::SCOPE_REFERENCE),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern for_loop_pattern("for_loop", elements);
    matcher.register_body_pattern(for_loop_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("body_pattern_definitions");
    LOG_DEBUG("Registered body pattern: for_loop");
}

// NOTE: Comment patterns are not needed - comments are filtered out during preprocessing
// so pattern matching should never see comment tokens

} // namespace cprime::layer2_contextualization