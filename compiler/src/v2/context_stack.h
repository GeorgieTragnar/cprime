#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>

namespace cprime::v2 {

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
    
    static ParseContext union_definition(const std::string& union_name, bool is_runtime = false) {
        return ParseContext(ParseContextType::UnionDefinition, {
            {"union_name", union_name},
            {"is_runtime", is_runtime ? "true" : "false"}
        });
    }
    
    static ParseContext function_body(const std::string& function_name, bool is_coroutine = false) {
        return ParseContext(ParseContextType::FunctionBody, {
            {"function_name", function_name},
            {"is_coroutine", is_coroutine ? "true" : "false"}
        });
    }
    
    static ParseContext access_rights_declaration(const std::string& access_right, bool is_runtime = false) {
        return ParseContext(ParseContextType::AccessRightsDeclaration, {
            {"access_right", access_right},
            {"is_runtime", is_runtime ? "true" : "false"}
        });
    }
    
    static ParseContext type_expression() {
        return ParseContext(ParseContextType::TypeExpression);
    }
    
    static ParseContext coroutine_context(const std::string& function_name) {
        return ParseContext(ParseContextType::CoroutineContext, {
            {"function_name", function_name}
        });
    }
    
    // Attribute accessors
    bool has_attribute(const std::string& key) const {
        return attributes.find(key) != attributes.end();
    }
    
    std::string get_attribute(const std::string& key, const std::string& default_value = "") const {
        auto it = attributes.find(key);
        return it != attributes.end() ? it->second : default_value;
    }
    
    bool get_bool_attribute(const std::string& key, bool default_value = false) const {
        auto value = get_attribute(key);
        if (value.empty()) return default_value;
        return value == "true";
    }
    
    // Debug string representation
    std::string to_string() const;
};

/**
 * Context stack for tracking nested parsing contexts.
 * This is the core of the context-sensitive keyword resolution system.
 */
class ContextStack {
public:
    ContextStack();
    
    // Stack operations
    void push(ParseContext context);
    void pop();
    void clear();
    
    // Context queries
    const ParseContext* current() const;
    const ParseContext* parent() const;
    const ParseContext* find_context(ParseContextType type) const;
    const ParseContext* find_context(std::function<bool(const ParseContext&)> predicate) const;
    
    // Context checking helpers
    bool is_in_context(ParseContextType type) const;
    bool is_in_class_definition() const;
    bool is_in_functional_class() const;
    bool is_in_union_definition() const;
    bool is_in_function_body() const;
    bool is_in_coroutine_context() const;
    bool is_in_type_expression() const;
    bool is_in_access_rights_declaration() const;
    
    // Advanced context queries
    std::string current_class_name() const;
    std::string current_function_name() const;
    bool current_context_is_runtime() const;
    bool is_inside_runtime_union() const;
    
    // Stack introspection
    size_t depth() const { return contexts.size(); }
    bool empty() const { return contexts.empty(); }
    
    // Context path - useful for debugging and complex context resolution
    std::vector<ParseContextType> get_context_path() const;
    std::string get_context_path_string() const;
    
    // Debug output
    void dump_stack() const;
    
private:
    std::vector<ParseContext> contexts;
    
    // Helper methods
    const ParseContext* find_context_reverse(ParseContextType type) const;
    const ParseContext* find_context_reverse(std::function<bool(const ParseContext&)> predicate) const;
};

/**
 * Context-sensitive keyword resolution helper.
 * This class provides utilities for determining keyword meaning based on context.
 */
class ContextResolver {
public:
    explicit ContextResolver(const ContextStack& context_stack);
    
    // Keyword interpretation queries
    bool is_runtime_access_right_context() const;
    bool is_runtime_union_context() const;
    bool is_runtime_type_parameter_context() const;
    bool is_defer_raii_context() const;
    bool is_defer_coroutine_context() const;
    bool is_exposes_compile_time_context() const;
    bool is_exposes_runtime_context() const;
    
    // Context-sensitive token classification
    enum class KeywordInterpretation {
        RuntimeAccessRight,      // "runtime exposes UserOps"
        RuntimeUnionDeclaration, // "union runtime ConnectionSpace"
        RuntimeTypeParameter,    // "Connection<runtime UserOps>"
        RuntimeVariableDecl,     // "let conn: runtime Connection = ..."
        DeferRaii,              // "defer FileOps::destruct(&mut file)"
        DeferCoroutine,         // "co_defer cleanup_resources()"
        ExposesCompileTime,     // "exposes UserOps { ... }" (default)
        ExposesRuntime,         // "runtime exposes UserOps { ... }"
        ClassData,              // "class Connection { ... }" (default)
        ClassFunctional,        // "functional class FileOps { ... }"
        ClassDanger,            // "danger class UnsafeWrapper { ... }"
        UnionCompileTime,       // "union Message { ... }" (default)
        UnionRuntime,           // "union runtime ConnectionSpace { ... }"
        Unknown                 // Could not determine interpretation
    };
    
    KeywordInterpretation resolve_runtime_keyword() const;
    KeywordInterpretation resolve_defer_keyword() const;
    KeywordInterpretation resolve_exposes_keyword() const;
    KeywordInterpretation resolve_class_keyword() const;
    KeywordInterpretation resolve_union_keyword() const;
    
    // Interpretation helpers
    std::string interpretation_to_string(KeywordInterpretation interpretation) const;
    bool is_valid_interpretation(KeywordInterpretation interpretation) const;
    
private:
    const ContextStack& context_stack;
    
    // Internal helper methods
    bool has_runtime_modifier_in_current_context() const;
    bool is_in_coroutine_or_async_context() const;
    bool is_in_template_or_type_context() const;
};

/**
 * Scoped context guard for RAII-style context management.
 * Automatically pushes context on construction and pops on destruction.
 */
class ScopedContext {
public:
    ScopedContext(ContextStack& stack, ParseContext context);
    ~ScopedContext();
    
    // Non-copyable, moveable
    ScopedContext(const ScopedContext&) = delete;
    ScopedContext& operator=(const ScopedContext&) = delete;
    ScopedContext(ScopedContext&&) = default;
    ScopedContext& operator=(ScopedContext&&) = default;
    
private:
    ContextStack& stack;
    bool active;
};

} // namespace cprime::v2