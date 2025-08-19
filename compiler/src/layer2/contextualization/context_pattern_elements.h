#pragma once

#include "base_contextualizer.h"

namespace cprime::layer2_contextualization {

// Header-specific pattern elements (range 1000-1999)
enum class HeaderPatternElement : uint32_t {
    // Inherit all base elements by value casting
    
    // Function declaration patterns (1000-1099)
    KEYWORD_FUNC = 1000,
    FUNCTION_PARAMETERS = 1001,     // (type param, type param)
    RETURN_TYPE_ARROW = 1002,       // ->
    RETURN_TYPE = 1003,             // return type specification
    FUNCTION_SIGNATURE = 1004,      // Complete function signature
    
    // Type definition patterns (1100-1199)
    KEYWORD_CLASS = 1100,
    KEYWORD_STRUCT = 1101,
    KEYWORD_INTERFACE = 1102,
    KEYWORD_ENUM = 1103,
    KEYWORD_TYPEDEF = 1104,
    TYPE_BODY = 1105,               // { class/struct body }
    
    // Template patterns (1200-1299)
    KEYWORD_TEMPLATE = 1200,
    TEMPLATE_PARAMETERS = 1201,     // <T, U, V>
    TEMPLATE_CONSTRAINTS = 1202,    // where T: Comparable
    TEMPLATE_SPECIALIZATION = 1203, // <int, string>
    
    // Namespace patterns (1300-1399)
    KEYWORD_NAMESPACE = 1300,
    NAMESPACE_PATH = 1301,          // math::geometry::point
    NAMESPACE_ALIAS = 1302,         // using alias = long::namespace::path
    
    // Visibility/Access patterns (1400-1499)
    KEYWORD_PUBLIC = 1400,
    KEYWORD_PRIVATE = 1401,
    KEYWORD_PROTECTED = 1402,
    KEYWORD_INTERNAL = 1403,
    
    // Inheritance patterns (1500-1599)
    KEYWORD_EXTENDS = 1500,
    KEYWORD_IMPLEMENTS = 1501,
    INHERITANCE_LIST = 1502,        // : public Base1, private Base2
    
    // Import/Export patterns (1600-1699)
    KEYWORD_IMPORT = 1600,
    KEYWORD_EXPORT = 1601,
    KEYWORD_FROM = 1602,
    MODULE_PATH = 1603,             // "module/path" or module.path
};

// Footer-specific pattern elements (range 2000-2999)
enum class FooterPatternElement : uint32_t {
    // Inherit all base elements by value casting
    
    // Return patterns (2000-2099)
    KEYWORD_RETURN = 2000,
    RETURN_EXPRESSION = 2001,
    RETURN_VOID = 2002,             // return without value
    
    // Control flow patterns (2100-2199)
    KEYWORD_BREAK = 2100,
    KEYWORD_CONTINUE = 2101,
    KEYWORD_GOTO = 2102,
    LABEL_REFERENCE = 2103,         // label name for goto
    
    // Exception patterns (2200-2299)
    KEYWORD_THROW = 2200,
    KEYWORD_RETHROW = 2201,
    EXCEPTION_EXPRESSION = 2202,
    
    // Cleanup patterns (2300-2399)
    KEYWORD_DEFER = 2300,
    CLEANUP_STATEMENT = 2301,
    RESOURCE_RELEASE = 2302,
    DESTRUCTOR_CALL = 2303,
    
    // Scope finalization (2400-2499)
    SCOPE_CLEANUP = 2400,
    SCOPE_VALIDATION = 2401,
    SCOPE_SUMMARY = 2402,
};

// Enhanced instruction-specific pattern elements (range 3000-4999)
enum class InstructionPatternElement : uint32_t {
    // Inherit all base elements by value casting
    
    // Basic keywords (already in instruction_contextualizer.h but redefined here for consistency)
    KEYWORD_INT = 3000,
    KEYWORD_STRING = 3001,
    KEYWORD_BOOL = 3002,
    KEYWORD_FLOAT = 3003,
    KEYWORD_DOUBLE = 3004,
    KEYWORD_VOID = 3005,
    KEYWORD_AUTO = 3006,
    KEYWORD_FUNC = 3007,
    KEYWORD_IF = 3008,
    KEYWORD_WHILE = 3009,
    KEYWORD_FOR = 3010,
    KEYWORD_RETURN = 3011,
    KEYWORD_EXEC = 3012,
    
