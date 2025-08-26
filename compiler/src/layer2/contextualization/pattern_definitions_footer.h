#pragma once

#include "pattern_core_structures.h"

namespace cprime::layer2_contextualization {

// Forward declaration
class ContextualizationPatternMatcher;

// Footer Pattern Definitions
// These patterns are used to identify and contextualize footer instructions (closing braces, end statements, etc.)
class FooterPatternDefinitions {
public:
    // Initialize all footer patterns into the matcher
    static void initialize_builtin_footer_patterns(ContextualizationPatternMatcher& matcher);
    
private:
    // Individual pattern creation methods
    static void create_closing_brace_pattern(ContextualizationPatternMatcher& matcher);
    static void create_end_namespace_pattern(ContextualizationPatternMatcher& matcher);
    static void create_end_function_pattern(ContextualizationPatternMatcher& matcher);
    static void create_end_class_pattern(ContextualizationPatternMatcher& matcher);
    static void create_return_statement_pattern(ContextualizationPatternMatcher& matcher);
};

} // namespace cprime::layer2_contextualization