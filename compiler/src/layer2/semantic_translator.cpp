#include "semantic_translator.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <algorithm>

namespace cprime {

// SemanticTranslator implementation
SemanticTranslator::SemanticTranslator(RawTokenStream raw_tokens)
    : raw_tokens(std::move(raw_tokens)), position(0) {
    context_resolver = std::make_unique<ContextResolver>(context_stack);
}

std::vector<SemanticToken> SemanticTranslator::translate() {
    semantic_tokens.clear();
    errors.clear();
    position = 0;
    
    // Reset context stack to initial state
    context_stack.clear();
    
    while (!is_at_end()) {
        try {
            SemanticToken semantic_token = translate_next_token();
            
            // Update context based on translated token
            enter_context_from_semantic_token(semantic_token);
            
            semantic_tokens.push_back(std::move(semantic_token));
            
        } catch (const std::exception& e) {
            error("Translation error: " + std::string(e.what()));
            advance_raw_token(); // Skip problematic token
        }
    }
    
    return semantic_tokens;
}

SemanticTokenStream SemanticTranslator::translate_to_stream() {
    return SemanticTokenStream(translate());
}

SemanticToken SemanticTranslator::translate_next_token() {
    const RawToken& raw_token = current_raw_token();
    
    // Update context based on raw token
    update_context_from_token(raw_token);
    
    // Handle context-sensitive keywords
    if (raw_token.type == RawTokenType::KEYWORD) {
        if (raw_token.value == "runtime") {
            return resolve_runtime_keyword();
        } else if (raw_token.value == "defer") {
            return resolve_defer_keyword();
        } else if (raw_token.value == "exposes") {
            return resolve_exposes_keyword();
        } else if (raw_token.value == "class") {
            return resolve_class_keyword();
        } else if (raw_token.value == "union") {
            return resolve_union_keyword();
        } else if (raw_token.value == "interface") {
            return resolve_interface_keyword();
        } else if (raw_token.value == "fn" || raw_token.value == "async") {
            return resolve_function_keyword();
        }
        
        // Regular keyword - pass through as identifier for now
        advance_raw_token();
        return SemanticToken::identifier(raw_token.value, raw_token.line, raw_token.column);
    }
    
    // Handle other token types
    switch (raw_token.type) {
        case RawTokenType::IDENTIFIER:
            return handle_identifier();
        case RawTokenType::LITERAL:
            return handle_literal();
        case RawTokenType::SYMBOL:
            return handle_symbol();
        case RawTokenType::COMMENT:
            return handle_comment();
        default:
            advance_raw_token();
            return SemanticToken::identifier(raw_token.value, raw_token.line, raw_token.column);
    }
}

SemanticToken SemanticTranslator::resolve_runtime_keyword() {
    const RawToken& runtime_token = current_raw_token();
    advance_raw_token(); // consume "runtime"
    
    auto interpretation = context_resolver->resolve_runtime_keyword();
    
    switch (interpretation) {
        case ContextResolver::KeywordInterpretation::RuntimeAccessRight: {
            // "runtime exposes UserOps { ... }"
            if (!current_raw_token().is_keyword("exposes")) {
                error("Expected 'exposes' after 'runtime'");
                return SemanticToken::placeholder("runtime_access_right", "Expected exposes", 
                                                 runtime_token.line, runtime_token.column);
            }
            
            return resolve_access_rights_declaration();
        }
        
        case ContextResolver::KeywordInterpretation::RuntimeUnionDeclaration: {
            // This is actually handled by resolve_union_keyword when it sees "union runtime"
            // We should not reach here in normal parsing
            error("Unexpected runtime keyword in union context");
            return SemanticToken::placeholder("runtime_union", "Unexpected runtime in union", 
                                             runtime_token.line, runtime_token.column);
        }
        
        case ContextResolver::KeywordInterpretation::RuntimeTypeParameter: {
            // "Connection<runtime UserOps>"
            if (!current_raw_token().is_identifier()) {
                error("Expected type name after 'runtime' in type expression");
                return SemanticToken::placeholder("runtime_type_param", "Expected type name", 
                                                 runtime_token.line, runtime_token.column);
            }
            
            std::string type_name = current_raw_token().value;
            advance_raw_token();
            
            return SemanticToken::runtime_type_parameter(type_name, runtime_token.line, runtime_token.column);
        }
        
        case ContextResolver::KeywordInterpretation::RuntimeVariableDecl: {
            // "let conn: runtime Connection = ..."
            // Parse the type expression that follows
            return resolve_type_expression();
        }
        
        default:
            error("Could not resolve 'runtime' keyword in current context");
            return SemanticToken::placeholder("runtime_unknown", "Unresolved runtime keyword", 
                                             runtime_token.line, runtime_token.column);
    }
}

SemanticToken SemanticTranslator::resolve_defer_keyword() {
    const RawToken& defer_token = current_raw_token();
    advance_raw_token(); // consume "defer"
    
    auto interpretation = context_resolver->resolve_defer_keyword();
    
    switch (interpretation) {
        case ContextResolver::KeywordInterpretation::DeferRaii: {
            // "defer FileOps::destruct(&mut file)"
            std::string function_call = parse_function_call();
            return SemanticToken::raii_defer(function_call, defer_token.line, defer_token.column);
        }
        
        case ContextResolver::KeywordInterpretation::DeferCoroutine: {
            // "co_defer cleanup_resources()"
            std::string cleanup_expression = parse_expression_until(";");
            return SemanticToken::coroutine_defer(cleanup_expression, defer_token.line, defer_token.column);
        }
        
        default:
            error("Could not resolve 'defer' keyword in current context");
            return SemanticToken::placeholder("defer_unknown", "Unresolved defer keyword", 
                                             defer_token.line, defer_token.column);
    }
}

SemanticToken SemanticTranslator::resolve_exposes_keyword() {
    const RawToken& exposes_token = current_raw_token();
    advance_raw_token(); // consume "exposes"
    
    auto interpretation = context_resolver->resolve_exposes_keyword();
    
    switch (interpretation) {
        case ContextResolver::KeywordInterpretation::ExposesCompileTime: {
            // "exposes UserOps { ... }"
            return resolve_access_rights_declaration();
        }
        
        case ContextResolver::KeywordInterpretation::ExposesRuntime: {
            // This should be handled by resolve_runtime_keyword() when it sees "runtime exposes"
            // If we reach here, it means we have a standalone "exposes" in a runtime context
            return resolve_access_rights_declaration();
        }
        
        default:
            error("Could not resolve 'exposes' keyword in current context");
            return SemanticToken::placeholder("exposes_unknown", "Unresolved exposes keyword", 
                                             exposes_token.line, exposes_token.column);
    }
}

SemanticToken SemanticTranslator::resolve_class_keyword() {
    advance_raw_token(); // consume "class"
    
    return resolve_class_declaration();
}

SemanticToken SemanticTranslator::resolve_union_keyword() {
    advance_raw_token(); // consume "union"
    
    // Check if next token is "runtime"
    if (current_raw_token().is_keyword("runtime")) {
        advance_raw_token(); // consume "runtime"
    }
    
    return resolve_union_declaration();
}

SemanticToken SemanticTranslator::resolve_interface_keyword() {
    const RawToken& interface_token = current_raw_token();
    advance_raw_token(); // consume "interface"
    
    // Parse interface declaration
    if (!current_raw_token().is_identifier()) {
        error("Expected interface name after 'interface'");
        return SemanticToken::placeholder("interface", "Expected interface name", 
                                         interface_token.line, interface_token.column);
    }
    
    std::string interface_name = current_raw_token().value;
    advance_raw_token();
    
    // Push interface context
    context_stack.push(ParseContext(ParseContextType::InterfaceDefinition, {
        {"interface_name", interface_name}
    }));
    
    SemanticToken token(SemanticTokenType::Interface, interface_token.line, interface_token.column);
    token.set_name(interface_name);
    return token;
}

SemanticToken SemanticTranslator::resolve_function_keyword() {
    const RawToken& fn_token = current_raw_token();
    bool is_async = fn_token.value == "async";
    advance_raw_token(); // consume "fn" or "async"
    
    if (is_async && !current_raw_token().is_keyword("fn")) {
        error("Expected 'fn' after 'async'");
        return SemanticToken::placeholder("async_fn", "Expected fn after async", 
                                         fn_token.line, fn_token.column);
    }
    
    if (is_async) {
        advance_raw_token(); // consume "fn"
    }
    
    if (!current_raw_token().is_identifier()) {
        error("Expected function name after 'fn'");
        return SemanticToken::placeholder("function", "Expected function name", 
                                         fn_token.line, fn_token.column);
    }
    
    std::string function_name = current_raw_token().value;
    advance_raw_token();
    
    // Push function context
    context_stack.push(ParseContext::function_body(function_name, is_async));
    
    SemanticTokenType type = is_async ? SemanticTokenType::CoroutineFunction : SemanticTokenType::Function;
    SemanticToken token(type, fn_token.line, fn_token.column);
    token.set_name(function_name);
    token.set_attribute("is_async", is_async ? "true" : "false");
    return token;
}

SemanticToken SemanticTranslator::resolve_access_rights_declaration() {
    // At this point, we've consumed "exposes" (and possibly "runtime" before it)
    
    if (!current_raw_token().is_identifier()) {
        error("Expected access right name after 'exposes'");
        return SemanticToken::placeholder("access_right", "Expected access right name");
    }
    
    std::string access_right = current_raw_token().value;
    const RawToken& name_token = current_raw_token();
    advance_raw_token();
    
    // Parse granted fields list: { field1, field2, ... }
    std::vector<std::string> granted_fields = parse_field_list();
    
    // Determine if this is runtime or compile-time based on context
    bool is_runtime = context_stack.current_context_is_runtime() || 
                     context_stack.find_context(ParseContextType::AccessRightsDeclaration) != nullptr;
    
    // Push access rights context
    context_stack.push(ParseContext::access_rights_declaration(access_right, is_runtime));
    
    if (is_runtime) {
        return SemanticToken::runtime_access_right_declaration(access_right, granted_fields, 
                                                             name_token.line, name_token.column);
    } else {
        return SemanticToken::compile_time_access_right_declaration(access_right, granted_fields, 
                                                                  name_token.line, name_token.column);
    }
}

SemanticToken SemanticTranslator::resolve_union_declaration() {
    // At this point, we've consumed "union" (and possibly "runtime" before it)
    
    if (!current_raw_token().is_identifier()) {
        error("Expected union name");
        return SemanticToken::placeholder("union", "Expected union name");
    }
    
    std::string union_name = current_raw_token().value;
    const RawToken& name_token = current_raw_token();
    advance_raw_token();
    
    // Check if union is runtime (context should have been set)
    bool is_runtime = context_stack.is_inside_runtime_union();
    
    // Parse union variants
    std::vector<std::string> variants = parse_union_variants();
    
    // Push union context
    context_stack.push(ParseContext::union_definition(union_name, is_runtime));
    
    if (is_runtime) {
        return SemanticToken::runtime_union(union_name, variants, name_token.line, name_token.column);
    } else {
        return SemanticToken::compile_time_union(union_name, variants, name_token.line, name_token.column);
    }
}

SemanticToken SemanticTranslator::resolve_class_declaration() {
    // At this point, we've consumed "class"
    
    if (!current_raw_token().is_identifier()) {
        error("Expected class name after 'class'");
        return SemanticToken::placeholder("class", "Expected class name");
    }
    
    std::string class_name = current_raw_token().value;
    const RawToken& name_token = current_raw_token();
    advance_raw_token();
    
    // Determine class type (default is data class)
    std::string class_type = "data";
    
    // Check for functional or danger modifiers in context
    const ParseContext* ctx = context_stack.current();
    if (ctx && ctx->has_attribute("class_type")) {
        class_type = ctx->get_attribute("class_type");
    }
    
    // Push class context
    if (class_type == "functional") {
        context_stack.push(ParseContext::functional_class_definition(class_name));
    } else {
        context_stack.push(ParseContext::class_definition(class_name, class_type == "data"));
    }
    
    // Create appropriate semantic token
    SemanticTokenType type = SemanticTokenType::DataClass;
    if (class_type == "functional") {
        type = SemanticTokenType::FunctionalClass;
    } else if (class_type == "danger") {
        type = SemanticTokenType::DangerClass;
    }
    
    SemanticToken token(type, name_token.line, name_token.column);
    token.set_class_name(class_name);
    token.set_attribute("class_type", class_type);
    return token;
}

SemanticToken SemanticTranslator::resolve_type_expression() {
    // This is a complex multi-token resolution for type expressions
    // For now, return a placeholder
    const RawToken& token = current_raw_token();
    advance_raw_token();
    
    return SemanticToken::placeholder("type_expression", "Type expression parsing not fully implemented", 
                                     token.line, token.column);
}

SemanticToken SemanticTranslator::resolve_defer_statement() {
    // This should be handled by resolve_defer_keyword
    const RawToken& token = current_raw_token();
    advance_raw_token();
    
    return SemanticToken::placeholder("defer_statement", "Defer statement parsing", 
                                     token.line, token.column);
}

SemanticToken SemanticTranslator::handle_identifier() {
    const RawToken& token = current_raw_token();
    advance_raw_token();
    
    return SemanticToken::identifier(token.value, token.line, token.column);
}

SemanticToken SemanticTranslator::handle_literal() {
    const RawToken& token = current_raw_token();
    advance_raw_token();
    
    // Determine literal type based on content
    std::string literal_type = "unknown";
    if (token.value == "true" || token.value == "false") {
        literal_type = "boolean";
    } else if (token.value.front() == '"' || token.value.front() == '\'') {
        literal_type = "string";
    } else if (std::isdigit(token.value.front()) || token.value.front() == '.') {
        literal_type = "number";
    }
    
    return SemanticToken::literal(token.value, literal_type, token.line, token.column);
}

SemanticToken SemanticTranslator::handle_symbol() {
    const RawToken& token = current_raw_token();
    advance_raw_token();
    
    // Handle context changes for block punctuation
    if (token.value == "{") {
        // Entering a block - context should already be set by the preceding construct
    } else if (token.value == "}") {
        exit_context_on_block_end();
    }
    
    // Determine semantic type based on symbol characteristics
    // For now, use the original logic: operators get Operator type, structural punctuation gets Punctuation
    bool is_structural = (token.value == "{" || token.value == "}" || token.value == "(" || 
                         token.value == ")" || token.value == "[" || token.value == "]" || 
                         token.value == ";" || token.value == "," || token.value == ":");
    
    if (is_structural) {
        SemanticToken semantic_token(SemanticTokenType::Punctuation, token.line, token.column);
        semantic_token.set_attribute("punctuation", token.value);
        semantic_token.raw_value = token.value;
        return semantic_token;
    } else {
        SemanticToken semantic_token(SemanticTokenType::Operator, token.line, token.column);
        semantic_token.set_attribute("operator", token.value);
        semantic_token.raw_value = token.value;
        return semantic_token;
    }
}

SemanticToken SemanticTranslator::handle_comment() {
    const RawToken& token = current_raw_token();
    advance_raw_token();
    
    SemanticToken semantic_token(SemanticTokenType::Comment, token.line, token.column);
    semantic_token.set_attribute("comment", token.value);
    semantic_token.raw_value = token.value;
    return semantic_token;
}

void SemanticTranslator::update_context_from_token(const RawToken& token) {
    // Handle special keywords that modify parsing context
    if (token.is_keyword("functional")) {
        // Next class declaration should be functional
        context_stack.push(ParseContext(ParseContextType::TopLevel, {{"class_type", "functional"}}));
    } else if (token.is_keyword("danger")) {
        // Next class declaration should be danger
        context_stack.push(ParseContext(ParseContextType::TopLevel, {{"class_type", "danger"}}));
    } else if (token.is_operator("<")) {
        // Entering type expression context
        context_stack.push(ParseContext::type_expression());
    } else if (token.is_operator(">")) {
        // Exiting type expression context
        if (context_stack.current() && context_stack.current()->type == ParseContextType::TypeExpression) {
            context_stack.pop();
        }
    }
}

void SemanticTranslator::enter_context_from_semantic_token(const SemanticToken&) {
    // Some semantic tokens don't need additional context changes
    // as they were already handled during translation
}

void SemanticTranslator::exit_context_on_block_end() {
    // Pop context when exiting a block
    if (context_stack.depth() > 1) { // Keep at least top-level context
        context_stack.pop();
    }
}

std::vector<std::string> SemanticTranslator::parse_field_list() {
    std::vector<std::string> fields;
    
    if (!current_raw_token().is_punctuation("{")) {
        return fields; // No field list
    }
    
    advance_raw_token(); // consume '{'
    
    while (!is_at_end() && !current_raw_token().is_punctuation("}")) {
        if (current_raw_token().is_identifier()) {
            fields.push_back(current_raw_token().value);
            advance_raw_token();
            
            // Skip optional comma
            if (current_raw_token().is_punctuation(",")) {
                advance_raw_token();
            }
        } else {
            advance_raw_token(); // Skip unexpected token
        }
    }
    
    if (current_raw_token().is_punctuation("}")) {
        advance_raw_token(); // consume '}'
    }
    
    return fields;
}

std::vector<std::string> SemanticTranslator::parse_union_variants() {
    std::vector<std::string> variants;
    
    if (!current_raw_token().is_punctuation("{")) {
        return variants; // No variants list
    }
    
    advance_raw_token(); // consume '{'
    
    while (!is_at_end() && !current_raw_token().is_punctuation("}")) {
        if (current_raw_token().is_identifier()) {
            variants.push_back(current_raw_token().value);
            advance_raw_token();
            
            // Skip optional parameters/fields
            if (current_raw_token().is_punctuation("(") || current_raw_token().is_punctuation("{")) {
                // Skip to matching closing punctuation
                int depth = 1;
                char open_char = current_raw_token().value[0];
                char close_char = (open_char == '(') ? ')' : '}';
                advance_raw_token();
                
                while (!is_at_end() && depth > 0) {
                    if (current_raw_token().value[0] == open_char) depth++;
                    else if (current_raw_token().value[0] == close_char) depth--;
                    advance_raw_token();
                }
            }
            
            // Skip optional comma
            if (current_raw_token().is_punctuation(",")) {
                advance_raw_token();
            }
        } else {
            advance_raw_token(); // Skip unexpected token
        }
    }
    
    if (current_raw_token().is_punctuation("}")) {
        advance_raw_token(); // consume '}'
    }
    
    return variants;
}

std::string SemanticTranslator::parse_function_call() {
    std::stringstream ss;
    
    // Parse until semicolon or end of statement
    while (!is_at_end() && !current_raw_token().is_punctuation(";")) {
        ss << current_raw_token().value;
        if (!is_at_end()) {
            const RawToken& next = peek_raw_token();
            if (!next.is_punctuation(";") && !next.is_punctuation(")") && !next.is_punctuation("}")) {
                ss << " ";
            }
        }
        advance_raw_token();
    }
    
    return ss.str();
}

std::string SemanticTranslator::parse_expression_until(const std::string& terminator) {
    std::stringstream ss;
    
    while (!is_at_end() && current_raw_token().value != terminator) {
        ss << current_raw_token().value;
        advance_raw_token();
        if (!is_at_end() && current_raw_token().value != terminator) {
            ss << " ";
        }
    }
    
    return ss.str();
}

const RawToken& SemanticTranslator::current_raw_token() const {
    return raw_tokens.current();
}

const RawToken& SemanticTranslator::peek_raw_token(size_t offset) const {
    return raw_tokens.peek(offset);
}

void SemanticTranslator::advance_raw_token() {
    raw_tokens.advance();
}

bool SemanticTranslator::is_at_end() const {
    return raw_tokens.is_at_end();
}

void SemanticTranslator::error(const std::string& message) {
    const RawToken& token = current_raw_token();
    error_at_token(message, token);
}

void SemanticTranslator::error_at_token(const std::string& message, const RawToken& token) {
    errors.emplace_back(message, token.line, token.column, context_stack.get_context_path_string());
}

} // namespace cprime