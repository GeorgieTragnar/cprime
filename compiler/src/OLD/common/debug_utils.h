#pragma once

#include "tokens.h"
#include "token_types.h"
#include "structural_types.h"
#include "string_table.h"
#include <string>
#include <vector>

namespace cprime::debug_utils {

// ========================================================================
// Token String Representations (Layer 1 Debug Functions)
// ========================================================================

/**
 * Convert TokenKind enum to string representation.
 */
const char* token_kind_to_string(TokenKind kind);

/**
 * Convert ContextualTokenKind enum to string representation.
 */
const char* contextual_token_kind_to_string(ContextualTokenKind kind);

/**
 * Convert RawToken to human-readable string with position info.
 */
std::string raw_token_to_string(const RawToken& token, const StringTable& string_table);

// ========================================================================
// Token Sequence Analysis (Layer 1 Debug Functions)
// ========================================================================

/**
 * Convert token sequence to debug string.
 */
std::string tokens_to_string(const std::vector<RawToken>& tokens, const StringTable& string_table);

/**
 * Print token sequence to stdout with analysis.
 */
void print_tokens(const std::vector<RawToken>& tokens, const StringTable& string_table);

/**
 * Analyze and print token distribution statistics.
 */
void analyze_token_distribution(const std::vector<RawToken>& tokens);

/**
 * Print comprehensive token statistics.
 */
void print_token_statistics(const std::vector<RawToken>& tokens, const StringTable& string_table);

// ========================================================================
// Structured Token Analysis (Layer 2 Debug Functions)
// ========================================================================

/**
 * Convert StructuredTokens to comprehensive debug string.
 */
std::string structured_tokens_to_debug_string(const StructuredTokens& structured);

/**
 * Print structured tokens with hierarchy visualization.
 */
void print_structured_tokens(const StructuredTokens& structured);

/**
 * Print scope hierarchy as tree structure.
 */
void print_scope_hierarchy(const StructuredTokens& structured);

/**
 * Analyze and print scope distribution statistics.
 */
void analyze_scope_distribution(const StructuredTokens& structured);

/**
 * Convert single scope to debug string.
 */
std::string scope_to_string(const Scope& scope, size_t scope_index, 
                           const StringTable& string_table, bool contextualized = false);

/**
 * Convert scope type to string.
 */
std::string scope_type_to_string(Scope::Type type);

// ========================================================================
// Contextualization Analysis (Layer 3 Debug Functions)
// ========================================================================

/**
 * Print contextualization report showing transformations.
 */
void print_contextualization_report(const StructuredTokens& structured);

/**
 * Analyze changes between pre and post contextualization.
 */
void analyze_contextualization_changes(const StructuredTokens& before, const StructuredTokens& after);

/**
 * Print only context-sensitive tokens with their interpretations.
 */
void print_context_sensitive_tokens(const StructuredTokens& structured);

/**
 * Analyze context resolution patterns across the structure.
 */
void analyze_context_resolution_patterns(const StructuredTokens& structured);

/**
 * Print contextualization errors with details.
 */
void print_contextualization_errors(const std::vector<std::string>& errors);

// ========================================================================
// Private Helper Functions
// ========================================================================

namespace internal {

/**
 * Format token position for debug output.
 */
std::string format_token_position(const RawToken& token);

/**
 * Format literal value for debug output.
 */
std::string format_literal_value(const RawToken& token);

/**
 * Format token sequence for debug output.
 */
std::string format_token_sequence(const std::vector<uint32_t>& tokens, 
                                 const StringTable& string_table, bool contextualized);

/**
 * Print scope tree recursively.
 */
void print_scope_tree(const StructuredTokens& structured, size_t scope_idx, int indent_level = 0);

/**
 * Check if token is context-sensitive.
 */
bool is_context_sensitive_token(ContextualTokenKind kind);

/**
 * Format contextualization change for display.
 */
std::string format_contextualization_change(TokenKind original, ContextualTokenKind contextual);

} // namespace internal

} // namespace cprime::debug_utils