#include "parser.h"
#include <stdexcept>
#include <sstream>
#include <string>

namespace cprime {

Parser::Parser(std::vector<Token> tokens) 
    : tokens(std::move(tokens)), pos(0) {}

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();
    
    while (!is_at_end()) {
        if (check(TokenType::CLASS)) {
            program->classes.push_back(parse_class());
        } else if (is_type_keyword()) {
            program->functions.push_back(parse_function());
        } else {
            error("Expected class or function definition");
        }
    }
    
    return program;
}

std::unique_ptr<Function> Parser::parse_function() {
    // Parse return type (int, void, bool, auto)
    Type return_type = parse_type();
    
    // Parse function name
    consume(TokenType::IDENTIFIER, "Expected function name");
    std::string name = previous().value;
    
    auto func = std::make_unique<Function>(return_type, name);
    
    consume(TokenType::LPAREN, "Expected '(' after function name");
    consume(TokenType::RPAREN, "Expected ')' after '(' (no parameters supported yet)");
    
    func->body = parse_block();
    return func;
}

std::unique_ptr<Block> Parser::parse_block() {
    consume(TokenType::LBRACE, "Expected '{'");
    
    // Enter new scope
    symbol_table.enter_scope();
    
    auto block = std::make_unique<Block>();
    
    while (!check(TokenType::RBRACE) && !is_at_end()) {
        block->statements.push_back(parse_statement());
    }
    
    consume(TokenType::RBRACE, "Expected '}'");
    
    // Exit scope
    symbol_table.exit_scope();
    
    return block;
}

std::unique_ptr<Statement> Parser::parse_statement() {
    if (match(TokenType::IF)) {
        return parse_if_statement();
    } else if (match(TokenType::WHILE)) {
        return parse_while_loop();
    } else if (match(TokenType::FOR)) {
        return parse_for_loop();
    } else if (match(TokenType::LBRACE)) {
        // Standalone block
        pos--; // Back up to consume the brace in parse_block
        return parse_block();
    } else if (is_type_keyword()) {
        return parse_variable_declaration();
    } else if (check(TokenType::IDENTIFIER)) {
        // Could be assignment or function call
        size_t saved_pos = pos;
        advance(); // consume identifier
        
        if (check(TokenType::ASSIGN)) {
            // It's an assignment
            pos = saved_pos; // restore position
            return parse_assignment();
        } else {
            // It's a function call
            pos = saved_pos; // restore position
            auto call = parse_function_call();
            consume(TokenType::SEMICOLON, "Expected ';' after statement");
            return call;
        }
    } else {
        error("Expected statement");
        return nullptr;
    }
}

std::unique_ptr<VariableDeclaration> Parser::parse_variable_declaration() {
    // Check for complex type declarations (pointers, references)
    size_t saved_pos = pos;
    
    // Try to detect pointer type: type*
    if ((is_type_keyword() || check(TokenType::IDENTIFIER)) && peek_for_pointer()) {
        auto pointer_type = parse_pointer_type();
        
        consume(TokenType::IDENTIFIER, "Expected variable name");
        std::string name = previous().value;
        
        consume(TokenType::ASSIGN, "Expected '=' after pointer variable name");
        auto initializer = parse_expression();
        consume(TokenType::SEMICOLON, "Expected ';' after pointer variable declaration");
        
        // Add to symbol table (TODO: extend symbol table for pointers)
        // For now, just declare as POINTER type
        if (!symbol_table.declare_variable(name, Type::POINTER)) {
            error("Variable '" + name + "' already declared in this scope");
        }
        
        return std::make_unique<VariableDeclaration>(name, std::move(pointer_type), std::move(initializer));
    }
    
    // Try to detect reference type: [const] type& or type&&
    pos = saved_pos;
    if ((check(TokenType::CONST) || is_type_keyword() || check(TokenType::IDENTIFIER)) && peek_for_reference()) {
        auto reference_type = parse_reference_type();
        
        consume(TokenType::IDENTIFIER, "Expected variable name");
        std::string name = previous().value;
        
        consume(TokenType::ASSIGN, "Expected '=' after reference variable name");
        auto initializer = parse_expression();
        consume(TokenType::SEMICOLON, "Expected ';' after reference variable declaration");
        
        // Add to symbol table (TODO: extend symbol table for references)
        Type ref_type = reference_type->is_rvalue ? Type::RVALUE_REFERENCE : Type::REFERENCE;
        if (!symbol_table.declare_variable(name, ref_type)) {
            error("Variable '" + name + "' already declared in this scope");
        }
        
        return std::make_unique<VariableDeclaration>(name, std::move(reference_type), std::move(initializer));
    }
    
    // Fall back to simple type parsing
    pos = saved_pos;
    Type type = parse_type();
    
    consume(TokenType::IDENTIFIER, "Expected variable name");
    std::string name = previous().value;
    
    consume(TokenType::ASSIGN, "Expected '=' after variable name");
    auto initializer = parse_expression();
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    
    // Type deduction for auto
    if (type == Type::AUTO) {
        type = symbol_table.deduce_type(*initializer);
    }
    
    // Add to symbol table
    if (!symbol_table.declare_variable(name, type)) {
        error("Variable '" + name + "' already declared in this scope");
    }
    
    return std::make_unique<VariableDeclaration>(type, name, std::move(initializer));
}

