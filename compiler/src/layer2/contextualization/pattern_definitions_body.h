#pragma once

#include "pattern_core_structures.h"

namespace cprime::layer2_contextualization {

// Forward declaration
class ContextualizationPatternMatcher;

// Body/Instruction Pattern Definitions
// These patterns are used to identify and contextualize body instructions (variable declarations, assignments, function calls, etc.)
class BodyPatternDefinitions {
public:
    // Initialize all body patterns into the matcher
    static void initialize_builtin_body_patterns(ContextualizationPatternMatcher& matcher);
    
private:
    // Variable and assignment patterns
    static void create_variable_declaration_with_assignment_pattern(ContextualizationPatternMatcher& matcher);
    static void create_variable_declaration_without_assignment_pattern(ContextualizationPatternMatcher& matcher);
    static void create_complex_variable_declaration_pattern(ContextualizationPatternMatcher& matcher);
    static void create_assignment_statement_pattern(ContextualizationPatternMatcher& matcher);
    
    // Function and expression patterns
    static void create_function_call_pattern(ContextualizationPatternMatcher& matcher);
    static void create_expression_statement_pattern(ContextualizationPatternMatcher& matcher);
    
    // Control flow patterns
    static void create_if_statement_pattern(ContextualizationPatternMatcher& matcher);
    static void create_while_loop_pattern(ContextualizationPatternMatcher& matcher);
    static void create_for_loop_pattern(ContextualizationPatternMatcher& matcher);
    
    // Other patterns
    // NOTE: Comment patterns are not needed - comments are filtered during preprocessing
};

} // namespace cprime::layer2_contextualization