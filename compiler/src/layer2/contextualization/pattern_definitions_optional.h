#pragma once

#include "pattern_core_structures.h"
#include "reusable_pattern_registry.h"

namespace cprime::layer2_contextualization {

// Optional/Reusable Pattern Definitions
// These patterns don't have END_OF_PATTERN requirements and can be shared across all contexts
class OptionalPatternDefinitions {
public:
    // Initialize all optional/reusable patterns into the registry
    static void initialize_builtin_optional_patterns(ReusablePatternRegistry& registry);
    
private:
    // Individual pattern creation methods
    static void create_optional_assignment_pattern(ReusablePatternRegistry& registry);
    static void create_optional_type_modifier_pattern(ReusablePatternRegistry& registry);
    static void create_optional_access_modifier_pattern(ReusablePatternRegistry& registry);
    static void create_optional_whitespace_pattern(ReusablePatternRegistry& registry);
    
    static void create_repeatable_namespace_pattern(ReusablePatternRegistry& registry);
    static void create_repeatable_parameter_list_pattern(ReusablePatternRegistry& registry);
    static void create_repeatable_template_args_pattern(ReusablePatternRegistry& registry);
    
    // Expression pattern creation methods
    static void create_base_expression_pattern(ReusablePatternRegistry& registry);
    static void create_mandatory_expression_pattern(ReusablePatternRegistry& registry);
    static void create_optional_parenthesized_pattern(ReusablePatternRegistry& registry);
    static void create_optional_binary_operator_pattern(ReusablePatternRegistry& registry);
    static void create_optional_unary_operator_pattern(ReusablePatternRegistry& registry);
    
    // Function-specific pattern creation methods
    static void create_mandatory_assignment_default_pattern(ReusablePatternRegistry& registry);
    static void create_optional_parameter_list_pattern(ReusablePatternRegistry& registry);
};

} // namespace cprime::layer2_contextualization