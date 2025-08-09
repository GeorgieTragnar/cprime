#pragma once

#include "../layer1/raw_token.h"
#include "contextual_token.h"
#include "contextual_token_kind.h"
#include "../layer1/context_stack.h"
#include "../common/string_table.h"
#include <functional>
#include <unordered_map>
#include <memory>

namespace cprime {

/**
 * Semantic translator - Layer 2 of the three-layer architecture.
 * Converts raw tokens into contextual tokens using pure enum transformations.
 * NO STRING OPERATIONS OR OWNERSHIP - only enum mappings and string table indices.
 */
class SemanticTranslator {
public:
    explicit SemanticTranslator(RawTokenStream raw_tokens, StringTable& string_table);
    
    // Main translation method - returns ContextualTokens with enum-based resolution
    std::vector<ContextualToken> translate();
    
    // Get result as stream for convenient processing
    ContextualTokenStream translate_to_stream();
    
    // Error handling
    struct TranslationError {
        std::string message;
        size_t line;
        size_t column;
        std::string context_path;
        
        TranslationError(const std::string& msg, size_t line, size_t col, const std::string& ctx)
            : message(msg), line(line), column(col), context_path(ctx) {}
    };
    
    // Get any translation errors
    const std::vector<TranslationError>& get_errors() const { return errors; }
    bool has_errors() const { return !errors.empty(); }
    
private:
    RawTokenStream raw_tokens;
    ContextStack context_stack;
    std::unique_ptr<ContextResolver> context_resolver;
    std::vector<TranslationError> errors;
    
    // Translation state
    std::vector<ContextualToken> contextual_tokens;
    StringTable& string_table_;  // Reference to string table for index-only operations
    size_t position;
    
    // Main translation loop - pure enum transformations
    ContextualToken translate_next_token();
    
    // Context-sensitive enum transformers (TokenKind -> ContextualTokenKind)
    ContextualTokenKind resolve_runtime_context(const RawToken& token);
    ContextualTokenKind resolve_defer_context(const RawToken& token);
    ContextualTokenKind resolve_exposes_context(const RawToken& token);
    ContextualTokenKind resolve_class_context(const RawToken& token);
    ContextualTokenKind resolve_union_context(const RawToken& token);
    ContextualTokenKind resolve_interface_context(const RawToken& token);
    ContextualTokenKind resolve_function_context(const RawToken& token);
    
    // Direct enum mapping for non-context-sensitive tokens
    ContextualTokenKind map_token_kind_to_contextual(TokenKind kind);
    
    // Context management using enum state only
    void update_parse_context(TokenKind kind, ContextualTokenKind contextual_kind);
    bool is_in_runtime_context() const;
    bool is_in_type_expression_context() const;
    bool is_in_class_declaration_context() const;
    
    // Context management helpers - enum-based only
    void enter_context_from_contextual_token(const ContextualToken& token);
    void exit_context_on_block_end();
    
    // Lookahead helpers - enum-based pattern matching
    bool peek_for_token_sequence(const std::vector<TokenKind>& sequence);
    bool peek_for_runtime_modifier();
    
    // Multi-token construct detection using enum patterns
    bool is_start_of_access_rights_declaration() const;
    bool is_start_of_union_declaration() const;
    bool is_start_of_class_declaration() const;
    bool is_start_of_function_declaration() const;
    
    // Token stream navigation
    const RawToken& current_raw_token() const;
    const RawToken& peek_raw_token(size_t offset = 1) const;
    void advance_raw_token();
    bool is_at_end() const;
    
    // Error reporting
    void error(const std::string& message);
    void error_at_token(const std::string& message, const RawToken& token);
    
    // Debug helpers
    void debug_print_context() const;
    void debug_print_translation_step(const RawToken& raw, const ContextualToken& contextual) const;
};

/**
 * Pure enum-based token transformation registry.
 * Maps TokenKind -> ContextualTokenKind based on parsing context.
 * NO STRING OPERATIONS - only enum transformations.
 */
class ContextualTokenMapper {
public:
    using ContextTransformer = std::function<ContextualTokenKind(const SemanticTranslator&, TokenKind)>;
    
    ContextualTokenMapper();
    
    // Register contextual transformations for specific TokenKinds
    void register_context_transformer(TokenKind kind, ContextTransformer transformer);
    
