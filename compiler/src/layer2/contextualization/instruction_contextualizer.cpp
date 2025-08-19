#include "instruction_contextualizer.h"
#include "../../commons/logger.h"
#include "../../commons/enum/token.h"

namespace cprime::layer2_contextualization {

InstructionContextualizer::InstructionContextualizer() {
    setup_instruction_patterns();
}

bool InstructionContextualizer::token_matches_element(const Token& token, InstructionPatternElement element) {
    // First check if it's a base pattern element
    if (is_base_pattern_element(element)) {
        BasePatternElement base_element = to_base_pattern_element(element);
        return token_matches_base_element(token, base_element);
    }
    
    // Handle instruction-specific pattern elements
    switch (element) {
        // Specific keywords
        case InstructionPatternElement::KEYWORD_INT:
            return token._token == EToken::INT32_T || token._token == EToken::INT8_T ||
                   token._token == EToken::INT16_T || token._token == EToken::INT64_T;
        case InstructionPatternElement::KEYWORD_FUNC:
            return token._token == EToken::FUNC || token._token == EToken::FUNCTION;
        case InstructionPatternElement::KEYWORD_AUTO:
            return token._token == EToken::AUTO;
        case InstructionPatternElement::KEYWORD_IF:
            return token._token == EToken::IF;
        case InstructionPatternElement::KEYWORD_WHILE:
            return token._token == EToken::WHILE;
        case InstructionPatternElement::KEYWORD_FOR:
            return token._token == EToken::FOR;
        case InstructionPatternElement::KEYWORD_RETURN:
            return token._token == EToken::RETURN;
        case InstructionPatternElement::KEYWORD_EXEC:
            return token._token == EToken::EXEC;
            
        // Complex pattern elements (not implemented yet)
        case InstructionPatternElement::VARIABLE_DECLARATION:
        case InstructionPatternElement::VARIABLE_INITIALIZATION:
        case InstructionPatternElement::CONST_DECLARATION:
        case InstructionPatternElement::MUTABLE_DECLARATION:
        case InstructionPatternElement::SIMPLE_ASSIGNMENT:
        case InstructionPatternElement::COMPOUND_ASSIGNMENT:
        case InstructionPatternElement::DESTRUCTURING_ASSIGNMENT:
        case InstructionPatternElement::FUNCTION_CALL:
        case InstructionPatternElement::METHOD_CALL:
        case InstructionPatternElement::CONSTRUCTOR_CALL:
        case InstructionPatternElement::STATIC_CALL:
        case InstructionPatternElement::NEW_EXPRESSION:
        case InstructionPatternElement::STACK_ALLOCATION:
        case InstructionPatternElement::TEMPLATE_INSTANTIATION:
        case InstructionPatternElement::ARRAY_CREATION:
        case InstructionPatternElement::DOT_ACCESS:
        case InstructionPatternElement::ARROW_ACCESS:
        case InstructionPatternElement::SUBSCRIPT_ACCESS:
        case InstructionPatternElement::POINTER_DEREFERENCE:
        case InstructionPatternElement::ADDRESS_OF:
        case InstructionPatternElement::LAMBDA_EXPRESSION:
        case InstructionPatternElement::LAMBDA_CAPTURE:
        case InstructionPatternElement::LAMBDA_PARAMETERS:
        case InstructionPatternElement::LAMBDA_BODY:
        case InstructionPatternElement::LAMBDA_RETURN_TYPE:
        case InstructionPatternElement::IF_STATEMENT:
        case InstructionPatternElement::ELSE_STATEMENT:
        case InstructionPatternElement::WHILE_LOOP:
        case InstructionPatternElement::FOR_LOOP:
        case InstructionPatternElement::SWITCH_STATEMENT:
        case InstructionPatternElement::CASE_LABEL:
        case InstructionPatternElement::DEFAULT_LABEL:
        case InstructionPatternElement::BINARY_EXPRESSION:
        case InstructionPatternElement::UNARY_EXPRESSION:
        case InstructionPatternElement::TERNARY_EXPRESSION:
        case InstructionPatternElement::PARENTHESIZED_EXPRESSION:
        case InstructionPatternElement::TYPE_CAST:
        case InstructionPatternElement::TYPE_CHECK:
        case InstructionPatternElement::SIZEOF_EXPRESSION:
        case InstructionPatternElement::TYPEOF_EXPRESSION:
        case InstructionPatternElement::GENERIC_CONSTRAINT:
        case InstructionPatternElement::ASYNC_EXPRESSION:
        case InstructionPatternElement::AWAIT_EXPRESSION:
        case InstructionPatternElement::YIELD_EXPRESSION:
            return false; // Handled by specialized N:M matching logic or not implemented yet
            
        default:
            return false;
    }
}

bool InstructionContextualizer::is_whitespace_pattern_element(InstructionPatternElement element) {
    // Check base whitespace patterns first
    if (is_base_pattern_element(element)) {
        uint32_t element_value = static_cast<uint32_t>(element);
        
        return element_value == static_cast<uint32_t>(BasePatternElement::OPTIONAL_WHITESPACE) ||
               element_value == static_cast<uint32_t>(BasePatternElement::REQUIRED_WHITESPACE) ||
               element_value == static_cast<uint32_t>(BasePatternElement::SINGLE_WHITESPACE) ||
               element_value == static_cast<uint32_t>(BasePatternElement::MERGED_WHITESPACE);
    }
    
    // Instruction-specific whitespace patterns (none for now)
    return false;
}

bool InstructionContextualizer::token_matches_base_element(const Token& token, BasePatternElement element) {
    switch (element) {
        // Generic token types
        case BasePatternElement::ANY_IDENTIFIER:
            return token._token == EToken::IDENTIFIER;
        case BasePatternElement::ANY_LITERAL:
            return token._token == EToken::STRING_LITERAL || 
                   token._token == EToken::INT_LITERAL ||
                   token._token == EToken::FLOAT_LITERAL;
        case BasePatternElement::ANY_STRING_LITERAL:
            return token._token == EToken::STRING_LITERAL;
        case BasePatternElement::ANY_INT_LITERAL:
            return token._token == EToken::INT_LITERAL;
            
        // Specific operators and punctuation
        case BasePatternElement::LITERAL_ASSIGN:
            return token._token == EToken::ASSIGN;
        case BasePatternElement::LITERAL_PLUS:
            return token._token == EToken::PLUS;
        case BasePatternElement::LITERAL_MINUS:
            return token._token == EToken::MINUS;
        case BasePatternElement::LITERAL_MULTIPLY:
            return token._token == EToken::MULTIPLY;
        case BasePatternElement::LITERAL_DIVIDE:
            return token._token == EToken::DIVIDE;
        case BasePatternElement::LITERAL_SEMICOLON:
            return token._token == EToken::SEMICOLON;
        case BasePatternElement::LITERAL_COLON:
            return token._token == EToken::COLON;
        case BasePatternElement::LITERAL_COMMA:
            return token._token == EToken::COMMA;
        case BasePatternElement::LITERAL_DOT:
            return token._token == EToken::DOT;
            
        // Brackets and delimiters
        case BasePatternElement::LITERAL_PAREN_L:
            return token._token == EToken::LEFT_PAREN;
        case BasePatternElement::LITERAL_PAREN_R:
            return token._token == EToken::RIGHT_PAREN;
        case BasePatternElement::LITERAL_BRACE_L:
            return token._token == EToken::LEFT_BRACE;
        case BasePatternElement::LITERAL_BRACE_R:
            return token._token == EToken::RIGHT_BRACE;
        case BasePatternElement::LITERAL_BRACKET_L:
            return token._token == EToken::LEFT_BRACKET;
        case BasePatternElement::LITERAL_BRACKET_R:
            return token._token == EToken::RIGHT_BRACKET;
        case BasePatternElement::LITERAL_LESS:
            return token._token == EToken::LESS_THAN;
        case BasePatternElement::LITERAL_GREATER:
            return token._token == EToken::GREATER_THAN;
            
        // Compound operators
        case BasePatternElement::LITERAL_DOUBLE_COLON:
            return token._token == EToken::SCOPE_RESOLUTION;
        case BasePatternElement::LITERAL_ARROW:
            return token._token == EToken::ARROW;
        case BasePatternElement::LITERAL_PLUS_ASSIGN:
        case BasePatternElement::LITERAL_MINUS_ASSIGN:
            return false; // Not implemented yet
            
        // Complex patterns
        case BasePatternElement::EXPRESSION_TOKENS:
        case BasePatternElement::TYPE_TOKEN_LIST:
        case BasePatternElement::PARAMETER_LIST:
        case BasePatternElement::ARGUMENT_LIST:
            return false; // Not implemented yet
            
        default:
            return false;
    }
}

void InstructionContextualizer::setup_instruction_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("instruction_contextualizer");
    LOG_INFO("Setting up instruction contextualization patterns");
    
