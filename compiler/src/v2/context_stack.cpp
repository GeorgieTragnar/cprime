#include "context_stack.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace cprime::v2 {

std::string ParseContext::to_string() const {
    std::stringstream ss;
    
    switch (type) {
        case ParseContextType::TopLevel: ss << "TopLevel"; break;
        case ParseContextType::ClassDefinition: ss << "ClassDefinition"; break;
        case ParseContextType::FunctionalClassDefinition: ss << "FunctionalClassDefinition"; break;
        case ParseContextType::DangerClassDefinition: ss << "DangerClassDefinition"; break;
        case ParseContextType::UnionDefinition: ss << "UnionDefinition"; break;
        case ParseContextType::InterfaceDefinition: ss << "InterfaceDefinition"; break;
        case ParseContextType::FunctionBody: ss << "FunctionBody"; break;
        case ParseContextType::Block: ss << "Block"; break;
        case ParseContextType::TypeExpression: ss << "TypeExpression"; break;
        case ParseContextType::AccessRightsDeclaration: ss << "AccessRightsDeclaration"; break;
        case ParseContextType::FieldDeclaration: ss << "FieldDeclaration"; break;
        case ParseContextType::ParameterList: ss << "ParameterList"; break;
        case ParseContextType::ExpressionContext: ss << "ExpressionContext"; break;
        case ParseContextType::CoroutineContext: ss << "CoroutineContext"; break;
        case ParseContextType::TemplateContext: ss << "TemplateContext"; break;
        case ParseContextType::AttributeContext: ss << "AttributeContext"; break;
    }
    
    if (!attributes.empty()) {
        ss << "(";
        bool first = true;
        for (const auto& [key, value] : attributes) {
            if (!first) ss << ", ";
            ss << key << "=" << value;
            first = false;
        }
        ss << ")";
    }
    
    return ss.str();
}

// ContextStack implementation
ContextStack::ContextStack() {
    // Start with top-level context
    contexts.emplace_back(ParseContext::top_level());
}

void ContextStack::push(ParseContext context) {
    contexts.push_back(std::move(context));
}

void ContextStack::pop() {
    if (contexts.size() > 1) { // Always keep at least top-level context
        contexts.pop_back();
    }
}

void ContextStack::clear() {
    contexts.clear();
    contexts.emplace_back(ParseContext::top_level());
}

const ParseContext* ContextStack::current() const {
    return contexts.empty() ? nullptr : &contexts.back();
}

const ParseContext* ContextStack::parent() const {
    return contexts.size() < 2 ? nullptr : &contexts[contexts.size() - 2];
}

const ParseContext* ContextStack::find_context(ParseContextType type) const {
    return find_context_reverse(type);
}

const ParseContext* ContextStack::find_context(std::function<bool(const ParseContext&)> predicate) const {
    return find_context_reverse(predicate);
}

bool ContextStack::is_in_context(ParseContextType type) const {
    return find_context(type) != nullptr;
}

bool ContextStack::is_in_class_definition() const {
    return is_in_context(ParseContextType::ClassDefinition) ||
           is_in_context(ParseContextType::FunctionalClassDefinition) ||
           is_in_context(ParseContextType::DangerClassDefinition);
}

bool ContextStack::is_in_functional_class() const {
    return is_in_context(ParseContextType::FunctionalClassDefinition);
}

bool ContextStack::is_in_union_definition() const {
    return is_in_context(ParseContextType::UnionDefinition);
}

bool ContextStack::is_in_function_body() const {
    return is_in_context(ParseContextType::FunctionBody);
}

bool ContextStack::is_in_coroutine_context() const {
    return is_in_context(ParseContextType::CoroutineContext) ||
           (is_in_function_body() && 
            find_context(ParseContextType::FunctionBody)->get_bool_attribute("is_coroutine"));
}

bool ContextStack::is_in_type_expression() const {
    return is_in_context(ParseContextType::TypeExpression);
}

bool ContextStack::is_in_access_rights_declaration() const {
    return is_in_context(ParseContextType::AccessRightsDeclaration);
}

