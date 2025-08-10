#pragma once

#include "../commons/compilation_context.h"
#include "../commons/common_types.h"
#include "../commons/logger.h"
#include <map>
#include <string>
#include <sstream>

namespace cprime {

/**
 * Layer 1: Tokenization
 * 
 * Responsibilities:
 * - Read input streams from CompilationContext
 * - Convert raw text to token streams  
 * - Map token streams by stream ID
 * - Populate token_streams field in root scope
 * - Mark Layer 1 as completed
 * 
 * API Design:
 * - Static methods (stateless processing)
 * - Takes CompilationContext, modifies it in-place
 * - Returns VoidResult for error handling
 * - Fills token_streams in scopes[0] (root scope)
 */
class Tokenizer {
public:
    /**
     * Main Layer 1 entry point.
     * Tokenizes all input streams and populates context.scopes[0].token_streams
     * 
     * @param context Compilation context with input_streams populated
     * @return VoidResult indicating success or failure with error message
     */
    static VoidResult tokenize_all_streams(CompilationContext& context);
    
    /**
     * Tokenize a single input stream.
     * 
     * @param stream_id Identifier for the stream (filename)
     * @param input_stream The stringstream containing source code
     * @return Result<std::vector<Token>> containing tokens or error
     */
    static Result<std::vector<Token>> tokenize_stream(
        const std::string& stream_id,
        std::stringstream& input_stream
    );

private:
    /**
     * Core tokenization logic for a single source string.
     */
    static Result<std::vector<Token>> tokenize_source(
        const std::string& source_code,
        const std::string& source_file
    );
    
    /**
     * Character-level tokenization state.
     */
    struct TokenizerState {
        const std::string& source;
        const std::string& source_file;
        size_t position;
        uint32_t line;
        uint32_t column;
        std::vector<Token> tokens;
        
        TokenizerState(const std::string& src, const std::string& file)
            : source(src), source_file(file), position(0), line(1), column(1) {}
    };
    
    // Character inspection
    static char peek(const TokenizerState& state);
    static char peek_next(const TokenizerState& state);
    static void advance(TokenizerState& state);
    static bool is_at_end(const TokenizerState& state);
    
    // Token reading methods
    static bool skip_whitespace(TokenizerState& state);
    static bool read_comment(TokenizerState& state);
    static bool read_identifier_or_keyword(TokenizerState& state);
    static bool read_string_literal(TokenizerState& state);
    static bool read_number_literal(TokenizerState& state);
    static bool read_operator_or_punctuation(TokenizerState& state);
    
    // Utility methods
    static bool is_alpha(char c);
    static bool is_digit(char c);
    static bool is_alphanumeric(char c);
    static bool is_whitespace(char c);
    static TokenKind classify_identifier(const std::string& identifier);
    static TokenKind classify_operator(const std::string& op_text);
    
    // Token creation helpers
    static void add_token(TokenizerState& state, TokenKind kind);
    static void add_token(TokenizerState& state, TokenKind kind, const std::string& value);
    
    template<typename T>
    static void add_token(TokenizerState& state, TokenKind kind, T literal_value);
    
    // Error handling
    static void handle_unknown_character(TokenizerState& state);
};

} // namespace cprime