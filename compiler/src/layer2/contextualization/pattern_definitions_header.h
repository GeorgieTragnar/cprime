#pragma once

#include "pattern_core_structures.h"

namespace cprime::layer2_contextualization {

// Forward declaration
class ContextualizationPatternMatcher;

// Header Pattern Definitions
// These patterns are used to identify and contextualize header instructions (class/function declarations, etc.)
class HeaderPatternDefinitions {
public:
    // Initialize all header patterns into the matcher
    static void initialize_builtin_header_patterns(ContextualizationPatternMatcher& matcher);
    
private:
    // Individual pattern creation methods
    static void create_class_definition_pattern(ContextualizationPatternMatcher& matcher);
    static void create_function_declaration_pattern(ContextualizationPatternMatcher& matcher);
    static void create_function_definition_with_default_pattern(ContextualizationPatternMatcher& matcher);
    static void create_namespace_declaration_pattern(ContextualizationPatternMatcher& matcher);
    static void create_import_statement_pattern(ContextualizationPatternMatcher& matcher);
    static void create_typedef_pattern(ContextualizationPatternMatcher& matcher);
    static void create_enum_declaration_pattern(ContextualizationPatternMatcher& matcher);
};

} // namespace cprime::layer2_contextualization