std::string ContextStack::current_class_name() const {
    // Look for any class context
    const ParseContext* class_ctx = find_context([](const ParseContext& ctx) {
        return ctx.type == ParseContextType::ClassDefinition ||
               ctx.type == ParseContextType::FunctionalClassDefinition ||
               ctx.type == ParseContextType::DangerClassDefinition;
    });
    
    return class_ctx ? class_ctx->get_attribute("class_name") : "";
}

std::string ContextStack::current_function_name() const {
    const ParseContext* func_ctx = find_context(ParseContextType::FunctionBody);
    if (func_ctx) {
        return func_ctx->get_attribute("function_name");
    }
    
    const ParseContext* coro_ctx = find_context(ParseContextType::CoroutineContext);
    return coro_ctx ? coro_ctx->get_attribute("function_name") : "";
}

bool ContextStack::current_context_is_runtime() const {
    const ParseContext* ctx = current();
    return ctx && ctx->get_bool_attribute("is_runtime");
}

bool ContextStack::is_inside_runtime_union() const {
    const ParseContext* union_ctx = find_context(ParseContextType::UnionDefinition);
    return union_ctx && union_ctx->get_bool_attribute("is_runtime");
}

std::vector<ParseContextType> ContextStack::get_context_path() const {
    std::vector<ParseContextType> path;
    path.reserve(contexts.size());
    
    for (const auto& context : contexts) {
        path.push_back(context.type);
    }
    
    return path;
}

std::string ContextStack::get_context_path_string() const {
    std::stringstream ss;
    bool first = true;
    
    for (const auto& context : contexts) {
        if (!first) ss << " -> ";
        ss << context.to_string();
        first = false;
    }
    
    return ss.str();
}

void ContextStack::dump_stack() const {
    std::cout << "Context Stack (depth " << contexts.size() << "):\n";
    for (size_t i = 0; i < contexts.size(); ++i) {
        std::cout << "  [" << i << "] " << contexts[i].to_string() << "\n";
    }
    std::cout << "Current path: " << get_context_path_string() << "\n";
}

const ParseContext* ContextStack::find_context_reverse(ParseContextType type) const {
    for (auto it = contexts.rbegin(); it != contexts.rend(); ++it) {
        if (it->type == type) {
            return &(*it);
        }
    }
    return nullptr;
}

const ParseContext* ContextStack::find_context_reverse(std::function<bool(const ParseContext&)> predicate) const {
    for (auto it = contexts.rbegin(); it != contexts.rend(); ++it) {
        if (predicate(*it)) {
            return &(*it);
        }
    }
    return nullptr;
}

// ContextResolver implementation
ContextResolver::ContextResolver(const ContextStack& context_stack)
    : context_stack(context_stack) {}

bool ContextResolver::is_runtime_access_right_context() const {
    return context_stack.is_in_class_definition() && 
           has_runtime_modifier_in_current_context();
}

bool ContextResolver::is_runtime_union_context() const {
    return context_stack.is_in_union_definition() &&
           context_stack.is_inside_runtime_union();
}

bool ContextResolver::is_runtime_type_parameter_context() const {
    return context_stack.is_in_type_expression();
}

bool ContextResolver::is_defer_raii_context() const {
    return context_stack.is_in_function_body() && 
           !context_stack.is_in_coroutine_context();
}

bool ContextResolver::is_defer_coroutine_context() const {
    return context_stack.is_in_coroutine_context();
}

bool ContextResolver::is_exposes_compile_time_context() const {
    return context_stack.is_in_class_definition() && 
           !has_runtime_modifier_in_current_context();
}

bool ContextResolver::is_exposes_runtime_context() const {
    return is_runtime_access_right_context();
}

ContextResolver::KeywordInterpretation ContextResolver::resolve_runtime_keyword() const {
    if (is_runtime_access_right_context()) {
        return KeywordInterpretation::RuntimeAccessRight;
    }
    
    if (context_stack.is_in_union_definition()) {
        return KeywordInterpretation::RuntimeUnionDeclaration;
    }
    
    if (context_stack.is_in_type_expression()) {
        return KeywordInterpretation::RuntimeTypeParameter;
    }
    
    if (context_stack.is_in_function_body() || context_stack.is_in_context(ParseContextType::Block)) {
        return KeywordInterpretation::RuntimeVariableDecl;
    }
    
    return KeywordInterpretation::Unknown;
}

