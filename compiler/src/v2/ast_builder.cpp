#include "ast_builder.h"
#include <sstream>
#include <stdexcept>

namespace cprime::v2 {

// ============================================================================
// ASTBuilder Implementation
// ============================================================================

ASTBuilder::ASTBuilder()
    : tokens(std::vector<ContextualToken>{}),
      current_position(0),
      symbol_builder(symbol_table) {
}

std::shared_ptr<ast::CompilationUnit> ASTBuilder::build(const ContextualTokenStream& token_stream) {
    // Initialize state
    this->tokens = token_stream;
    current_position = 0;
    errors.clear();
    
    // Parse the compilation unit
    return parse_compilation_unit();
}

// ============================================================================
// Token Navigation
// ============================================================================

const ContextualToken& ASTBuilder::current() const {
    return tokens.current();
}

const ContextualToken& ASTBuilder::peek(size_t offset) const {
    return tokens.peek(offset);
}

void ASTBuilder::advance() {
    if (!is_at_end()) {
        tokens.advance();
        current_position++;
    }
}

bool ASTBuilder::is_at_end() const {
    return tokens.is_at_end() || current().type() == RawTokenType::EOF_TOKEN;
}

// ============================================================================
// Token Matching
// ============================================================================

bool ASTBuilder::check(RawTokenType type) const {
    if (is_at_end()) return false;
    return current().type() == type;
}

bool ASTBuilder::check_keyword(const std::string& keyword) const {
    if (is_at_end()) return false;
    return current().is_keyword(keyword);
}

bool ASTBuilder::check_operator(const std::string& op) const {
    if (is_at_end()) return false;
    return current().is_operator(op);
}

bool ASTBuilder::check_punctuation(const std::string& punct) const {
    if (is_at_end()) return false;
    return current().is_punctuation(punct);
}

bool ASTBuilder::match(RawTokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool ASTBuilder::match_keyword(const std::string& keyword) {
    if (check_keyword(keyword)) {
        advance();
        return true;
    }
    return false;
}

bool ASTBuilder::match_operator(const std::string& op) {
    if (check_operator(op)) {
        advance();
        return true;
    }
    return false;
}

bool ASTBuilder::match_punctuation(const std::string& punct) {
    if (check_punctuation(punct)) {
        advance();
        return true;
    }
    return false;
}

// ============================================================================
// Error Handling
// ============================================================================

void ASTBuilder::error(const std::string& message) {
    error_at(message, get_current_location());
}

void ASTBuilder::error_at(const std::string& message, const ast::SourceLocation& location) {
    ParseError err;
    err.message = message;
    err.location = location;
    err.context = current().context_resolution;
    errors.push_back(err);
}

// ============================================================================
// Source Location Helpers
// ============================================================================

ast::SourceLocation ASTBuilder::get_current_location() const {
    if (is_at_end()) {
        return ast::SourceLocation(0, 0, 0, 0);
    }
    const auto& token = current();
    return ast::SourceLocation(token.line(), token.column(), token.position(), token.position() + token.value().length());
}

ast::SourceLocation ASTBuilder::combine_locations(const ast::SourceLocation& start, 
                                                  const ast::SourceLocation& end) const {
    return ast::SourceLocation(start.line, start.column, start.start_pos, end.end_pos);
}

// ============================================================================
// Top-level Parsing
// ============================================================================

std::shared_ptr<ast::CompilationUnit> ASTBuilder::parse_compilation_unit() {
    ast::DeclList declarations;
    auto start_loc = get_current_location();
    
    // Skip any leading whitespace/comments
    while (!is_at_end() && (check(RawTokenType::WHITESPACE) || check(RawTokenType::COMMENT))) {
        advance();
    }
    
    // Parse all top-level declarations
    while (!is_at_end()) {
        // Skip whitespace and comments
        while (!is_at_end() && (check(RawTokenType::WHITESPACE) || check(RawTokenType::COMMENT))) {
            advance();
        }
        
        if (is_at_end()) break;
        
        auto decl = parse_top_level_declaration();
        if (decl) {
            declarations.push_back(decl);
        } else {
            // Skip to next declaration on error
            advance();
        }
    }
    
    auto end_loc = get_current_location();
    return std::make_shared<ast::CompilationUnit>(declarations, combine_locations(start_loc, end_loc));
}

ast::DeclPtr ASTBuilder::parse_top_level_declaration() {
    // Use context resolution from enriched tokens
    auto context_resolution = current().context_resolution;
    
    // Check for class modifiers (functional, danger)
    if (check_keyword("functional") || check_keyword("danger")) {
        return parse_class_declaration();
    }
    
    // Check for type declarations
    if (check_keyword("class") || check_keyword("plex")) {
        return parse_class_declaration();
    }
    
    if (check_keyword("struct")) {
        return parse_struct_declaration();
    }
    
    if (check_keyword("union")) {
        return parse_union_declaration();
    }
    
    if (check_keyword("interface")) {
        return parse_interface_declaration();
    }
    
    // Check for function declarations
    if (context_resolution == "FunctionDeclaration" || is_function_declaration()) {
        return parse_function_declaration();
    }
    
    // Check for variable declarations
    if (check_keyword("let") || check_keyword("const") || is_type_specifier()) {
        return parse_variable_declaration();
    }
    
    error("Expected declaration at top level");
    return nullptr;
}

// ============================================================================
// Declaration Parsing
// ============================================================================

ast::DeclPtr ASTBuilder::parse_class_declaration() {
    auto start_loc = get_current_location();
    
    // Parse class kind
    auto kind = parse_class_kind();
    
    // Expect class or plex keyword
    if (!match_keyword("class") && !match_keyword("plex")) {
        error("Expected 'class' or 'plex' keyword");
        return nullptr;
    }
    
    // Parse class name
    if (!check(RawTokenType::IDENTIFIER)) {
        error("Expected class name");
        return nullptr;
    }
    
    std::string class_name = current().value();
    advance();
    
    // Enter class scope
    SymbolTableBuilder::ScopeGuard scope_guard(symbol_table, Scope::Kind::Class, class_name);
    
    // Expect opening brace
    if (!match_punctuation("{")) {
        error("Expected '{' after class name");
        return nullptr;
    }
    
    // Parse members and access rights
    ast::DeclList members;
    std::vector<ast::AccessRight> access_rights;
    
    while (!check_punctuation("}") && !is_at_end()) {
        // Skip whitespace
        while (match(RawTokenType::WHITESPACE) || match(RawTokenType::COMMENT)) {}
        
        // Check for access rights
        if (check_keyword("runtime") || check_keyword("exposes")) {
            auto access_right = parse_single_access_right();
            access_rights.push_back(access_right);
        }
        // Parse member declaration
        else if (!check_punctuation("}")) {
            auto member = parse_class_member();
            if (member) {
                members.push_back(member);
            }
        }
    }
    
    // Expect closing brace
    if (!match_punctuation("}")) {
        error("Expected '}' after class body");
        return nullptr;
    }
    
    auto end_loc = get_current_location();
    auto class_decl = std::make_shared<ast::ClassDecl>(
        class_name, kind, members, access_rights, 
        combine_locations(start_loc, end_loc)
    );
    
    // Register in symbol table
    symbol_builder.process_class_declaration(*class_decl);
    
    return class_decl;
}

ast::ClassDecl::Kind ASTBuilder::parse_class_kind() {
    if (match_keyword("functional")) {
        return ast::ClassDecl::Kind::Functional;
    }
    if (match_keyword("danger")) {
        return ast::ClassDecl::Kind::Danger;
    }
    return ast::ClassDecl::Kind::Data;
}

ast::AccessRight ASTBuilder::parse_single_access_right() {
    bool is_runtime = false;
    
    // Check for runtime modifier
    if (match_keyword("runtime")) {
        is_runtime = true;
    }
    
    // Expect 'exposes' keyword
    if (!match_keyword("exposes")) {
        error("Expected 'exposes' keyword");
        return ast::AccessRight("", {}, false);
    }
    
    // Parse access right name
    if (!check(RawTokenType::IDENTIFIER)) {
        error("Expected access right name");
        return ast::AccessRight("", {}, false);
    }
    
    std::string right_name = current().value();
    advance();
    
    // Parse granted fields
    std::vector<std::string> fields;
    if (match_punctuation("{")) {
        while (!check_punctuation("}") && !is_at_end()) {
            if (check(RawTokenType::IDENTIFIER)) {
                fields.push_back(current().value());
                advance();
                
                // Optional comma
                match_punctuation(",");
            } else {
                advance(); // Skip unexpected token
            }
        }
        
        if (!match_punctuation("}")) {
            error("Expected '}' after field list");
        }
    }
    
    return ast::AccessRight(right_name, fields, is_runtime);
}

ast::DeclPtr ASTBuilder::parse_class_member() {
    // Could be a field or method
    if (is_function_declaration()) {
        return parse_function_declaration();
    } else {
        return parse_variable_declaration();
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

bool ASTBuilder::is_type_specifier() const {
    if (is_at_end()) return false;
    
    // Check for built-in types
    const std::string& value = current().value();
    if (value == "int" || value == "bool" || value == "float" || value == "double" ||
        value == "char" || value == "void" || value == "auto") {
        return true;
    }
    
    // Check for type modifiers
    if (value == "const" || value == "volatile" || value == "signed" || 
        value == "unsigned" || value == "short" || value == "long") {
        return true;
    }
    
    // Could be a user-defined type (identifier)
    if (current().type() == RawTokenType::IDENTIFIER) {
        // Look ahead to see if it's followed by variable name
        if (!is_at_end() && peek().type() == RawTokenType::IDENTIFIER) {
            return true;
        }
        // Or pointer/reference
        if (!is_at_end() && (peek().is_operator("*") || peek().is_operator("&"))) {
            return true;
        }
    }
    
    return false;
}

bool ASTBuilder::is_function_declaration() const {
    // Simple heuristic: identifier followed by '('
    if (current().type() == RawTokenType::IDENTIFIER) {
        return !is_at_end() && peek().is_punctuation("(");
    }
    
    // Or type specifier followed by identifier and '('
    if (is_type_specifier()) {
        size_t lookahead = 1;
        // Skip type tokens
        while (lookahead < 5 && !is_at_end()) {
            const auto& token = peek(lookahead);
            if (token.type() == RawTokenType::IDENTIFIER) {
                // Check if next is '('
                return peek(lookahead + 1).is_punctuation("(");
            }
            lookahead++;
        }
    }
    
    return false;
}

std::string ASTBuilder::get_context_resolution() const {
    if (is_at_end()) return "";
    return current().context_resolution;
}

bool ASTBuilder::has_context_attribute(const std::string& key) const {
    if (is_at_end()) return false;
    return current().has_attribute(key);
}

std::string ASTBuilder::get_context_attribute(const std::string& key) const {
    if (is_at_end()) return "";
    return current().get_attribute(key);
}

// ============================================================================
// Stub implementations for remaining methods
// ============================================================================

ast::DeclPtr ASTBuilder::parse_struct_declaration() {
    // TODO: Implement
    error("Struct declarations not yet implemented");
    return nullptr;
}

ast::DeclPtr ASTBuilder::parse_union_declaration() {
    // TODO: Implement
    error("Union declarations not yet implemented");
    return nullptr;
}

ast::DeclPtr ASTBuilder::parse_interface_declaration() {
    // TODO: Implement
    error("Interface declarations not yet implemented");
    return nullptr;
}

ast::DeclPtr ASTBuilder::parse_function_declaration() {
    // TODO: Implement
    error("Function declarations not yet implemented");
    return nullptr;
}

ast::DeclPtr ASTBuilder::parse_variable_declaration() {
    // TODO: Implement
    error("Variable declarations not yet implemented");
    return nullptr;
}

ast::DeclList ASTBuilder::parse_class_members() {
    ast::DeclList members;
    // TODO: Implement
    return members;
}

} // namespace cprime::v2