    setup_basic_patterns();
    setup_declaration_patterns();
    setup_assignment_patterns();
    setup_function_call_patterns();
    setup_operator_patterns();
    setup_whitespace_patterns();
    setup_advanced_patterns();
    
    LOG_INFO("Instruction pattern setup complete - {} patterns registered", pattern_count());
}

void InstructionContextualizer::setup_basic_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("instruction_contextualizer");
    LOG_DEBUG("Setting up basic instruction patterns");
    
    // Pattern: int identifier;
    // Example: int x;
    InstructionContextualizationPattern int_declaration(
        "int_declaration",
        {InstructionPatternElement::KEYWORD_INT,
         static_cast<InstructionPatternElement>(BasePatternElement::REQUIRED_WHITESPACE),
         static_cast<InstructionPatternElement>(BasePatternElement::ANY_IDENTIFIER),
         static_cast<InstructionPatternElement>(BasePatternElement::OPTIONAL_WHITESPACE),
         static_cast<InstructionPatternElement>(BasePatternElement::LITERAL_SEMICOLON)},
        {
            ContextualTokenTemplate(EContextualToken::VARIABLE_DECLARATION, {0, 2}, "integer variable declaration"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {1}, "type name spacing"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {3}, "pre-semicolon spacing"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {4}, "statement terminator")
        },
        100
    );
    register_pattern(int_declaration);
    
    LOG_DEBUG("Basic instruction patterns registered");
}

