#pragma once

#include "ast.h"
#include "../layer1/raw_token.h"
#include "../layer1/context_stack.h"
#include "../common/tokens.h"
#include "symbol_table.h"
#include <memory>
#include <vector>
#include <stack>
#include <optional>

namespace cprime {

/**
 * AST Builder - Layer 3 of the compiler pipeline.
 * Pure construction component that builds AST from context-enriched tokens.
 * Populates symbol table during construction but performs no validation.
 */
class ASTBuilder {
public:
    ASTBuilder();
    
    /**
     * Build AST from contextual token stream.
     * This is the main entry point for AST construction.
     */
    std::shared_ptr<ast::CompilationUnit> build(const ContextualTokenStream& tokens);
    
    /**
     * Get the populated symbol table after building.
     */
    const SymbolTable& get_symbol_table() const { return symbol_table; }
    SymbolTable& get_symbol_table() { return symbol_table; }
    
    /**
     * Get any parsing errors encountered during AST construction.
     */
    struct ParseError {
        std::string message;
        ast::SourceLocation location;
        std::string context;
    };
    
    const std::vector<ParseError>& get_errors() const { return errors; }
    bool has_errors() const { return !errors.empty(); }
    
private:
    // Internal state
    ContextualTokenStream tokens;
    size_t current_position;
    SymbolTable symbol_table;
    SymbolTableBuilder symbol_builder;
    std::vector<ParseError> errors;
    
    // Token navigation
    const ContextualToken& current() const;
    const ContextualToken& peek(size_t offset = 1) const;
    void advance();
    bool is_at_end() const;
    
    // Token matching
    bool check(RawTokenType type) const;
    bool check_keyword(const std::string& keyword) const;
    bool check_operator(const std::string& op) const;
    bool check_punctuation(const std::string& punct) const;
    bool match(RawTokenType type);
    bool match_keyword(const std::string& keyword);
    bool match_operator(const std::string& op);
    bool match_punctuation(const std::string& punct);
    
    // Error handling
    void error(const std::string& message);
    void error_at(const std::string& message, const ast::SourceLocation& location);
    
    // Source location helpers
    ast::SourceLocation get_current_location() const;
    ast::SourceLocation combine_locations(const ast::SourceLocation& start, 
                                         const ast::SourceLocation& end) const;
    
    // ========================================================================
    // Top-level parsing
    // ========================================================================
    
    std::shared_ptr<ast::CompilationUnit> parse_compilation_unit();
    ast::DeclPtr parse_top_level_declaration();
    
    // ========================================================================
    // Declaration parsing
    // ========================================================================
    
    ast::DeclPtr parse_class_declaration();
    ast::DeclPtr parse_struct_declaration();
    ast::DeclPtr parse_union_declaration();
    ast::DeclPtr parse_interface_declaration();
    ast::DeclPtr parse_function_declaration();
    ast::DeclPtr parse_variable_declaration();
    
    // Class-specific parsing
    ast::ClassDecl::Kind parse_class_kind();
    std::vector<ast::AccessRight> parse_access_rights();
    ast::AccessRight parse_single_access_right();
    ast::DeclList parse_class_members();
    ast::DeclPtr parse_class_member();
    
    // Union-specific parsing
    std::vector<ast::UnionVariant> parse_union_variants();
    ast::UnionVariant parse_single_variant();
    
    // Function-specific parsing
    std::vector<ast::Parameter> parse_parameter_list();
    ast::Parameter parse_parameter();
    ast::StmtPtr parse_function_body();
    
    // ========================================================================
    // Type parsing
    // ========================================================================
    
    ast::TypePtr parse_type();
    ast::TypePtr parse_base_type();
    ast::TypePtr parse_pointer_type(ast::TypePtr base);
    ast::TypePtr parse_reference_type(ast::TypePtr base);
    ast::TypePtr parse_array_type(ast::TypePtr base);
    ast::TypePtr parse_generic_type(ast::TypePtr base);
    ast::TypePtr parse_runtime_type();
    
    // ========================================================================
    // Statement parsing
    // ========================================================================
    
