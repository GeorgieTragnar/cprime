#pragma once

#include "../common/parse_context.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>

namespace cprime {

// ParseContextType and ParseContext are now defined in common/parse_context.h

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

} // namespace cprime