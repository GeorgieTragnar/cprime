#pragma once

#include <string>
#include <unordered_map>

namespace cprime {

/**
 * Parsing contexts for context-sensitive keyword resolution.
 * These contexts determine how keywords like 'runtime', 'defer', 'exposes' are interpreted.
 */
enum class ParseContextType {
    TopLevel,                    // Global scope
    ClassDefinition,            // Inside class { ... }
    FunctionalClassDefinition,  // Inside functional class { ... }
    DangerClassDefinition,      // Inside danger class { ... }
    UnionDefinition,            // Inside union { ... }
    InterfaceDefinition,        // Inside interface { ... }
    FunctionBody,               // Inside function body { ... }
    Block,                      // Inside general block { ... }
    TypeExpression,             // In type position: Connection<runtime UserOps>
    AccessRightsDeclaration,    // In "exposes" declaration
    FieldDeclaration,           // In class field declaration
    ParameterList,              // In function parameter list
    ExpressionContext,          // In expression evaluation
    CoroutineContext,           // In async function or coroutine
    TemplateContext,            // In template/generic parameter list
    AttributeContext,           // In attribute declaration #[...]
};

/**
 * Context data structure - holds context type and associated metadata.
 */
struct ParseContext {
    ParseContextType type;
    std::unordered_map<std::string, std::string> attributes;
    
    ParseContext(ParseContextType type) : type(type) {}
    
    ParseContext(ParseContextType type, std::unordered_map<std::string, std::string> attributes)
        : type(type), attributes(std::move(attributes)) {}
    
    // Convenience constructors for common contexts
    static ParseContext top_level() {
        return ParseContext(ParseContextType::TopLevel);
    }
    
    static ParseContext class_definition(const std::string& class_name, bool is_data_class = true) {
        return ParseContext(ParseContextType::ClassDefinition, {
            {"class_name", class_name},
            {"is_data_class", is_data_class ? "true" : "false"}
        });
    }
    
    static ParseContext functional_class_definition(const std::string& class_name) {
        return ParseContext(ParseContextType::FunctionalClassDefinition, {
            {"class_name", class_name}
        });
    }
    
    static ParseContext danger_class_definition(const std::string& class_name) {
        return ParseContext(ParseContextType::DangerClassDefinition, {
            {"class_name", class_name}
        });
    }
    
    static ParseContext union_definition(const std::string& union_name, bool is_runtime = false) {
        return ParseContext(ParseContextType::UnionDefinition, {
            {"union_name", union_name},
            {"is_runtime", is_runtime ? "true" : "false"}
        });
    }
    
    static ParseContext interface_definition(const std::string& interface_name) {
        return ParseContext(ParseContextType::InterfaceDefinition, {
            {"interface_name", interface_name}
        });
    }
    
    static ParseContext function_body(const std::string& function_name, bool is_async = false) {
        return ParseContext(ParseContextType::FunctionBody, {
            {"function_name", function_name},
            {"is_async", is_async ? "true" : "false"}
        });
    }
    
    static ParseContext type_expression() {
        return ParseContext(ParseContextType::TypeExpression);
    }
    
    static ParseContext access_rights_declaration(const std::string& access_right_name, bool is_runtime = false) {
        return ParseContext(ParseContextType::AccessRightsDeclaration, {
            {"access_right_name", access_right_name},
            {"is_runtime", is_runtime ? "true" : "false"}
        });
    }
    
    static ParseContext block() {
        return ParseContext(ParseContextType::Block);
    }
    
    static ParseContext field_declaration(const std::string& field_name) {
        return ParseContext(ParseContextType::FieldDeclaration, {
            {"field_name", field_name}
        });
    }
    
    static ParseContext parameter_list() {
        return ParseContext(ParseContextType::ParameterList);
    }
    
    static ParseContext expression_context() {
        return ParseContext(ParseContextType::ExpressionContext);
    }
    
    static ParseContext coroutine_context(const std::string& coroutine_name) {
        return ParseContext(ParseContextType::CoroutineContext, {
            {"coroutine_name", coroutine_name}
        });
    }
    
    static ParseContext template_context() {
        return ParseContext(ParseContextType::TemplateContext);
    }
    
    static ParseContext attribute_context() {
        return ParseContext(ParseContextType::AttributeContext);
    }
    
    // Attribute access
    bool has_attribute(const std::string& key) const {
        return attributes.find(key) != attributes.end();
    }
    
    std::string get_attribute(const std::string& key, const std::string& default_value = "") const {
        auto it = attributes.find(key);
        return it != attributes.end() ? it->second : default_value;
    }
    
    void set_attribute(const std::string& key, const std::string& value) {
        attributes[key] = value;
    }
    
    bool get_bool_attribute(const std::string& key, bool default_value = false) const {
        auto value = get_attribute(key);
        if (value.empty()) return default_value;
        return value == "true";
    }
    
    // Context queries
    bool is_class_context() const {
        return type == ParseContextType::ClassDefinition ||
               type == ParseContextType::FunctionalClassDefinition ||
               type == ParseContextType::DangerClassDefinition;
    }
    
    bool is_function_context() const {
        return type == ParseContextType::FunctionBody ||
               type == ParseContextType::CoroutineContext;
    }
    
    bool is_runtime_context() const {
        return has_attribute("is_runtime") && get_attribute("is_runtime") == "true";
    }
    
    bool is_type_context() const {
        return type == ParseContextType::TypeExpression ||
               type == ParseContextType::TemplateContext;
    }
    
    // Debug representation
    std::string to_string() const;
};

} // namespace cprime