    ast::StmtPtr parse_statement();
    ast::StmtPtr parse_block_statement();
    ast::StmtPtr parse_if_statement();
    ast::StmtPtr parse_while_statement();
    ast::StmtPtr parse_for_statement();
    ast::StmtPtr parse_return_statement();
    ast::StmtPtr parse_defer_statement();
    ast::StmtPtr parse_expression_statement();
    ast::StmtPtr parse_variable_declaration_statement();
    ast::StmtPtr parse_throw_statement();
    ast::StmtPtr parse_try_statement();
    
    // ========================================================================
    // Expression parsing (precedence climbing)
    // ========================================================================
    
    ast::ExprPtr parse_expression();
    ast::ExprPtr parse_assignment_expression();
    ast::ExprPtr parse_ternary_expression();
    ast::ExprPtr parse_logical_or_expression();
    ast::ExprPtr parse_logical_and_expression();
    ast::ExprPtr parse_bitwise_or_expression();
    ast::ExprPtr parse_bitwise_xor_expression();
    ast::ExprPtr parse_bitwise_and_expression();
    ast::ExprPtr parse_equality_expression();
    ast::ExprPtr parse_relational_expression();
    ast::ExprPtr parse_shift_expression();
    ast::ExprPtr parse_additive_expression();
    ast::ExprPtr parse_multiplicative_expression();
    ast::ExprPtr parse_unary_expression();
    ast::ExprPtr parse_postfix_expression();
    ast::ExprPtr parse_primary_expression();
    
    // Expression helpers
    ast::ExprPtr parse_identifier_expression();
    ast::ExprPtr parse_literal_expression();
    ast::ExprPtr parse_parenthesized_expression();
    ast::ExprPtr parse_call_expression(ast::ExprPtr callee);
    ast::ExprPtr parse_member_expression(ast::ExprPtr object);
    ast::ExprList parse_argument_list();
    ast::ExprPtr parse_new_expression();
    ast::ExprPtr parse_delete_expression();
    ast::ExprPtr parse_sizeof_expression();
    ast::ExprPtr parse_alignof_expression();
    ast::ExprPtr parse_cast_expression();
    
    // ========================================================================
    // Context-aware parsing helpers
    // ========================================================================
    
    bool is_type_specifier() const;
    bool is_class_modifier() const;
    bool is_access_specifier() const;
    bool is_statement_keyword() const;
    bool is_expression_start() const;
    bool is_function_declaration() const;
    
    // Use contextual token information
    std::string get_context_resolution() const;
    bool has_context_attribute(const std::string& key) const;
    std::string get_context_attribute(const std::string& key) const;
    
    // ========================================================================
    // Operator precedence helpers
    // ========================================================================
    
    ast::BinaryExpr::Operator token_to_binary_operator(const std::string& op) const;
    ast::UnaryExpr::Operator token_to_unary_operator(const std::string& op) const;
    int get_operator_precedence(const std::string& op) const;
    bool is_right_associative(const std::string& op) const;
};

/**
 * Parallel AST Builder - builds AST sections in parallel.
 * Leverages the self-contained nature of context-enriched tokens.
 */
class ParallelASTBuilder {
public:
    ParallelASTBuilder(size_t num_threads = 0); // 0 = auto-detect
    
    /**
     * Build AST from contextual tokens using parallel processing.
     * Functions and classes are built in parallel, then assembled.
     */
    std::shared_ptr<ast::CompilationUnit> build_parallel(const ContextualTokenStream& tokens);
    
    /**
     * Get the merged symbol table from all parallel builders.
     */
    const SymbolTable& get_symbol_table() const { return merged_symbol_table; }
    
private:
    size_t num_threads;
    SymbolTable merged_symbol_table;
    
    // Parallel building strategies
    struct BuildTask {
        size_t start_token;
        size_t end_token;
        ParseContextType context_type;
        std::string context_name;
    };
    
    std::vector<BuildTask> identify_parallel_tasks(const ContextualTokenStream& tokens);
    ast::DeclPtr build_task(const ContextualTokenStream& tokens, const BuildTask& task);
    void merge_symbol_tables(const std::vector<SymbolTable>& tables);
};

} // namespace cprime