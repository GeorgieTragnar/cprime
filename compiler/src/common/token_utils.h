#pragma once

#include "token_types.h"
#include "parse_context.h"
#include <string>

namespace cprime {

// Forward declarations
struct RawToken;
struct ContextualToken;

/**
 * Utility functions for token and context type conversions and queries.
 */

/**
 * Convert TokenKind to string for debugging.
 */
const char* token_kind_to_string(TokenKind kind);

/**
 * Convert ContextualTokenKind to string for debugging.
 */
const char* contextual_token_kind_to_string(ContextualTokenKind kind);

/**
 * Convert ParseContextType to string for debugging.
 */
const char* parse_context_type_to_string(ParseContextType context);

/**
 * Check if a TokenKind represents a literal value.
 */
bool is_literal(TokenKind kind);

/**
 * Check if a ContextualTokenKind represents a literal value.
 */
bool is_contextual_literal(ContextualTokenKind kind);

/**
 * Check if a ContextualTokenKind represents an operator.
 */
bool is_contextual_operator(ContextualTokenKind kind);

/**
 * Check if a ContextualTokenKind represents a keyword.
 */
bool is_contextual_keyword(ContextualTokenKind kind);

/**
 * Check if a ContextualTokenKind represents a type declaration.
 */
bool is_contextual_type_declaration(ContextualTokenKind kind);

/**
 * Check if a ContextualTokenKind represents a context-sensitive interpretation.
 */
bool is_context_sensitive(ContextualTokenKind kind);

} // namespace cprime