std::unique_ptr<Assignment> Parser::parse_assignment() {
    consume(TokenType::IDENTIFIER, "Expected variable name");
    std::string name = previous().value;
    
    consume(TokenType::ASSIGN, "Expected '='");
    auto value = parse_expression();
    consume(TokenType::SEMICOLON, "Expected ';' after assignment");
    
    // Check if variable exists
    if (!symbol_table.lookup_variable(name)) {
        error("Undefined variable '" + name + "'");
    }
    
    symbol_table.assign_variable(name);
    
    return std::make_unique<Assignment>(name, std::move(value));
}

Type Parser::parse_type() {
    // This method now only handles simple built-in types
    // For complex types (pointers, references), use specific parsing methods
    return parse_base_type();
}

Type Parser::parse_base_type() {
    if (match(TokenType::AUTO)) {
        return Type::AUTO;
    } else if (match(TokenType::INT)) {
        return Type::INT;
    } else if (match(TokenType::BOOL)) {
        return Type::BOOL;
    } else if (match(TokenType::VOID)) {
        return Type::VOID;
    } else {
        error("Expected type specifier");
        return Type::VOID;
    }
}

std::unique_ptr<PointerType> Parser::parse_pointer_type() {
    // Parse base type first
    if (is_type_keyword()) {
        Type base_type = parse_base_type();
        consume(TokenType::MULTIPLY, "Expected '*' after base type for pointer");
        return std::make_unique<PointerType>(base_type);
    } else if (check(TokenType::IDENTIFIER)) {
        // Custom type pointer
        auto custom_type = parse_custom_type();
        consume(TokenType::MULTIPLY, "Expected '*' after custom type for pointer");
        return std::make_unique<PointerType>(std::move(custom_type));
    } else {
        error("Expected type before '*' in pointer declaration");
        return std::make_unique<PointerType>(Type::VOID);
    }
}

std::unique_ptr<ReferenceType> Parser::parse_reference_type() {
    bool is_const = false;
    
    // Check for const keyword
    if (match(TokenType::CONST)) {
        is_const = true;
    }
    
    // Parse base type
    if (is_type_keyword()) {
        Type base_type = parse_base_type();
        
        // Check for reference type
        if (match(TokenType::AMPERSAND)) {
            if (match(TokenType::AMPERSAND)) {
                // Rvalue reference (&&)
                return std::make_unique<ReferenceType>(base_type, is_const, true);
            } else {
                // Regular reference (&)
                return std::make_unique<ReferenceType>(base_type, is_const, false);
            }
        } else {
            error("Expected '&' or '&&' after type for reference");
            return std::make_unique<ReferenceType>(Type::VOID, is_const, false);
        }
    } else if (check(TokenType::IDENTIFIER)) {
        // Custom type reference
        auto custom_type = parse_custom_type();
        
        if (match(TokenType::AMPERSAND)) {
            if (match(TokenType::AMPERSAND)) {
                // Rvalue reference (&&)
                return std::make_unique<ReferenceType>(std::move(custom_type), is_const, true);
            } else {
                // Regular reference (&)
                return std::make_unique<ReferenceType>(std::move(custom_type), is_const, false);
            }
        } else {
            error("Expected '&' or '&&' after custom type for reference");
            return std::make_unique<ReferenceType>(std::move(custom_type), is_const, false);
        }
    } else {
        error("Expected type before '&' in reference declaration");
        return std::make_unique<ReferenceType>(Type::VOID, is_const, false);
    }
}

