#include "function_registry.h"
#include "logger.h"
#include <algorithm>
#include <limits>

namespace cprime {

FunctionRegistry::FunctionRegistry(StringTable& string_table) 
    : string_table_(string_table) {
    // Initialize global namespace
    global_namespace_ = string_table_.intern("__global__");
    register_namespace(global_namespace_, StringIndex{UINT32_MAX}); // Global has no parent
}

void FunctionRegistry::register_namespace(StringIndex namespace_name, StringIndex parent_namespace) {
    auto logger = cprime::LoggerFactory::get_logger("function_registry");
    
    if (is_namespace_registered(namespace_name)) {
        LOG_DEBUG("Namespace already registered: {}", string_table_.get_string(namespace_name));
        return;
    }
    
    namespace_parents_[namespace_name] = parent_namespace;
    namespace_functions_[namespace_name] = std::unordered_map<StringIndex, std::vector<FunctionDescriptor>>{};
    
    LOG_DEBUG("Registered namespace: {} with parent: {}", 
              string_table_.get_string(namespace_name),
              parent_namespace.value == UINT32_MAX ? "__global__" : string_table_.get_string(parent_namespace));
}

bool FunctionRegistry::is_namespace_registered(StringIndex namespace_name) const {
    return namespace_parents_.find(namespace_name) != namespace_parents_.end();
}

std::vector<StringIndex> FunctionRegistry::build_namespace_chain(StringIndex namespace_name) const {
    std::vector<StringIndex> chain;
    
    StringIndex current = namespace_name;
    while (current.value != UINT32_MAX) {
        chain.push_back(current);
        
        auto parent_it = namespace_parents_.find(current);
        if (parent_it == namespace_parents_.end()) {
            break; // Should not happen if registry is properly maintained
        }
        
        current = parent_it->second;
    }
    
    return chain; // Returns [current_namespace, parent_namespace, ..., global]
}

void FunctionRegistry::register_function_declaration(StringIndex namespace_name,
                                                    StringIndex function_name,
                                                    const FunctionDescriptor& descriptor) {
    auto logger = cprime::LoggerFactory::get_logger("function_registry");
    
    // Ensure namespace is registered
    if (!is_namespace_registered(namespace_name)) {
        LOG_ERROR("Attempted to register function in unregistered namespace: {}", 
                  string_table_.get_string(namespace_name));
        return;
    }
    
    // Check for shadowing violations (only for first overload)
    if (!is_function_declared_in_namespace(namespace_name, function_name)) {
        check_for_function_shadowing_violation(namespace_name, function_name);
    }
    
    // Add the function overload
    namespace_functions_[namespace_name][function_name].push_back(descriptor);
    
    LOG_DEBUG("Registered function declaration: {}::{} (overload #{})", 
              string_table_.get_string(namespace_name),
              string_table_.get_string(function_name),
              namespace_functions_[namespace_name][function_name].size());
}

void FunctionRegistry::mark_function_called(StringIndex function_name,
                                           const std::vector<StringIndex>& namespace_path,
                                           const CallSite& call_site) {
    auto logger = cprime::LoggerFactory::get_logger("function_registry");
    
    // Try to resolve the function in the namespace chain
    std::vector<FunctionDescriptor>* overloads = resolve_function_overloads(function_name, namespace_path);
    if (!overloads) {
        LOG_DEBUG("Cannot mark unknown function as called: {}", 
                  string_table_.get_string(function_name));
        return;
    }
    
    // Find the best matching overload
    FunctionDescriptor* best_match = resolve_best_overload(function_name, namespace_path, call_site.actual_arg_types);
    if (!best_match) {
        LOG_DEBUG("No matching overload found for function call: {}", 
                  string_table_.get_string(function_name));
        return;
    }
    
    // Mark the specific overload as called
    best_match->is_called = true;
    
    // Track the call site
    StringIndex qualified_name = best_match->qualified_name;
    function_calls_[qualified_name].push_back(call_site);
    
    LOG_DEBUG("Marked function as called: {} at scope {} instruction {}", 
              string_table_.get_string(qualified_name),
              call_site.scope_index,
              call_site.instruction_index);
}

std::vector<FunctionDescriptor>* FunctionRegistry::resolve_function_overloads(StringIndex function_name,
                                                                              const std::vector<StringIndex>& namespace_path) {
    // Search from current namespace up to global
    // Since no shadowing is allowed, first match is THE set of overloads
    
    for (size_t i = 0; i < namespace_path.size(); ++i) {
        StringIndex search_namespace = namespace_path[i];
        
        auto ns_it = namespace_functions_.find(search_namespace);
        if (ns_it != namespace_functions_.end()) {
            auto func_it = ns_it->second.find(function_name);
            if (func_it != ns_it->second.end() && !func_it->second.empty()) {
                return &func_it->second; // Found overloads - this is THE definition
            }
        }
    }
    
    // Check global namespace as final fallback
    auto global_it = namespace_functions_.find(global_namespace_);
    if (global_it != namespace_functions_.end()) {
        auto func_it = global_it->second.find(function_name);
        if (func_it != global_it->second.end() && !func_it->second.empty()) {
            return &func_it->second;
        }
    }
    
    return nullptr; // Not found in any namespace in the chain
}

FunctionDescriptor* FunctionRegistry::resolve_best_overload(StringIndex function_name,
                                                           const std::vector<StringIndex>& namespace_path,
                                                           const std::vector<StringIndex>& arg_types) {
    // Get all available overloads
    std::vector<FunctionDescriptor>* overloads = resolve_function_overloads(function_name, namespace_path);
    if (!overloads) {
        return nullptr;
    }
    
    FunctionDescriptor* best_match = nullptr;
    int lowest_cost = std::numeric_limits<int>::max();
    
    // Find the overload with the lowest conversion cost
    for (auto& overload : *overloads) {
        // Extract parameter types from the overload
        std::vector<StringIndex> param_types;
        for (const auto& param : overload.parameters) {
            param_types.push_back(param.parameter_type);
        }
        
        // Check if the number of arguments matches
        if (param_types.size() != arg_types.size()) {
            continue; // Parameter count mismatch
        }
        
        // Calculate conversion cost
        int cost = calculate_conversion_cost(param_types, arg_types);
        if (cost >= 0 && cost < lowest_cost) {
            lowest_cost = cost;
            best_match = &overload;
        }
    }
    
    return best_match;
}

std::vector<FunctionDescriptor>* FunctionRegistry::find_function_overloads_in_namespace(StringIndex namespace_name, 
                                                                                       StringIndex function_name) {
    auto ns_it = namespace_functions_.find(namespace_name);
    if (ns_it == namespace_functions_.end()) {
        return nullptr;
    }
    
    auto func_it = ns_it->second.find(function_name);
    if (func_it == ns_it->second.end() || func_it->second.empty()) {
        return nullptr;
    }
    
    return &func_it->second;
}

bool FunctionRegistry::is_function_declared(StringIndex namespace_name, StringIndex function_name) const {
    auto ns_it = namespace_functions_.find(namespace_name);
    if (ns_it == namespace_functions_.end()) {
        return false;
    }
    
    auto func_it = ns_it->second.find(function_name);
    return (func_it != ns_it->second.end() && !func_it->second.empty());
}

bool FunctionRegistry::is_function_called(StringIndex qualified_function_name) const {
    return function_calls_.find(qualified_function_name) != function_calls_.end();
}

std::vector<FunctionDescriptor*> FunctionRegistry::get_functions_in_namespace(StringIndex namespace_name) {
    std::vector<FunctionDescriptor*> functions;
    
    auto ns_it = namespace_functions_.find(namespace_name);
    if (ns_it != namespace_functions_.end()) {
        for (auto& [function_name, overloads] : ns_it->second) {
            for (auto& overload : overloads) {
                functions.push_back(&overload);
            }
        }
    }
    
    return functions;
}

std::vector<FunctionDescriptor*> FunctionRegistry::get_all_called_functions() {
    std::vector<FunctionDescriptor*> called_functions;
    
    for (auto& [namespace_name, functions] : namespace_functions_) {
        for (auto& [function_name, overloads] : functions) {
            for (auto& overload : overloads) {
                if (overload.is_called) {
                    called_functions.push_back(&overload);
                }
            }
        }
    }
    
    return called_functions;
}

std::vector<CallSite> FunctionRegistry::get_call_sites(StringIndex qualified_function_name) const {
    auto it = function_calls_.find(qualified_function_name);
    if (it != function_calls_.end()) {
        return it->second;
    }
    return {};
}

size_t FunctionRegistry::get_total_registered_functions() const {
    size_t total = 0;
    for (const auto& [namespace_name, functions] : namespace_functions_) {
        for (const auto& [function_name, overloads] : functions) {
            total += overloads.size();
        }
    }
    return total;
}

size_t FunctionRegistry::get_total_called_functions() const {
    return function_calls_.size();
}

void FunctionRegistry::debug_print_function_registry() const {
    auto logger = cprime::LoggerFactory::get_logger("function_registry");
    
    LOG_DEBUG("=== Function Registry Debug ===");
    for (const auto& [namespace_name, functions] : namespace_functions_) {
        LOG_DEBUG("Namespace: {}", string_table_.get_string(namespace_name));
        
        for (const auto& [function_name, overloads] : functions) {
            LOG_DEBUG("  Function: {} ({} overloads)", 
                      string_table_.get_string(function_name), overloads.size());
            
            for (size_t i = 0; i < overloads.size(); ++i) {
                const auto& overload = overloads[i];
                LOG_DEBUG("    Overload {}: returns {} (declared: {}, called: {})", 
                          i,
                          string_table_.get_string(overload.return_type),
                          overload.is_declared,
                          overload.is_called);
            }
        }
    }
    LOG_DEBUG("=== End Function Registry Debug ===");
}

void FunctionRegistry::check_for_function_shadowing_violation(StringIndex namespace_name, StringIndex function_name) const {
    auto logger = cprime::LoggerFactory::get_logger("function_registry");
    
    // Check parent namespaces for conflicts
    std::vector<StringIndex> namespace_chain = build_namespace_chain(namespace_name);
    for (size_t i = 1; i < namespace_chain.size(); ++i) { // Skip current namespace (i=0)
        StringIndex parent_namespace = namespace_chain[i];
        if (is_function_declared_in_namespace(parent_namespace, function_name)) {
            LOG_ERROR("Function shadowing violation: {} already declared in parent namespace {}", 
                      string_table_.get_string(function_name),
                      string_table_.get_string(parent_namespace));
            throw ShadowingError(function_name, namespace_name, parent_namespace);
        }
    }
    
    // Check global namespace if not already checked
    if (std::find(namespace_chain.begin(), namespace_chain.end(), global_namespace_) == namespace_chain.end()) {
        if (is_function_declared_in_namespace(global_namespace_, function_name)) {
            LOG_ERROR("Function shadowing violation: {} already declared in global namespace", 
                      string_table_.get_string(function_name));
            throw ShadowingError(function_name, namespace_name, global_namespace_);
        }
    }
    
    // Check descendant namespaces for conflicts
    std::vector<StringIndex> descendants = get_descendant_namespaces(namespace_name);
    for (StringIndex descendant : descendants) {
        if (is_function_declared_in_namespace(descendant, function_name)) {
            LOG_ERROR("Function shadowing violation: {} already declared in descendant namespace {}", 
                      string_table_.get_string(function_name),
                      string_table_.get_string(descendant));
            throw ShadowingError(function_name, descendant, namespace_name);
        }
    }
}

bool FunctionRegistry::is_function_declared_in_namespace(StringIndex namespace_name, StringIndex function_name) const {
    auto ns_it = namespace_functions_.find(namespace_name);
    if (ns_it == namespace_functions_.end()) {
        return false;
    }
    
    auto func_it = ns_it->second.find(function_name);
    return (func_it != ns_it->second.end() && !func_it->second.empty());
}

std::vector<StringIndex> FunctionRegistry::get_descendant_namespaces(StringIndex namespace_name) const {
    std::vector<StringIndex> descendants;
    collect_descendant_namespaces(namespace_name, descendants);
    return descendants;
}

void FunctionRegistry::collect_descendant_namespaces(StringIndex namespace_name, 
                                                    std::vector<StringIndex>& descendants) const {
    // Find all namespaces that have this namespace as their parent
    for (const auto& [child_namespace, parent_namespace] : namespace_parents_) {
        if (parent_namespace.value != UINT32_MAX && parent_namespace.value == namespace_name.value) {
            descendants.push_back(child_namespace);
            // Recursively collect descendants of this child
            collect_descendant_namespaces(child_namespace, descendants);
        }
    }
}

StringIndex FunctionRegistry::build_qualified_function_name(StringIndex namespace_name, StringIndex function_name) {
    if (namespace_name.value == global_namespace_.value) {
        return function_name; // Global functions don't need qualification
    }
    
    std::string qualified = string_table_.get_string(namespace_name) + "::" + string_table_.get_string(function_name);
    return string_table_.intern(qualified);
}

bool FunctionRegistry::types_match(const std::vector<StringIndex>& signature_types,
                                  const std::vector<StringIndex>& call_types) const {
    if (signature_types.size() != call_types.size()) {
        return false;
    }
    
    for (size_t i = 0; i < signature_types.size(); ++i) {
        if (signature_types[i].value != call_types[i].value) {
            return false; // Exact match required for now
        }
    }
    
    return true;
}

int FunctionRegistry::calculate_conversion_cost(const std::vector<StringIndex>& signature_types,
                                               const std::vector<StringIndex>& call_types) const {
    if (signature_types.size() != call_types.size()) {
        return -1; // Invalid - parameter count mismatch
    }
    
    int total_cost = 0;
    
    for (size_t i = 0; i < signature_types.size(); ++i) {
        if (signature_types[i].value == call_types[i].value) {
            // Exact match - no conversion cost
            total_cost += 0;
        } else {
            // For now, no type conversions supported
            // In future: implement conversion cost calculation based on type compatibility
            return -1; // No conversion available
        }
    }
    
    return total_cost;
}

} // namespace cprime