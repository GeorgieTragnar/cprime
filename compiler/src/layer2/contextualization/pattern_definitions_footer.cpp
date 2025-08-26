#include "pattern_definitions_footer.h"
#include "contextualization_pattern_matcher.h"
#include "../../commons/logger.h"

namespace cprime::layer2_contextualization {

// Initialize all footer patterns into the matcher
void FooterPatternDefinitions::initialize_builtin_footer_patterns(ContextualizationPatternMatcher& matcher) {
    auto logger = cprime::LoggerFactory::get_logger("footer_pattern_definitions");
    LOG_INFO("🏗️ Initializing builtin footer patterns");
    
    // Create footer patterns (only basic ones)
    create_closing_brace_pattern(matcher);
    create_return_statement_pattern(matcher);
    // TODO: Add specialized patterns when needed:
    // create_end_function_pattern(matcher);
    // create_end_class_pattern(matcher);
    // NOTE: Patterns with comments are not needed - comments are filtered during preprocessing
    
    LOG_INFO("✅ Builtin footer patterns initialized: {} patterns registered", 
             matcher.get_footer_pattern_count());
}

// Footer Pattern 1: Closing Brace
// Pattern: [OPTIONAL_WHITESPACE] } [OPTIONAL_WHITESPACE]
void FooterPatternDefinitions::create_closing_brace_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::RIGHT_BRACE, EContextualToken::SCOPE_REFERENCE),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern closing_brace_pattern("closing_brace", elements);
    matcher.register_footer_pattern(closing_brace_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("footer_pattern_definitions");
    LOG_DEBUG("Registered footer pattern: closing_brace");
}

// Footer Pattern 2: End Namespace
// Pattern: [OPTIONAL_WHITESPACE] } [OPTIONAL_WHITESPACE] // end namespace [identifier] [OPTIONAL_WHITESPACE]
void FooterPatternDefinitions::create_end_namespace_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::RIGHT_BRACE, EContextualToken::SCOPE_REFERENCE),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Optional comment indicating end of namespace
        PatternElement(EToken::COMMENT, EContextualToken::SCOPE_REFERENCE),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern end_namespace_pattern("end_namespace", elements);
    matcher.register_footer_pattern(end_namespace_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("footer_pattern_definitions");
    LOG_DEBUG("Registered footer pattern: end_namespace");
}

// Footer Pattern 3: End Function  
// Pattern: [OPTIONAL_WHITESPACE] } [OPTIONAL_WHITESPACE]
void FooterPatternDefinitions::create_end_function_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::RIGHT_BRACE, EContextualToken::FUNCTION_CALL),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern end_function_pattern("end_function", elements);
    matcher.register_footer_pattern(end_function_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("footer_pattern_definitions");
    LOG_DEBUG("Registered footer pattern: end_function");
}

// Footer Pattern 4: End Class
// Pattern: [OPTIONAL_WHITESPACE] } ; [OPTIONAL_WHITESPACE] 
void FooterPatternDefinitions::create_end_class_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::RIGHT_BRACE, EContextualToken::TYPE_REFERENCE),
        PatternElement(EToken::SEMICOLON, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern end_class_pattern("end_class", elements);
    matcher.register_footer_pattern(end_class_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("footer_pattern_definitions");
    LOG_DEBUG("Registered footer pattern: end_class");
}

// Footer Pattern 5: Return Statement
// Pattern: [OPTIONAL_WHITESPACE] RETURN [REQUIRED_WHITESPACE EXPRESSION] [OPTIONAL_WHITESPACE] ; [OPTIONAL_WHITESPACE]
void FooterPatternDefinitions::create_return_statement_pattern(ContextualizationPatternMatcher& matcher) {
    std::vector<PatternElement> elements = {
        // Optional leading whitespace
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::RETURN, EContextualToken::FUNCTION_CALL),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        // Optional return value (could be identifier, literal, or complex expression)
        PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::EXPRESSION),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(EToken::SEMICOLON, EContextualToken::OPERATOR),
        PatternElement(PatternElementType::OPTIONAL_WHITESPACE, EContextualToken::INVALID),
        PatternElement(PatternElementType::END_OF_PATTERN, EContextualToken::INVALID)
    };
    
    Pattern return_statement_pattern("return_statement", elements);
    matcher.register_footer_pattern(return_statement_pattern);
    
    auto logger = cprime::LoggerFactory::get_logger("footer_pattern_definitions");
    LOG_DEBUG("Registered footer pattern: return_statement");
}

} // namespace cprime::layer2_contextualization