void InstructionContextualizer::setup_declaration_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("instruction_contextualizer");
    LOG_DEBUG("Setting up declaration patterns");
    
    // TODO: Implement declaration patterns
    
    LOG_DEBUG("Declaration patterns setup (placeholder)");
}

void InstructionContextualizer::setup_assignment_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("instruction_contextualizer");
    LOG_DEBUG("Setting up assignment patterns");
    
    // TODO: Implement assignment patterns
    
    LOG_DEBUG("Assignment patterns setup (placeholder)");
}

void InstructionContextualizer::setup_function_call_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("instruction_contextualizer");
    LOG_DEBUG("Setting up function call patterns");
    
    // TODO: Implement function call patterns
    
    LOG_DEBUG("Function call patterns setup (placeholder)");
}

void InstructionContextualizer::setup_operator_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("instruction_contextualizer");
    LOG_DEBUG("Setting up operator patterns");
    
    // TODO: Implement operator patterns
    
    LOG_DEBUG("Operator patterns setup (placeholder)");
}

void InstructionContextualizer::setup_whitespace_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("instruction_contextualizer");
    LOG_DEBUG("Setting up whitespace patterns");
    
    // TODO: Implement whitespace-specific patterns
    
    LOG_DEBUG("Whitespace patterns setup (placeholder)");
}

void InstructionContextualizer::setup_advanced_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("instruction_contextualizer");
    LOG_DEBUG("Setting up advanced patterns");
    
    // TODO: Implement advanced patterns
    
    LOG_DEBUG("Advanced patterns setup (placeholder)");
}

} // namespace cprime::layer2_contextualization