bool Parser::is_type_keyword() const {
    return check(TokenType::AUTO) || check(TokenType::INT) || check(TokenType::BOOL) || check(TokenType::VOID);
}

std::unique_ptr<IfStatement> Parser::parse_if_statement() {
    consume(TokenType::LPAREN, "Expected '(' after 'if'");
    auto condition = parse_expression();
    consume(TokenType::RPAREN, "Expected ')' after condition");
    
    auto then_block = parse_block();
    
    std::unique_ptr<Block> else_block = nullptr;
    if (match(TokenType::ELSE)) {
        else_block = parse_block();
    }
    
    return std::make_unique<IfStatement>(std::move(condition), std::move(then_block), std::move(else_block));
}

std::unique_ptr<WhileLoop> Parser::parse_while_loop() {
    consume(TokenType::LPAREN, "Expected '(' after 'while'");
    auto condition = parse_expression();
    consume(TokenType::RPAREN, "Expected ')' after condition");
    
    auto body = parse_block();
    
    return std::make_unique<WhileLoop>(std::move(condition), std::move(body));
}

std::unique_ptr<ForLoop> Parser::parse_for_loop() {
    consume(TokenType::LPAREN, "Expected '(' after 'for'");
    consume(TokenType::IDENTIFIER, "Expected variable name");
    std::string variable = previous().value;
    
    consume(TokenType::IN, "Expected 'in' after variable");
    auto iterable = parse_expression();
    consume(TokenType::RPAREN, "Expected ')' after iterable");
    
    // Enter new scope for the for loop
    symbol_table.enter_scope();
    
    // For loops automatically declare their iteration variable as int
    if (!symbol_table.declare_variable(variable, Type::INT)) {
        error("Cannot declare for loop variable '" + variable + "'");
    }
    
    // Parse body with the loop variable in scope
    consume(TokenType::LBRACE, "Expected '{' after for loop header");
    
    auto block = std::make_unique<Block>();
    while (!check(TokenType::RBRACE) && !is_at_end()) {
        block->statements.push_back(parse_statement());
    }
    
    consume(TokenType::RBRACE, "Expected '}'");
    
    // Exit scope
    symbol_table.exit_scope();
    
    return std::make_unique<ForLoop>(variable, std::move(iterable), std::move(block));
}

std::unique_ptr<Expression> Parser::parse_expression() {
    return parse_logical_or();
}

std::unique_ptr<Expression> Parser::parse_logical_or() {
    // For now, just go to next level (no logical OR yet)
    return parse_logical_and();
}

std::unique_ptr<Expression> Parser::parse_logical_and() {
    // For now, just go to next level (no logical AND yet)
    return parse_equality();
}