ContextResolver::KeywordInterpretation ContextResolver::resolve_defer_keyword() const {
    if (is_defer_coroutine_context()) {
        return KeywordInterpretation::DeferCoroutine;
    }
    
    if (is_defer_raii_context()) {
        return KeywordInterpretation::DeferRaii;
    }
    
    return KeywordInterpretation::Unknown;
}

ContextResolver::KeywordInterpretation ContextResolver::resolve_exposes_keyword() const {
    if (is_exposes_runtime_context()) {
        return KeywordInterpretation::ExposesRuntime;
    }
    
    if (is_exposes_compile_time_context()) {
        return KeywordInterpretation::ExposesCompileTime;
    }
    
    return KeywordInterpretation::Unknown;
}

ContextResolver::KeywordInterpretation ContextResolver::resolve_class_keyword() const {
    // Look at surrounding context for class type modifiers
    const ParseContext* current = context_stack.current();
    
    if (current && current->has_attribute("class_type")) {
        std::string class_type = current->get_attribute("class_type");
        if (class_type == "functional") {
            return KeywordInterpretation::ClassFunctional;
        } else if (class_type == "danger") {
            return KeywordInterpretation::ClassDanger;
        }
    }
    
    return KeywordInterpretation::ClassData; // Default
}

ContextResolver::KeywordInterpretation ContextResolver::resolve_union_keyword() const {
    if (context_stack.is_inside_runtime_union()) {
        return KeywordInterpretation::UnionRuntime;
    }
    
    return KeywordInterpretation::UnionCompileTime; // Default
}

std::string ContextResolver::interpretation_to_string(KeywordInterpretation interpretation) const {
    switch (interpretation) {
        case KeywordInterpretation::RuntimeAccessRight: return "RuntimeAccessRight";
        case KeywordInterpretation::RuntimeUnionDeclaration: return "RuntimeUnionDeclaration";
        case KeywordInterpretation::RuntimeTypeParameter: return "RuntimeTypeParameter";
        case KeywordInterpretation::RuntimeVariableDecl: return "RuntimeVariableDecl";
        case KeywordInterpretation::DeferRaii: return "DeferRaii";
        case KeywordInterpretation::DeferCoroutine: return "DeferCoroutine";
        case KeywordInterpretation::ExposesCompileTime: return "ExposesCompileTime";
        case KeywordInterpretation::ExposesRuntime: return "ExposesRuntime";
        case KeywordInterpretation::ClassData: return "ClassData";
        case KeywordInterpretation::ClassFunctional: return "ClassFunctional";
        case KeywordInterpretation::ClassDanger: return "ClassDanger";
        case KeywordInterpretation::UnionCompileTime: return "UnionCompileTime";
        case KeywordInterpretation::UnionRuntime: return "UnionRuntime";
        case KeywordInterpretation::Unknown: return "Unknown";
    }
    return "Invalid";
}

bool ContextResolver::is_valid_interpretation(KeywordInterpretation interpretation) const {
    return interpretation != KeywordInterpretation::Unknown;
}

bool ContextResolver::has_runtime_modifier_in_current_context() const {
    const ParseContext* access_rights_ctx = context_stack.find_context(ParseContextType::AccessRightsDeclaration);
    return access_rights_ctx && access_rights_ctx->get_bool_attribute("is_runtime");
}

bool ContextResolver::is_in_coroutine_or_async_context() const {
    return context_stack.is_in_coroutine_context() ||
           (context_stack.is_in_function_body() && 
            context_stack.find_context(ParseContextType::FunctionBody)->get_bool_attribute("is_coroutine"));
}

bool ContextResolver::is_in_template_or_type_context() const {
    return context_stack.is_in_context(ParseContextType::TemplateContext) ||
           context_stack.is_in_type_expression();
}

// ScopedContext implementation
ScopedContext::ScopedContext(ContextStack& stack, ParseContext context)
    : stack(stack), active(true) {
    stack.push(std::move(context));
}

ScopedContext::~ScopedContext() {
    if (active) {
        stack.pop();
    }
}

} // namespace cprime::v2