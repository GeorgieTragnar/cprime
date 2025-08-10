#pragma once

#include "token_types.h"
#include <string>

namespace cprime {

// Forward declarations
struct RawToken;

/**
 * Utility functions for token and context type conversions and queries.
 */



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