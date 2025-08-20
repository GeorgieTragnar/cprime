#include "type_registry.h"
#include "logger.h"
#include <algorithm>

namespace cprime {

TypeRegistry::TypeRegistry(StringTable& string_table) 
    : string_table_(string_table) {
    // Initialize global namespace
    global_namespace_ = string_table_.intern("__global__");
    register_namespace(global_namespace_, StringIndex{UINT32_MAX}); // Global has no parent
}

void TypeRegistry::register_namespace(StringIndex namespace_name, StringIndex parent_namespace) {
    auto logger = cprime::LoggerFactory::get_logger("type_registry");
    
    if (is_namespace_registered(namespace_name)) {
        LOG_DEBUG("Namespace already registered: {}", string_table_.get_string(namespace_name));
        return;
    }
    
    namespace_parents_[namespace_name] = parent_namespace;
    namespace_types_[namespace_name] = std::unordered_map<StringIndex, TypeDescriptor>{};
    namespace_instantiations_[namespace_name] = std::unordered_set<StringIndex>{};
    
    LOG_DEBUG("Registered namespace: {} with parent: {}", 
              string_table_.get_string(namespace_name),
              parent_namespace.value == UINT32_MAX ? "__global__" : string_table_.get_string(parent_namespace));
}

bool TypeRegistry::is_namespace_registered(StringIndex namespace_name) const {
    return namespace_parents_.find(namespace_name) != namespace_parents_.end();
}

std::vector<StringIndex> TypeRegistry::build_namespace_chain(StringIndex namespace_name) const {
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

void TypeRegistry::register_type_declaration(StringIndex namespace_name,
                                            StringIndex identifier,
                                            const TypeDescriptor& descriptor) {
    auto logger = cprime::LoggerFactory::get_logger("type_registry");
    
    // Ensure namespace is registered
    if (!is_namespace_registered(namespace_name)) {
        LOG_ERROR("Attempted to register type in unregistered namespace: {}", 
                  string_table_.get_string(namespace_name));
        return;
    }
    
    // Check for shadowing violations
    check_for_shadowing_violation(namespace_name, identifier);
    
    // Register the type
    namespace_types_[namespace_name][identifier] = descriptor;
    
    LOG_DEBUG("Registered type declaration: {}::{}", 
              string_table_.get_string(namespace_name),
              string_table_.get_string(identifier));
}

void TypeRegistry::mark_type_instantiated(StringIndex identifier,
                                         const std::vector<StringIndex>& namespace_path) {
    auto logger = cprime::LoggerFactory::get_logger("type_registry");
    
    // Try to resolve the type in the namespace chain
    TypeDescriptor* type = resolve_type(identifier, namespace_path);
    if (!type) {
        LOG_DEBUG("Cannot mark unknown type as instantiated: {}", 
                  string_table_.get_string(identifier));
        return;
    }
    
    // Mark as instantiated in the namespace where it was found
    type->is_instantiated = true;
    namespace_instantiations_[type->namespace_name].insert(identifier);
    
    LOG_DEBUG("Marked type as instantiated: {}::{}", 
              string_table_.get_string(type->namespace_name),
              string_table_.get_string(identifier));
}

TypeDescriptor* TypeRegistry::resolve_type(StringIndex identifier,
                                          const std::vector<StringIndex>& namespace_path) {
    // Search from current namespace up to global
    // Since no shadowing is allowed, first match is THE match
    
    for (size_t i = 0; i < namespace_path.size(); ++i) {
        StringIndex search_namespace = namespace_path[i];
        
        auto ns_it = namespace_types_.find(search_namespace);
        if (ns_it != namespace_types_.end()) {
            auto type_it = ns_it->second.find(identifier);
            if (type_it != ns_it->second.end()) {
                return &type_it->second; // Found it - this is THE definition
            }
        }
    }
    
    // Check global namespace as final fallback
    auto global_it = namespace_types_.find(global_namespace_);
    if (global_it != namespace_types_.end()) {
        auto type_it = global_it->second.find(identifier);
        if (type_it != global_it->second.end()) {
            return &type_it->second;
        }
    }
    
    return nullptr; // Not found in any namespace in the chain
}

TypeDescriptor* TypeRegistry::find_type_in_namespace(StringIndex namespace_name, StringIndex identifier) {
    auto ns_it = namespace_types_.find(namespace_name);
    if (ns_it == namespace_types_.end()) {
        return nullptr;
    }
    
    auto type_it = ns_it->second.find(identifier);
    if (type_it == ns_it->second.end()) {
        return nullptr;
    }
    
    return &type_it->second;
}

bool TypeRegistry::is_type_declared(StringIndex namespace_name, StringIndex identifier) const {
    auto ns_it = namespace_types_.find(namespace_name);
    if (ns_it == namespace_types_.end()) {
        return false;
    }
    
    return ns_it->second.find(identifier) != ns_it->second.end();
}

bool TypeRegistry::is_type_instantiated(StringIndex namespace_name, StringIndex identifier) const {
    auto inst_it = namespace_instantiations_.find(namespace_name);
    if (inst_it == namespace_instantiations_.end()) {
        return false;
    }
    
    return inst_it->second.find(identifier) != inst_it->second.end();
}

std::vector<TypeDescriptor*> TypeRegistry::get_types_in_namespace(StringIndex namespace_name) {
    std::vector<TypeDescriptor*> types;
    
    auto ns_it = namespace_types_.find(namespace_name);
    if (ns_it != namespace_types_.end()) {
        for (auto& [identifier, descriptor] : ns_it->second) {
            types.push_back(&descriptor);
        }
    }
    
    return types;
}

std::vector<TypeDescriptor*> TypeRegistry::get_all_instantiated_types() {
    std::vector<TypeDescriptor*> instantiated_types;
    
    for (auto& [namespace_name, types] : namespace_types_) {
        for (auto& [identifier, descriptor] : types) {
            if (descriptor.is_instantiated) {
                instantiated_types.push_back(&descriptor);
            }
        }
    }
    
    return instantiated_types;
}

size_t TypeRegistry::get_total_registered_types() const {
    size_t total = 0;
    for (const auto& [namespace_name, types] : namespace_types_) {
        total += types.size();
    }
    return total;
}

size_t TypeRegistry::get_total_instantiated_types() const {
    size_t total = 0;
    for (const auto& [namespace_name, instantiated] : namespace_instantiations_) {
        total += instantiated.size();
    }
    return total;
}

void TypeRegistry::debug_print_namespace_hierarchy() const {
    auto logger = cprime::LoggerFactory::get_logger("type_registry");
    
    LOG_DEBUG("=== Type Registry Namespace Hierarchy ===");
    for (const auto& [namespace_name, parent] : namespace_parents_) {
        std::string parent_name = (parent.value == UINT32_MAX) ? "__global__" : string_table_.get_string(parent);
        LOG_DEBUG("Namespace: {} -> Parent: {}", 
                  string_table_.get_string(namespace_name), parent_name);
        
        // Show types in this namespace
        auto types_it = namespace_types_.find(namespace_name);
        if (types_it != namespace_types_.end()) {
            for (const auto& [identifier, descriptor] : types_it->second) {
                LOG_DEBUG("  Type: {} (declared: {}, instantiated: {})", 
                          string_table_.get_string(identifier),
                          descriptor.is_declared,
                          descriptor.is_instantiated);
            }
        }
    }
    LOG_DEBUG("=== End Namespace Hierarchy ===");
}

void TypeRegistry::check_for_shadowing_violation(StringIndex namespace_name, StringIndex identifier) const {
    auto logger = cprime::LoggerFactory::get_logger("type_registry");
    
    // Check parent namespaces for conflicts
    std::vector<StringIndex> namespace_chain = build_namespace_chain(namespace_name);
    for (size_t i = 1; i < namespace_chain.size(); ++i) { // Skip current namespace (i=0)
        StringIndex parent_namespace = namespace_chain[i];
        if (is_identifier_declared_in_namespace(parent_namespace, identifier)) {
            LOG_ERROR("Shadowing violation: {} already declared in parent namespace {}", 
                      string_table_.get_string(identifier),
                      string_table_.get_string(parent_namespace));
            throw ShadowingError(identifier, namespace_name, parent_namespace);
        }
    }
    
    // Check global namespace if not already checked
    if (std::find(namespace_chain.begin(), namespace_chain.end(), global_namespace_) == namespace_chain.end()) {
        if (is_identifier_declared_in_namespace(global_namespace_, identifier)) {
            LOG_ERROR("Shadowing violation: {} already declared in global namespace", 
                      string_table_.get_string(identifier));
            throw ShadowingError(identifier, namespace_name, global_namespace_);
        }
    }
    
    // Check descendant namespaces for conflicts
    std::vector<StringIndex> descendants = get_descendant_namespaces(namespace_name);
    for (StringIndex descendant : descendants) {
        if (is_identifier_declared_in_namespace(descendant, identifier)) {
            LOG_ERROR("Shadowing violation: {} already declared in descendant namespace {}", 
                      string_table_.get_string(identifier),
                      string_table_.get_string(descendant));
            throw ShadowingError(identifier, descendant, namespace_name);
        }
    }
}

bool TypeRegistry::is_identifier_declared_in_namespace(StringIndex namespace_name, StringIndex identifier) const {
    auto ns_it = namespace_types_.find(namespace_name);
    if (ns_it == namespace_types_.end()) {
        return false;
    }
    
    return ns_it->second.find(identifier) != ns_it->second.end();
}

std::vector<StringIndex> TypeRegistry::get_descendant_namespaces(StringIndex namespace_name) const {
    std::vector<StringIndex> descendants;
    collect_descendant_namespaces(namespace_name, descendants);
    return descendants;
}

void TypeRegistry::collect_descendant_namespaces(StringIndex namespace_name, 
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

StringIndex TypeRegistry::build_qualified_name(StringIndex namespace_name, StringIndex identifier) {
    if (namespace_name.value == global_namespace_.value) {
        return identifier; // Global types don't need qualification
    }
    
    std::string qualified = string_table_.get_string(namespace_name) + "::" + string_table_.get_string(identifier);
    return string_table_.intern(qualified);
}

} // namespace cprime