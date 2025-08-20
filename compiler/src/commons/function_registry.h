#pragma once

#include "type_descriptors.h"
#include "dirty/string_table.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cprime {

/**
 * Function call site information for tracking usage
 */
struct CallSite {
    uint32_t scope_index;           // Which scope the call occurred in
    uint32_t instruction_index;     // Which instruction within the scope
    std::vector<StringIndex> actual_arg_types; // Actual argument types at call site
    
    CallSite() = default;
    CallSite(uint32_t scope_idx, uint32_t instr_idx, const std::vector<StringIndex>& arg_types)
        : scope_index(scope_idx), instruction_index(instr_idx), actual_arg_types(arg_types) {}
};

/**
 * Namespace-aware function registry with overload support
 * 
 * Design:
 * - Functions stored per namespace (namespace -> function_name -> vector<FunctionDescriptor>)
 * - Supports function overloading within the same namespace
 * - No shadowing allowed across namespace hierarchy
 * - Resolution follows namespace chain from current to global
 * - Tracks function calls for usage analysis
 */
class FunctionRegistry {
private:
    StringTable& string_table_;
    
    // Core storage: Namespace -> FunctionName -> vector<FunctionDescriptor> (for overloads)
    std::unordered_map<StringIndex, 
                      std::unordered_map<StringIndex, 
                                        std::vector<FunctionDescriptor>>> namespace_functions_;
    
    // Function call tracking: Qualified function name -> list of call sites
    std::unordered_map<StringIndex, std::vector<CallSite>> function_calls_;
    
    // Namespace hierarchy: namespace -> parent_namespace (same as TypeRegistry)
    std::unordered_map<StringIndex, StringIndex> namespace_parents_;
    
    // Cached global namespace index
    StringIndex global_namespace_;
    
public:
    explicit FunctionRegistry(StringTable& string_table);
    
    // Namespace management (should be kept in sync with TypeRegistry)
    void register_namespace(StringIndex namespace_name, StringIndex parent_namespace);
    bool is_namespace_registered(StringIndex namespace_name) const;
    std::vector<StringIndex> build_namespace_chain(StringIndex namespace_name) const;
    
    // Function declaration registration
    void register_function_declaration(StringIndex namespace_name,
                                      StringIndex function_name,
                                      const FunctionDescriptor& descriptor);
    
    // Function call tracking
    void mark_function_called(StringIndex function_name,
                             const std::vector<StringIndex>& namespace_path,
                             const CallSite& call_site);
    
    // Resolution following namespace chain (no shadowing)
    std::vector<FunctionDescriptor>* resolve_function_overloads(StringIndex function_name,
                                                               const std::vector<StringIndex>& namespace_path);
    
    // Overload resolution for specific call site
    FunctionDescriptor* resolve_best_overload(StringIndex function_name,
                                             const std::vector<StringIndex>& namespace_path,
                                             const std::vector<StringIndex>& arg_types);
    
    // Direct lookup in specific namespace
    std::vector<FunctionDescriptor>* find_function_overloads_in_namespace(StringIndex namespace_name, 
                                                                         StringIndex function_name);
    
    // Function queries
    bool is_function_declared(StringIndex namespace_name, StringIndex function_name) const;
    bool is_function_called(StringIndex qualified_function_name) const;
    
    // Get all functions in a namespace
    std::vector<FunctionDescriptor*> get_functions_in_namespace(StringIndex namespace_name);
    
    // Get all called functions across all namespaces
    std::vector<FunctionDescriptor*> get_all_called_functions();
    
    // Call site analysis
    std::vector<CallSite> get_call_sites(StringIndex qualified_function_name) const;
    
    // Statistics and debugging
    size_t get_total_registered_functions() const;
    size_t get_total_called_functions() const;
    void debug_print_function_registry() const;
    
private:
    // Shadowing prevention for function names
    void check_for_function_shadowing_violation(StringIndex namespace_name, StringIndex function_name) const;
    
    // Helper to check if function exists in specific namespace
    bool is_function_declared_in_namespace(StringIndex namespace_name, StringIndex function_name) const;
    
    // Get all descendant namespaces (for shadowing check)
    std::vector<StringIndex> get_descendant_namespaces(StringIndex namespace_name) const;
    void collect_descendant_namespaces(StringIndex namespace_name, 
                                      std::vector<StringIndex>& descendants) const;
    
    // Build qualified function name from namespace and identifier
    StringIndex build_qualified_function_name(StringIndex namespace_name, StringIndex function_name);
    
    // Overload resolution helpers
    bool types_match(const std::vector<StringIndex>& signature_types,
                    const std::vector<StringIndex>& call_types) const;
    int calculate_conversion_cost(const std::vector<StringIndex>& signature_types,
                                 const std::vector<StringIndex>& call_types) const;
};

} // namespace cprime