    // Transform TokenKind to ContextualTokenKind based on current context
    ContextualTokenKind transform(const SemanticTranslator& translator, TokenKind kind) const;
    
    // Check if a TokenKind has contextual transformations
    bool has_contextual_mapping(TokenKind kind) const;
    
    // Direct 1:1 mapping for non-contextual tokens
    ContextualTokenKind get_direct_mapping(TokenKind kind) const;
    
private:
    std::unordered_map<TokenKind, ContextTransformer> context_transformers;
    std::unordered_map<TokenKind, ContextualTokenKind> direct_mappings;
    
    void initialize_direct_mappings();
    void initialize_context_transformers();
};

/**
 * Multi-token construct detector using pure enum patterns.
 * Identifies complex constructs through TokenKind sequences without string operations.
 */
class ConstructDetector {
public:
    explicit ConstructDetector(const SemanticTranslator& translator);
    
    // Pattern-based construct detection using TokenKind sequences
    ContextualTokenKind detect_construct_start(const RawToken& token) const;
    
    // Specific construct detectors
    bool is_access_rights_pattern(const RawToken& current) const;
    bool is_union_declaration_pattern(const RawToken& current) const;
    bool is_class_declaration_pattern(const RawToken& current) const;
    bool is_function_declaration_pattern(const RawToken& current) const;
    bool is_defer_statement_pattern(const RawToken& current) const;
    bool is_type_expression_pattern(const RawToken& current) const;
    
private:
    const SemanticTranslator& translator;
    
    // Pattern matching helpers using TokenKind sequences
    bool matches_token_sequence(const std::vector<TokenKind>& pattern, size_t start_offset = 0) const;
    bool is_runtime_modified_construct(const RawToken& current) const;
    bool is_async_modified_construct(const RawToken& current) const;
};

/**
 * Contextual token validation using pure enum-based logic.
 * Ensures contextual tokens are valid in their parsing context.
 */
class ContextualTokenValidator {
public:
    explicit ContextualTokenValidator(const ContextStack& context_stack);
    
    // Validation methods using enum-based logic
    bool validate(const ContextualToken& token);
    std::vector<std::string> get_validation_errors() const { return validation_errors; }
    
    // Specific validations based on ContextualTokenKind
    bool validate_access_rights_declaration(ContextualTokenKind kind);
    bool validate_union_declaration(ContextualTokenKind kind);
    bool validate_class_declaration(ContextualTokenKind kind);
    bool validate_defer_statement(ContextualTokenKind kind);
    bool validate_type_parameter(ContextualTokenKind kind);
    
private:
    const ContextStack& context_stack;
    std::vector<std::string> validation_errors;
    
    void add_error(const std::string& error);
    bool is_valid_in_current_context(ContextualTokenKind kind);
    bool is_contextually_consistent(ContextualTokenKind kind, ParseContextType context);
};

/**
 * Translation statistics and debugging information.
 */
struct TranslationStats {
    size_t total_raw_tokens;
    size_t total_contextual_tokens;
    size_t context_sensitive_resolutions;
    size_t errors;
    
    std::unordered_map<TokenKind, size_t> token_kind_counts;
    std::unordered_map<ContextualTokenKind, size_t> contextual_token_counts;
    
    double translation_time_ms;
    
    void print_report() const;
    void reset();
};

/**
 * Contextual translator with full debugging and statistics support.
 */
class DebugSemanticTranslator : public SemanticTranslator {
public:
    explicit DebugSemanticTranslator(RawTokenStream raw_tokens, StringTable& string_table);
    
    // Enhanced translation with debugging
    std::vector<ContextualToken> translate_with_debug();
    
    // Get translation statistics
    const TranslationStats& get_stats() const { return stats; }
    
    // Debug output control
    void enable_debug_output(bool enable = true) { debug_output_enabled = enable; }
    void enable_context_tracing(bool enable = true) { context_tracing_enabled = enable; }
    void enable_token_tracing(bool enable = true) { token_tracing_enabled = enable; }
    
private:
    TranslationStats stats;
    bool debug_output_enabled = false;
    bool context_tracing_enabled = false;
    bool token_tracing_enabled = false;
    
    void update_stats(const RawToken& raw_token, const ContextualToken& contextual_token);
    void trace_context_change(const std::string& action, const ParseContext& context);
    void trace_token_translation(const RawToken& raw_token, const ContextualToken& contextual_token);
};

} // namespace cprime