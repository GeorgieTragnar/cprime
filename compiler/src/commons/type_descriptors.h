#pragma once

#include "dirty/string_table.h"
#include <vector>

namespace cprime {

// Forward declarations for circular dependency resolution
class TypeRegistry;
class FunctionRegistry;

/**
 * Type classification for CPrime's three-class system
 */
enum class TypeKind : uint32_t {
    PRIMITIVE = 0,     // Built-in types: int, float, bool, string
    DATA_CLASS = 1,    // Data classes (pure state)
    FUNCTIONAL_CLASS = 2,  // Functional classes (stateless operations)
    DANGER_CLASS = 3,  // Danger classes (traditional OOP/C++ interop)
    INTERFACE = 4,     // Interface definitions
    FUNCTION_TYPE = 5  // Function type signatures
};

/**
 * Primitive type classification
 */
enum class PrimitiveKind : uint32_t {
    // Integer types
    INT8 = 0, INT16 = 1, INT32 = 2, INT64 = 3,
    UINT8 = 4, UINT16 = 5, UINT32 = 6, UINT64 = 7,
    
    // Floating point types
    FLOAT = 8, DOUBLE = 9,
    
    // Other primitives
    BOOL = 10, CHAR = 11, STRING = 12, VOID = 13
};

/**
 * Basic type descriptor - stores type information
 */
struct TypeDescriptor {
    StringIndex qualified_name;     // Fully qualified name (namespace::Type)
    StringIndex namespace_name;     // Namespace this type belongs to
    StringIndex simple_name;        // Just the type name without namespace
    TypeKind kind;
    bool is_declared = false;
    bool is_instantiated = false;
    
    // For primitive types
    PrimitiveKind primitive_kind;
    
    // For class types - basic info only (composition registry handles detailed structure)
    std::vector<StringIndex> field_names;        // Field names discovered during usage
    std::vector<StringIndex> method_names;       // Method names discovered during usage
    std::vector<StringIndex> interface_impls;    // Interfaces this type implements
    
    TypeDescriptor() = default;
    
    TypeDescriptor(StringIndex qualified_name, StringIndex namespace_name, 
                  StringIndex simple_name, TypeKind kind)
        : qualified_name(qualified_name), namespace_name(namespace_name),
          simple_name(simple_name), kind(kind), primitive_kind(PrimitiveKind::VOID) {}
    
    // Constructor for primitive types
    TypeDescriptor(StringIndex qualified_name, StringIndex namespace_name,
                  StringIndex simple_name, PrimitiveKind prim_kind)
        : qualified_name(qualified_name), namespace_name(namespace_name),
          simple_name(simple_name), kind(TypeKind::PRIMITIVE), primitive_kind(prim_kind) {}
};

/**
 * Function parameter descriptor
 */
struct ParameterDescriptor {
    StringIndex parameter_name;
    StringIndex parameter_type;  // Qualified type name
    bool has_default_value = false;
    
    ParameterDescriptor() = default;
    ParameterDescriptor(StringIndex name, StringIndex type)
        : parameter_name(name), parameter_type(type) {}
};

/**
 * Function descriptor - stores function signature information
 */
struct FunctionDescriptor {
    StringIndex qualified_name;     // Fully qualified name (namespace::function)
    StringIndex namespace_name;     // Namespace this function belongs to
    StringIndex simple_name;        // Just the function name without namespace
    std::vector<ParameterDescriptor> parameters;
    StringIndex return_type;        // Qualified return type name
    bool is_declared = false;
    bool is_called = false;
    
    // For methods - which type owns this method
    StringIndex owner_type;         // Empty if not a method
    bool is_method = false;
    
    FunctionDescriptor() = default;
    
    FunctionDescriptor(StringIndex qualified_name, StringIndex namespace_name,
                      StringIndex simple_name, StringIndex return_type)
        : qualified_name(qualified_name), namespace_name(namespace_name),
          simple_name(simple_name), return_type(return_type) {}
};

/**
 * Exception types for shadowing violations
 */
class ShadowingError : public std::exception {
private:
    std::string message_;
    
public:
    ShadowingError(StringIndex identifier, StringIndex new_namespace, StringIndex existing_namespace)
        : message_("Shadowing error: identifier already exists in parent/child namespace") {}
    
    const char* what() const noexcept override {
        return message_.c_str();
    }
};

} // namespace cprime