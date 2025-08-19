#include "instruction_contextualizer.h"
#include "../../commons/enum/contextualToken.h"
#include "../../commons/logger.h"

namespace cprime::layer2_contextualization {

// Setup basic patterns for incremental development
void setup_basic_patterns(InstructionContextualizer& contextualizer) {
    auto logger = cprime::LoggerFactory::get_logger("pattern_definitions");
    LOG_INFO("Setting up basic contextualization patterns");
    
    // Phase 1: Simple type declarations
    setup_declaration_patterns(contextualizer);
    
    // Phase 2: Assignment patterns
    setup_assignment_patterns(contextualizer);
    
    // Phase 3: Function call patterns
    setup_function_call_patterns(contextualizer);
    
    // Phase 4: Operator patterns  
    setup_operator_patterns(contextualizer);
    
    // Phase 5: Whitespace patterns
    setup_whitespace_patterns(contextualizer);
    
    LOG_INFO("Basic pattern setup complete - {} patterns registered", contextualizer.pattern_count());
}

void setup_declaration_patterns(InstructionContextualizer& contextualizer) {
    auto logger = cprime::LoggerFactory::get_logger("pattern_definitions");
    LOG_DEBUG("Setting up declaration patterns");
    
    // Pattern: int identifier;
    // Example: int variable;
    ContextualizationPattern int_declaration(
        "int_variable_declaration",
        {PatternElement::KEYWORD_INT, PatternElement::ANY_IDENTIFIER, PatternElement::LITERAL_SEMICOLON},
        {
            ContextualTokenTemplate(EContextualToken::TYPE_REFERENCE, {0}, "int keyword"),
            ContextualTokenTemplate(EContextualToken::VARIABLE_DECLARATION, {1, 0}, "variable with type"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {2}, "statement terminator")
        },
        100  // High priority for specific patterns
    );
    contextualizer.register_pattern(int_declaration);
    
    // Pattern: func identifier()
    // Example: func main()
    ContextualizationPattern func_declaration(
        "function_declaration",
        {PatternElement::KEYWORD_FUNC, PatternElement::ANY_IDENTIFIER, 
         PatternElement::LITERAL_PAREN_L, PatternElement::LITERAL_PAREN_R},
        {
            ContextualTokenTemplate(EContextualToken::TYPE_REFERENCE, {0}, "func keyword"),
            ContextualTokenTemplate(EContextualToken::FUNCTION_CALL, {1, 0}, "function with return type"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {2, 3}, "function parentheses")
        },
        100
    );
    contextualizer.register_pattern(func_declaration);
    
    // Pattern: int identifier = literal;
    // Example: int count = 42;
    ContextualizationPattern int_initialization(
        "int_variable_initialization", 
        {PatternElement::KEYWORD_INT, PatternElement::ANY_IDENTIFIER, 
         PatternElement::LITERAL_ASSIGN, PatternElement::ANY_LITERAL, PatternElement::LITERAL_SEMICOLON},
        {
            ContextualTokenTemplate(EContextualToken::TYPE_REFERENCE, {0}, "int keyword"),
            ContextualTokenTemplate(EContextualToken::VARIABLE_DECLARATION, {1, 0, 3}, "variable with type and initializer"),
            ContextualTokenTemplate(EContextualToken::ASSIGNMENT, {2}, "assignment operator"),
            ContextualTokenTemplate(EContextualToken::LITERAL_VALUE, {3}, "initializer value"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {4}, "statement terminator")
        },
        120  // Higher priority than simple declaration
    );
    contextualizer.register_pattern(int_initialization);
    
    LOG_DEBUG("Declaration patterns registered");
}

void setup_assignment_patterns(InstructionContextualizer& contextualizer) {
    auto logger = cprime::LoggerFactory::get_logger("pattern_definitions");
    LOG_DEBUG("Setting up assignment patterns");
    
    // Pattern: identifier = identifier;
    // Example: result = value;
    ContextualizationPattern variable_assignment(
        "variable_assignment",
        {PatternElement::ANY_IDENTIFIER, PatternElement::LITERAL_ASSIGN, 
         PatternElement::ANY_IDENTIFIER, PatternElement::LITERAL_SEMICOLON},
        {
            ContextualTokenTemplate(EContextualToken::VARIABLE_DECLARATION, {0}, "assignment target"),
            ContextualTokenTemplate(EContextualToken::ASSIGNMENT, {1}, "assignment operator"),
            ContextualTokenTemplate(EContextualToken::EXPRESSION, {2}, "assignment source"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {3}, "statement terminator")
        },
        80
    );
    contextualizer.register_pattern(variable_assignment);
    
    // Pattern: identifier = literal;
    // Example: count = 10;
    ContextualizationPattern literal_assignment(
        "literal_assignment",
        {PatternElement::ANY_IDENTIFIER, PatternElement::LITERAL_ASSIGN, 
         PatternElement::ANY_LITERAL, PatternElement::LITERAL_SEMICOLON},
        {
            ContextualTokenTemplate(EContextualToken::VARIABLE_DECLARATION, {0}, "assignment target"),
            ContextualTokenTemplate(EContextualToken::ASSIGNMENT, {1}, "assignment operator"),
            ContextualTokenTemplate(EContextualToken::LITERAL_VALUE, {2}, "literal value"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {3}, "statement terminator")
        },
        85  // Slightly higher priority than variable assignment
    );
    contextualizer.register_pattern(literal_assignment);
    
    LOG_DEBUG("Assignment patterns registered");
}

void setup_function_call_patterns(InstructionContextualizer& contextualizer) {
    auto logger = cprime::LoggerFactory::get_logger("pattern_definitions");
    LOG_DEBUG("Setting up function call patterns");
    
    // Pattern: identifier();
    // Example: print();
    ContextualizationPattern simple_function_call(
        "simple_function_call",
        {PatternElement::ANY_IDENTIFIER, PatternElement::LITERAL_PAREN_L, PatternElement::LITERAL_PAREN_R},
        {
            ContextualTokenTemplate(EContextualToken::FUNCTION_CALL, {0, 1, 2}, "function call"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {1, 2}, "function call parentheses")
        },
        90
    );
    contextualizer.register_pattern(simple_function_call);
    
    // Pattern: identifier(identifier);
    // Example: print(message);
    ContextualizationPattern function_call_with_arg(
        "function_call_with_argument",
        {PatternElement::ANY_IDENTIFIER, PatternElement::LITERAL_PAREN_L, 
         PatternElement::ANY_IDENTIFIER, PatternElement::LITERAL_PAREN_R},
        {
            ContextualTokenTemplate(EContextualToken::FUNCTION_CALL, {0, 1, 2, 3}, "function call with argument"),
            ContextualTokenTemplate(EContextualToken::EXPRESSION, {2}, "function argument"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {1, 3}, "function call parentheses")
        },
        95  // Higher priority than simple function call
    );
    contextualizer.register_pattern(function_call_with_arg);
    
    // Pattern: identifier(literal);
    // Example: print("hello");
    ContextualizationPattern function_call_with_literal(
        "function_call_with_literal",
        {PatternElement::ANY_IDENTIFIER, PatternElement::LITERAL_PAREN_L, 
         PatternElement::ANY_LITERAL, PatternElement::LITERAL_PAREN_R},
        {
            ContextualTokenTemplate(EContextualToken::FUNCTION_CALL, {0, 1, 2, 3}, "function call with literal"),
            ContextualTokenTemplate(EContextualToken::LITERAL_VALUE, {2}, "literal argument"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {1, 3}, "function call parentheses")
        },
        97  // Higher priority than simple function call
    );
    contextualizer.register_pattern(function_call_with_literal);
    
    LOG_DEBUG("Function call patterns registered");
}

void setup_operator_patterns(InstructionContextualizer& contextualizer) {
    auto logger = cprime::LoggerFactory::get_logger("pattern_definitions");
    LOG_DEBUG("Setting up operator patterns");
    
    // Pattern: identifier + identifier
    // Example: a + b
    ContextualizationPattern addition_expression(
        "addition_expression",
        {PatternElement::ANY_IDENTIFIER, PatternElement::LITERAL_PLUS, PatternElement::ANY_IDENTIFIER},
        {
            ContextualTokenTemplate(EContextualToken::EXPRESSION, {0, 1, 2}, "addition expression"),
            ContextualTokenTemplate(EContextualToken::EXPRESSION, {0}, "left operand"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {1}, "addition operator"),
            ContextualTokenTemplate(EContextualToken::EXPRESSION, {2}, "right operand")
        },
        70
    );
    contextualizer.register_pattern(addition_expression);
    
    // Pattern: identifier - identifier  
    // Example: a - b
    ContextualizationPattern subtraction_expression(
        "subtraction_expression",
        {PatternElement::ANY_IDENTIFIER, PatternElement::LITERAL_MINUS, PatternElement::ANY_IDENTIFIER},
        {
            ContextualTokenTemplate(EContextualToken::EXPRESSION, {0, 1, 2}, "subtraction expression"),
            ContextualTokenTemplate(EContextualToken::EXPRESSION, {0}, "left operand"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {1}, "subtraction operator"),
            ContextualTokenTemplate(EContextualToken::EXPRESSION, {2}, "right operand")
        },
        70
    );
    contextualizer.register_pattern(subtraction_expression);
    
    LOG_DEBUG("Operator patterns registered");
}

void setup_whitespace_patterns(InstructionContextualizer& contextualizer) {
    auto logger = cprime::LoggerFactory::get_logger("pattern_definitions");
    LOG_DEBUG("Setting up whitespace patterns");
    
    // Pattern: keyword REQUIRED_WHITESPACE identifier;
    // Example: int variable; (with required space between int and variable)
    ContextualizationPattern spaced_int_declaration(
        "spaced_int_declaration",
        {PatternElement::KEYWORD_INT, PatternElement::REQUIRED_WHITESPACE, 
         PatternElement::ANY_IDENTIFIER, PatternElement::LITERAL_SEMICOLON},
        {
            ContextualTokenTemplate(EContextualToken::TYPE_REFERENCE, {0}, "int keyword"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {1}, "required spacing"),
            ContextualTokenTemplate(EContextualToken::VARIABLE_DECLARATION, {2, 0}, "variable with type"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {3}, "statement terminator")
        },
        150  // Higher priority than basic int declaration
    );
    contextualizer.register_pattern(spaced_int_declaration);
    
    // Pattern: identifier MERGED_WHITESPACE = OPTIONAL_WHITESPACE literal;
    // Example: count     =    42; (with variable whitespace around assignment)
    ContextualizationPattern flexible_assignment(
        "flexible_assignment",
        {PatternElement::ANY_IDENTIFIER, PatternElement::OPTIONAL_WHITESPACE,
         PatternElement::LITERAL_ASSIGN, PatternElement::OPTIONAL_WHITESPACE,
         PatternElement::ANY_LITERAL, PatternElement::LITERAL_SEMICOLON},
        {
            ContextualTokenTemplate(EContextualToken::VARIABLE_DECLARATION, {0}, "assignment target"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {1}, "pre-assignment spacing"),
            ContextualTokenTemplate(EContextualToken::ASSIGNMENT, {2}, "assignment operator"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {3}, "post-assignment spacing"),
            ContextualTokenTemplate(EContextualToken::LITERAL_VALUE, {4}, "literal value"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {5}, "statement terminator")
        },
        110  // Higher priority than basic assignment
    );
    contextualizer.register_pattern(flexible_assignment);
    
    // Pattern: MERGED_WHITESPACE (standalone whitespace blocks)
    // Example: multiple spaces/newlines that should be preserved as formatting
    ContextualizationPattern standalone_whitespace(
        "standalone_whitespace",
        {PatternElement::MERGED_WHITESPACE},
        {
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {0}, "formatting whitespace")
        },
        10  // Low priority - only matches if nothing else does
    );
    contextualizer.register_pattern(standalone_whitespace);
    
    LOG_DEBUG("Whitespace patterns registered");
}

// Advanced patterns for later phases
void setup_advanced_patterns(InstructionContextualizer& contextualizer) {
    auto logger = cprime::LoggerFactory::get_logger("pattern_definitions");
    LOG_INFO("Setting up advanced contextualization patterns");
    
    // Future: Template instantiation patterns
    // Future: Auto type deduction patterns
    // Future: Lambda expression patterns
    // Future: Namespace resolution patterns
    
    LOG_INFO("Advanced pattern setup complete");
}

} // namespace cprime::layer2_contextualization