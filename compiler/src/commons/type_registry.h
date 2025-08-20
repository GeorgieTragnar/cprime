#pragma once

#include "type_descriptors.h"
#include "dirty/string_table.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cprime {

/**
 * Namespace-aware type registry with no shadowing support
 * 
 * Design:
 * - Types are stored per namespace (namespace -> identifier -> TypeDescriptor)
 * - No shadowing allowed - prevents identifier conflicts in namespace hierarchy
 * - Resolution follows namespace chain from current to global
 * - Works purely with StringIndex for efficiency
 */
class TypeRegistry {
private:
    StringTable& string_table_;
    
    // Core storage: Namespace -> Identifier -> TypeDescriptor
    std::unordered_map<StringIndex, std::unordered_map<StringIndex, TypeDescriptor>> namespace_types_;
    
    // Instantiation tracking: Namespace -> Set of instantiated identifiers
    std::unordered_map<StringIndex, std::unordered_set<StringIndex>> namespace_instantiations_;
    
    // Namespace hierarchy: namespace -> parent_namespace (StringIndex{UINT32_MAX} = global)
    std::unordered_map<StringIndex, StringIndex> namespace_parents_;
    
    // Cached global namespace index
    StringIndex global_namespace_;
    
public:
    explicit TypeRegistry(StringTable& string_table);
    
    // Namespace management
    void register_namespace(StringIndex namespace_name, StringIndex parent_namespace);
    bool is_namespace_registered(StringIndex namespace_name) const;
    std::vector<StringIndex> build_namespace_chain(StringIndex namespace_name) const;
    
    // Type declaration registration
    void register_type_declaration(StringIndex namespace_name,
                                  StringIndex identifier,
                                  const TypeDescriptor& descriptor);
    
    // Type instantiation tracking
    void mark_type_instantiated(StringIndex identifier,
                               const std::vector<StringIndex>& namespace_path);
    
    // Resolution following namespace chain (no shadowing)
    TypeDescriptor* resolve_type(StringIndex identifier,
                                const std::vector<StringIndex>& namespace_path);
    
    // Direct lookup in specific namespace
    TypeDescriptor* find_type_in_namespace(StringIndex namespace_name, StringIndex identifier);
    
    // Type queries
    bool is_type_declared(StringIndex namespace_name, StringIndex identifier) const;
    bool is_type_instantiated(StringIndex namespace_name, StringIndex identifier) const;
    
    // Get all types in a namespace
    std::vector<TypeDescriptor*> get_types_in_namespace(StringIndex namespace_name);
    
    // Get all instantiated types across all namespaces
    std::vector<TypeDescriptor*> get_all_instantiated_types();
    
    // Statistics and debugging
    size_t get_total_registered_types() const;
    size_t get_total_instantiated_types() const;
    void debug_print_namespace_hierarchy() const;
    
private:
    // Shadowing prevention
    void check_for_shadowing_violation(StringIndex namespace_name, StringIndex identifier) const;
    
    // Helper to check if identifier exists in specific namespace
    bool is_identifier_declared_in_namespace(StringIndex namespace_name, StringIndex identifier) const;
    
    // Get all descendant namespaces (for shadowing check)
    std::vector<StringIndex> get_descendant_namespaces(StringIndex namespace_name) const;
    void collect_descendant_namespaces(StringIndex namespace_name, 
                                      std::vector<StringIndex>& descendants) const;
    
    // Build qualified name from namespace and identifier
    StringIndex build_qualified_name(StringIndex namespace_name, StringIndex identifier);
};

} // namespace cprime