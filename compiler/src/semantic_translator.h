#pragma once

#include "raw_token.h"
#include "semantic_token.h"
#include "context_stack.h"
#include <functional>
#include <unordered_map>
#include <memory>

namespace cprime {

/**
 * Semantic translator - Layer 2 of the three-layer architecture.
 * Converts raw tokens into semantic tokens with context-sensitive keyword resolution.
 */
class SemanticTranslator {
public:
    explicit SemanticTranslator(RawTokenStream raw_tokens);
    
    // Main translation method
    std::vector<SemanticToken> translate();
    
    // Get result as stream for convenient processing
    SemanticTokenStream translate_to_stream();
    
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
    std::vector<SemanticToken> semantic_tokens;
    size_t position;
    
    // Main translation loop
    SemanticToken translate_next_token();
    
    // Context-sensitive keyword resolvers
    SemanticToken resolve_runtime_keyword();
    SemanticToken resolve_defer_keyword();
    SemanticToken resolve_exposes_keyword();
    SemanticToken resolve_class_keyword();
    SemanticToken resolve_union_keyword();
    SemanticToken resolve_interface_keyword();
    SemanticToken resolve_function_keyword();
    
    // Context-sensitive phrase resolvers (multi-token constructs)
    SemanticToken resolve_access_rights_declaration();
    SemanticToken resolve_union_declaration();
    SemanticToken resolve_class_declaration();
    SemanticToken resolve_type_expression();
    SemanticToken resolve_defer_statement();
    
    // Pass-through token handlers
    SemanticToken handle_identifier();
    SemanticToken handle_literal();
    SemanticToken handle_operator();
    SemanticToken handle_punctuation();
    SemanticToken handle_comment();
    
    // Context management helpers
    void update_context_from_token(const RawToken& token);
    void enter_context_from_semantic_token(const SemanticToken& token);
    void exit_context_on_block_end();
    
    // Lookahead and parsing helpers
    bool peek_for_sequence(const std::vector<std::string>& sequence);
    bool peek_for_runtime_modifier();
    std::vector<std::string> parse_field_list();
    std::vector<std::string> parse_parameter_list();
    std::vector<std::string> parse_union_variants();
    std::string parse_function_call();
    std::string parse_expression_until(const std::string& terminator);
    
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
    void debug_print_translation_step(const RawToken& raw, const SemanticToken& semantic) const;
};

/**
 * Keyword resolver registry - maps keywords to their resolution strategies.
 */
class KeywordResolverRegistry {
public:
    using ResolverFunction = std::function<SemanticToken(SemanticTranslator&, const RawToken&)>;
    
    KeywordResolverRegistry();
    
    // Register keyword resolvers
    void register_resolver(const std::string& keyword, ResolverFunction resolver);
    
    // Resolve keyword based on context
    bool can_resolve(const std::string& keyword) const;
    SemanticToken resolve(const std::string& keyword, SemanticTranslator& translator, const RawToken& token);
    
    // Query available keywords
    std::vector<std::string> get_registered_keywords() const;
    
private:
    std::unordered_map<std::string, ResolverFunction> resolvers;
    
    void initialize_default_resolvers();
};

/**
 * Context-sensitive phrase parser for multi-token constructs.
 * Handles complex language constructs that span multiple tokens.
 */
class PhraseParser {
public:
    explicit PhraseParser(SemanticTranslator& translator);
    
    // Parse complex constructs
    SemanticToken parse_access_rights_declaration();
    SemanticToken parse_union_declaration(); 
    SemanticToken parse_class_declaration();
    SemanticToken parse_function_declaration();
    SemanticToken parse_defer_statement();
    SemanticToken parse_type_expression();
    
    // Helper methods for parsing sub-components
    struct AccessRightInfo {
        std::string access_right_name;
        std::vector<std::string> granted_fields;
        bool is_runtime;
    };
    
    struct UnionInfo {
        std::string union_name;
        std::vector<std::string> variants;
        bool is_runtime;
    };
    
    struct ClassInfo {
        std::string class_name;
        std::vector<std::string> fields;
        std::string class_type; // "data", "functional", "danger"
    };
    
    AccessRightInfo parse_access_right_info();
    UnionInfo parse_union_info();
    ClassInfo parse_class_info();
    
private:
    SemanticTranslator& translator;
    
    // Helper methods
    std::vector<std::string> parse_identifier_list();
    std::string parse_qualified_identifier();
    bool consume_keyword(const std::string& keyword);
    bool consume_punctuation(const std::string& punct);
    std::string expect_identifier();
};

/**
 * Semantic validation for translated tokens.
 * Ensures semantic tokens are valid in their context.
 */
class SemanticValidator {
public:
    explicit SemanticValidator(const ContextStack& context_stack);
    
    // Validation methods
    bool validate(const SemanticToken& token);
    std::vector<std::string> get_validation_errors() const { return validation_errors; }
    
    // Specific validations
    bool validate_access_rights_declaration(const SemanticToken& token);
    bool validate_union_declaration(const SemanticToken& token);
    bool validate_class_declaration(const SemanticToken& token);
    bool validate_defer_statement(const SemanticToken& token);
    bool validate_type_parameter(const SemanticToken& token);
    
private:
    const ContextStack& context_stack;
    std::vector<std::string> validation_errors;
    
    void add_error(const std::string& error);
    bool is_valid_identifier(const std::string& identifier);
    bool is_valid_in_current_context(SemanticTokenType type);
};

/**
 * Translation statistics and debugging information.
 */
struct TranslationStats {
    size_t total_raw_tokens;
    size_t total_semantic_tokens;
    size_t context_sensitive_resolutions;
    size_t errors;
    
    std::unordered_map<std::string, size_t> keyword_resolution_counts;
    std::unordered_map<SemanticTokenType, size_t> semantic_token_counts;
    
    double translation_time_ms;
    
    void print_report() const;
    void reset();
};

/**
 * Semantic translator with full debugging and statistics support.
 */
class DebugSemanticTranslator : public SemanticTranslator {
public:
    explicit DebugSemanticTranslator(RawTokenStream raw_tokens);
    
    // Enhanced translation with debugging
    std::vector<SemanticToken> translate_with_debug();
    
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
    
    void update_stats(const RawToken& raw_token, const SemanticToken& semantic_token);
    void trace_context_change(const std::string& action, const ParseContext& context);
    void trace_token_translation(const RawToken& raw_token, const SemanticToken& semantic_token);
};

} // namespace cprime