    // Variable declaration patterns (3100-3199)
    VARIABLE_DECLARATION = 3100,
    VARIABLE_INITIALIZATION = 3101,
    CONST_DECLARATION = 3102,
    MUTABLE_DECLARATION = 3103,
    
    // Assignment patterns (3200-3299)
    SIMPLE_ASSIGNMENT = 3200,       // =
    COMPOUND_ASSIGNMENT = 3201,     // +=, -=, *=, /=
    DESTRUCTURING_ASSIGNMENT = 3202, // [a, b] = tuple
    
    // Function call patterns (3300-3399)
    FUNCTION_CALL = 3300,
    METHOD_CALL = 3301,             // object.method()
    CONSTRUCTOR_CALL = 3302,        // MyClass(args)
    STATIC_CALL = 3303,             // Class::method()
    
    // Object creation patterns (3400-3499)
    NEW_EXPRESSION = 3400,          // new MyClass()
    STACK_ALLOCATION = 3401,        // MyClass obj;
    TEMPLATE_INSTANTIATION = 3402,  // MyClass<int, string>
    ARRAY_CREATION = 3403,          // new int[size]
    
    // Member access patterns (3500-3599)
    DOT_ACCESS = 3500,              // object.member
    ARROW_ACCESS = 3501,            // pointer->member
    SUBSCRIPT_ACCESS = 3502,        // array[index]
    POINTER_DEREFERENCE = 3503,     // *pointer
    ADDRESS_OF = 3504,              // &variable
    
    // Lambda patterns (3600-3699)
    LAMBDA_EXPRESSION = 3600,       // (args) -> return_type { body }
    LAMBDA_CAPTURE = 3601,          // [capture_list]
    LAMBDA_PARAMETERS = 3602,       // (int x, string s)
    LAMBDA_BODY = 3603,             // { lambda body }
    LAMBDA_RETURN_TYPE = 3604,      // -> return_type
    
    // Control flow patterns (3700-3799)
    IF_STATEMENT = 3700,
    ELSE_STATEMENT = 3701,
    WHILE_LOOP = 3702,
    FOR_LOOP = 3703,
    SWITCH_STATEMENT = 3704,
    CASE_LABEL = 3705,
    DEFAULT_LABEL = 3706,
    
    // Expression patterns (3800-3899)
    BINARY_EXPRESSION = 3800,       // a + b, a && b, etc.
    UNARY_EXPRESSION = 3801,        // !a, -a, ++a, etc.
    TERNARY_EXPRESSION = 3802,      // condition ? true_val : false_val
    PARENTHESIZED_EXPRESSION = 3803, // (expression)
    
    // Type patterns (3900-3999)
    TYPE_CAST = 3900,               // (Type)expression
    TYPE_CHECK = 3901,              // expression is Type
    SIZEOF_EXPRESSION = 3902,       // sizeof(Type)
    TYPEOF_EXPRESSION = 3903,       // typeof(expression)
    
    // Advanced patterns (4000-4099)
    GENERIC_CONSTRAINT = 4000,      // where T: Constraint
    ASYNC_EXPRESSION = 4001,        // async { ... }
    AWAIT_EXPRESSION = 4002,        // await expression
    YIELD_EXPRESSION = 4003,        // yield value
};

// Utility functions to check if pattern elements belong to base patterns
template<typename PatternElementType>
bool is_base_pattern_element(PatternElementType element) {
    uint32_t value = static_cast<uint32_t>(element);
    return value < 1000; // Base patterns are in range 0-999
}

template<typename PatternElementType>
BasePatternElement to_base_pattern_element(PatternElementType element) {
    uint32_t value = static_cast<uint32_t>(element);
    if (value < 1000) {
        return static_cast<BasePatternElement>(value);
    }
    // Invalid conversion - should not happen if is_base_pattern_element was checked first
    return static_cast<BasePatternElement>(0);
}

// Type aliases for the specific contextualizer pattern types
using HeaderContextualizationPattern = BaseContextualizationPattern<HeaderPatternElement>;
using FooterContextualizationPattern = BaseContextualizationPattern<FooterPatternElement>;
using InstructionContextualizationPattern = BaseContextualizationPattern<InstructionPatternElement>;

} // namespace cprime::layer2_contextualization