std::unique_ptr<Expression> Parser::parse_equality() {
    auto expr = parse_comparison();
    
    while (match(TokenType::EQ) || match(TokenType::NEQ)) {
        std::string op = previous().value;
        auto right = parse_comparison();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parse_comparison() {
    auto expr = parse_term();
    
    while (match(TokenType::LT) || match(TokenType::GT) || 
           match(TokenType::LTEQ) || match(TokenType::GTEQ)) {
        std::string op = previous().value;
        auto right = parse_term();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parse_term() {
    auto expr = parse_factor();
    
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        std::string op = previous().value;
        auto right = parse_factor();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parse_factor() {
    auto expr = parse_primary();
    
    while (match(TokenType::MULTIPLY) || match(TokenType::DIVIDE) || match(TokenType::MODULO)) {
        std::string op = previous().value;
        auto right = parse_primary();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parse_primary() {
    if (match(TokenType::TRUE)) {
        return std::make_unique<BooleanLiteral>(true);
    }
    
    if (match(TokenType::FALSE)) {
        return std::make_unique<BooleanLiteral>(false);
    }
    
    if (match(TokenType::NUMBER)) {
        int value = std::stoi(previous().value);
        return std::make_unique<NumberLiteral>(value);
    }
    
    if (match(TokenType::STRING_LITERAL)) {
        std::string value = previous().value;
        return std::make_unique<StringLiteral>(value);
    }
    
    if (match(TokenType::IDENTIFIER)) {
        std::string name = previous().value;
        // Check if variable exists
        if (!symbol_table.lookup_variable(name)) {
            error("Undefined variable '" + name + "'");
        }
        
        std::unique_ptr<Expression> expr = std::make_unique<VariableReference>(name);
        
        // Handle field access: obj.field
        while (match(TokenType::DOT)) {
            consume(TokenType::IDENTIFIER, "Expected field name after '.'");
            std::string field_name = previous().value;
            expr = std::make_unique<FieldAccess>(std::move(expr), field_name);
        }
        
        return expr;
    }
    
    if (match(TokenType::RANGE)) {
        consume(TokenType::LPAREN, "Expected '(' after 'range'");
        auto limit = parse_expression();
        consume(TokenType::RPAREN, "Expected ')' after range limit");
        return std::make_unique<RangeExpression>(std::move(limit));
    }
    
    if (match(TokenType::LPAREN)) {
        auto expr = parse_expression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    error("Expected expression");
    return nullptr;
}

std::unique_ptr<FunctionCall> Parser::parse_function_call() {
    consume(TokenType::IDENTIFIER, "Expected function name");
    std::string name = previous().value;
    
    auto call = std::make_unique<FunctionCall>(name);
    
    consume(TokenType::LPAREN, "Expected '(' after function name");
    
    // Parse arguments (expressions)
    if (!check(TokenType::RPAREN)) {
        do {
            call->args.push_back(parse_expression());
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RPAREN, "Expected ')' after arguments");
    return call;
}

Token& Parser::current() {
    return tokens[pos];
}

const Token& Parser::current() const {
    return tokens[pos];
}

Token& Parser::previous() {
    return tokens[pos - 1];
}

bool Parser::is_at_end() const {
    return current().type == TokenType::EOF_TOKEN;
}

void Parser::advance() {
    if (!is_at_end()) pos++;
}

bool Parser::check(TokenType type) const {
    if (is_at_end()) return false;
    return current().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

void Parser::consume(TokenType type, const std::string& error_msg) {
    if (check(type)) {
        advance();
        return;
    }
    
    error(error_msg);
}

void Parser::error(const std::string& message) {
    std::ostringstream oss;
    Token& tok = current();
    oss << "Parse error at line " << tok.line << ", column " << tok.column 
        << ": " << message;
    
    if (tok.type != TokenType::EOF_TOKEN) {
        oss << " (found '" << tok.value << "')";
    } else {
        oss << " (reached end of file)";
    }
    
    throw std::runtime_error(oss.str());
}

std::unique_ptr<ClassDefinition> Parser::parse_class() {
    consume(TokenType::CLASS, "Expected 'class'");
    consume(TokenType::IDENTIFIER, "Expected class name");
    std::string class_name = previous().value;
    
    auto class_def = std::make_unique<ClassDefinition>(class_name);
    
    consume(TokenType::LBRACE, "Expected '{' after class name");
    
    while (!check(TokenType::RBRACE) && !is_at_end()) {
        if (is_constructor_declaration()) {
            class_def->constructors.push_back(parse_constructor_declaration(class_name));
        } else {
            class_def->fields.push_back(parse_field_declaration());
        }
    }
    
    consume(TokenType::RBRACE, "Expected '}' after class body");
    consume(TokenType::SEMICOLON, "Expected ';' after class definition");
    
    return class_def;
}

std::unique_ptr<FieldDeclaration> Parser::parse_field_declaration() {
    consume(TokenType::IDENTIFIER, "Expected field name");
    std::string field_name = previous().value;
    
    consume(TokenType::COLON, "Expected ':' after field name");
    
    // Parse type - could be built-in or custom
    if (is_type_keyword()) {
        Type type = parse_type();
        auto field = std::make_unique<FieldDeclaration>(field_name, type);
        consume(TokenType::COMMA, "Expected ',' after field declaration");
        return field;
    } else {
        // Custom type
        auto custom_type = parse_custom_type();
        auto field = std::make_unique<FieldDeclaration>(field_name, std::move(custom_type));
        consume(TokenType::COMMA, "Expected ',' after field declaration");
        return field;
    }
}

std::unique_ptr<ConstructorDeclaration> Parser::parse_constructor_declaration(const std::string& class_name) {
    ConstructorType ctor_type;
    bool is_explicit = false;
    
    // Check for explicit keyword
    if (match(TokenType::EXPLICIT)) {
        is_explicit = true;
    }
    
    // Determine constructor type
    if (match(TokenType::IDENTIFIER) && previous().value == class_name) {
        // Default or custom constructor: ClassName(...)
        consume(TokenType::LPAREN, "Expected '(' after constructor name");
        
        if (check(TokenType::RPAREN)) {
            // Default constructor: ClassName()
            ctor_type = ConstructorType::DEFAULT;
            advance(); // consume ')'
        } else if (check(TokenType::IDENTIFIER) && current().value == "const") {
            // Copy constructor: ClassName(const ClassName& other)
            advance(); // consume 'const'
            consume(TokenType::IDENTIFIER, "Expected class name in copy constructor");
            if (previous().value != class_name) {
                error("Copy constructor parameter must be of same class type");
            }
            // Skip '&' and parameter name for now
            while (!check(TokenType::RPAREN) && !is_at_end()) {
                advance();
            }
            consume(TokenType::RPAREN, "Expected ')' after copy constructor parameters");
            ctor_type = ConstructorType::COPY;
        } else if (check(TokenType::IDENTIFIER) && current().value == class_name) {
            // Move constructor: ClassName(ClassName&& other)
            advance(); // consume class name
            // Skip '&&' and parameter name for now
            while (!check(TokenType::RPAREN) && !is_at_end()) {
                advance();
            }
            consume(TokenType::RPAREN, "Expected ')' after move constructor parameters");
            ctor_type = ConstructorType::MOVE;
        } else {
            error("Unsupported constructor type");
        }
    } else if (match(TokenType::IDENTIFIER) && previous().value == "~" + class_name) {
        // Destructor: ~ClassName()
        consume(TokenType::LPAREN, "Expected '(' after destructor name");
        consume(TokenType::RPAREN, "Expected ')' after destructor");
        ctor_type = ConstructorType::DESTRUCTOR;
    } else {
        error("Expected constructor or destructor declaration");
    }
    
    auto ctor = std::make_unique<ConstructorDeclaration>(ctor_type, false, is_explicit);
    
    // Check for = default
    if (match(TokenType::ASSIGN)) {
        consume(TokenType::DEFAULT, "Expected 'default' after '='");
        ctor->is_default = true;
        consume(TokenType::SEMICOLON, "Expected ';' after '= default'");
    } else {
        // Custom implementation
        ctor->body = parse_block();
    }
    
    return ctor;
}

std::unique_ptr<CustomType> Parser::parse_custom_type() {
    consume(TokenType::IDENTIFIER, "Expected custom type name");
    std::string type_name = previous().value;
    return std::make_unique<CustomType>(type_name);
}

bool Parser::is_constructor_declaration() const {
    // For now, skip constructor parsing - only parse fields
    // TODO: Implement proper constructor detection later
    return false;
}

bool Parser::peek_for_pointer() const {
    // Look ahead to see if we have a pattern like "type *"
    size_t lookahead_pos = pos;
    
    // Skip the base type
    if (tokens[lookahead_pos].type == TokenType::AUTO ||
        tokens[lookahead_pos].type == TokenType::INT ||
        tokens[lookahead_pos].type == TokenType::BOOL ||
        tokens[lookahead_pos].type == TokenType::VOID ||
        tokens[lookahead_pos].type == TokenType::IDENTIFIER) {
        lookahead_pos++;
        
        // Check if next token is '*'
        if (lookahead_pos < tokens.size() && tokens[lookahead_pos].type == TokenType::MULTIPLY) {
            return true;
        }
    }
    
    return false;
}

bool Parser::peek_for_reference() const {
    // Look ahead to see if we have a pattern like "[const] type &" or "type &&"
    size_t lookahead_pos = pos;
    
    // Skip optional 'const'
    if (lookahead_pos < tokens.size() && tokens[lookahead_pos].type == TokenType::CONST) {
        lookahead_pos++;
    }
    
    // Skip the base type
    if (lookahead_pos < tokens.size() && 
        (tokens[lookahead_pos].type == TokenType::AUTO ||
         tokens[lookahead_pos].type == TokenType::INT ||
         tokens[lookahead_pos].type == TokenType::BOOL ||
         tokens[lookahead_pos].type == TokenType::VOID ||
         tokens[lookahead_pos].type == TokenType::IDENTIFIER)) {
        lookahead_pos++;
        
        // Check if next token is '&'
        if (lookahead_pos < tokens.size() && tokens[lookahead_pos].type == TokenType::AMPERSAND) {
            return true;
        }
    }
    
    return false;
}

} // namespace cprime