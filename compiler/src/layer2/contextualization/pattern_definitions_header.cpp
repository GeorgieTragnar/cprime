#include "pattern_definitions_header.h"
#include "contextualization_pattern_matcher.h"
#include "../../commons/logger.h"

namespace cprime::layer2_contextualization {

// Initialize all header patterns into the matcher
void HeaderPatternDefinitions::initialize_builtin_header_patterns(ContextualizationPatternMatcher& matcher) {
    auto logger = cprime::LoggerFactory::get_logger("header_pattern_definitions");
    LOG_INFO("🏗️ Initializing builtin header patterns");
    
    // Create header patterns (only basic ones that have available tokens)
    create_class_definition_pattern(matcher);
    create_function_declaration_pattern(matcher);
    create_function_definition_with_default_pattern(matcher);
    // TODO: Add remaining patterns when tokens are available:
    // create_namespace_declaration_pattern(matcher);
    // create_import_statement_pattern(matcher);
    // create_typedef_pattern(matcher);
    // create_enum_declaration_pattern(matcher);
    
    LOG_INFO("✅ Builtin header patterns initialized: {} patterns registered", 
             matcher.get_header_pattern_count());
}

// Header Pattern 1: Class/Struct/Plex Definition
// Pattern: [OPTIONAL_WHITESPACE] CLASS|STRUCT|PLEX REQUIRED_WHITESPACE NAMESPACED_IDENTIFIER [OPTIONAL_WHITESPACE]
void HeaderPatternDefinitions::create_class_definition_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement({EToken::CLASS, EToken::STRUCT, EToken::PLEX}, EContextualToken::TYPE_REFERENCE),
        PatternElement(PatternElementType::REQUIRED_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::TYPE_REFERENCE),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern class_definition_pattern("class_definition", elements);
    matcher.register_header_pattern(class_definition_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("header_pattern_definitions");
    LOG_DEBUG("Registered header pattern: class_definition");
}

// Header Pattern 2: Function Declaration
// Pattern: [OPTIONAL_WHITESPACE] FUNC REQUIRED_WHITESPACE NAMESPACED_IDENTIFIER [OPTIONAL_WHITESPACE]
void HeaderPatternDefinitions::create_function_declaration_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::FUNC, EContextualToken::FUNCTION_CALL),
        PatternElement(PatternElementType::REQUIRED_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::FUNCTION_CALL),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern function_declaration_pattern("function_declaration", elements);
    matcher.register_header_pattern(function_declaration_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("header_pattern_definitions");
    LOG_DEBUG("Registered header pattern: function_declaration");
}

// Header Pattern 3: Main Function Definition with Default (Entry Point Pattern)
// Pattern: [OPTIONAL_WHITESPACE] int main ( int argc , char * argv[] ) = default [OPTIONAL_WHITESPACE]
// Example: "int main(int argc, char *argv[]) = default"
void HeaderPatternDefinitions::create_function_definition_with_default_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Return type MUST be int only for main function
        PatternElement(EToken::INT32_T, EContextualToken::TYPE_REFERENCE),
        PatternElement(PatternElementType::REQUIRED_WHITESPACE, EContextualToken::INVALID),
        // Function name MUST be "main"
        PatternElement(EToken::IDENTIFIER, EContextualToken::FUNCTION_CALL), // "main" identifier
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Opening parenthesis for parameters
        PatternElement(EToken::LEFT_PAREN, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // First parameter: int argc
        PatternElement(EToken::INT32_T, EContextualToken::TYPE_REFERENCE),
        PatternElement(PatternElementType::REQUIRED_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::IDENTIFIER, EContextualToken::VARIABLE_DECLARATION), // "argc" identifier
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Comma separator
        PatternElement(EToken::COMMA, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Second parameter: char *argv[]
        PatternElement(EToken::CHAR, EContextualToken::TYPE_REFERENCE),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::MULTIPLY, EContextualToken::OPERATOR), // pointer asterisk
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::IDENTIFIER, EContextualToken::VARIABLE_DECLARATION), // "argv" identifier
        PatternElement(EToken::LEFT_BRACKET, EContextualToken::OPERATOR), // [
        PatternElement(EToken::RIGHT_BRACKET, EContextualToken::OPERATOR), // ]
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Closing parenthesis
        PatternElement(EToken::RIGHT_PAREN, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Mandatory assignment default pattern
        PatternElement(PatternKey::MANDATORY_ASSIGNMENT_DEFAULT, EContextualToken::INVALID),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern function_definition_default_pattern("main_function_definition_with_default", elements);
    matcher.register_header_pattern(function_definition_default_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("header_pattern_definitions");
    LOG_DEBUG("Registered header pattern: main_function_definition_with_default");
}

// TODO: Additional header patterns can be added here when the necessary tokens are available:
// - Namespace declarations (needs NAMESPACE token)
// - Import statements (needs IMPORT token) 
// - Typedef patterns (needs TYPEDEF token)
// - Enum declarations (needs ENUM token)

} // namespace cprime::layer2_contextualization