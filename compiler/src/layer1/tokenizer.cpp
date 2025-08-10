#include "tokenizer.h"
#include <unordered_map>
#include <unordered_set>

namespace cprime {

// Static keyword mapping
static const std::unordered_map<std::string, TokenKind> KEYWORDS = {
    {"class", TokenKind::CLASS},
    {"struct", TokenKind::STRUCT}, 
    {"interface", TokenKind::INTERFACE},
    {"function", TokenKind::FUNCTION},
    {"runtime", TokenKind::RUNTIME},
    {"defer", TokenKind::DEFER},
    {"auto", TokenKind::AUTO},
    {"var", TokenKind::VAR},
    {"const", TokenKind::CONST},
    {"if", TokenKind::IF},
    {"else", TokenKind::ELSE},
    {"while", TokenKind::WHILE},
    {"for", TokenKind::FOR},
    {"return", TokenKind::RETURN},
    {"break", TokenKind::BREAK},
    {"continue", TokenKind::CONTINUE},
    {"try", TokenKind::TRY},
    {"catch", TokenKind::CATCH},
    {"finally", TokenKind::FINALLY},
    {"true", TokenKind::BOOL_LITERAL},
    {"false", TokenKind::BOOL_LITERAL}
};

VoidResult Tokenizer::tokenize_all_streams(CompilationContext& context) {
    // Ensure scope vector is initialized
    if (context.scopes.empty()) {
        context.initialize_scope_vector();
    }
    
    // Get root scope where we'll store token streams
    Scope& root_scope = context.get_root_scope();
    root_scope.token_streams.clear();
    
    // Tokenize each input stream
    for (auto& [stream_id, input_stream] : context.input_streams) {
        auto token_result = tokenize_stream(stream_id, input_stream);
        
        if (!token_result.success()) {
            return failure<bool>("Failed to tokenize stream " + stream_id + ": " + token_result.error());
        }
        
        // Store tokens in root scope
        root_scope.token_streams[stream_id] = std::move(token_result.value());
    }
    
    // Mark Layer 1 as completed
    root_scope.mark_layer_completed(1);
    context.current_processing_layer = 1;
    
    return success();
}

Result<std::vector<Token>> Tokenizer::tokenize_stream(
    const std::string& stream_id,
    std::stringstream& input_stream
) {
    // Extract source code from stream
    std::string source_code = input_stream.str();
    
    // Tokenize the source
    return tokenize_source(source_code, stream_id);
}

Result<std::vector<Token>> Tokenizer::tokenize_source(
    const std::string& source_code,
    const std::string& source_file
) {
    TokenizerState state(source_code, source_file);
    
    while (!is_at_end(state)) {
        // Skip whitespace (don't create tokens for it in basic implementation)
        if (skip_whitespace(state)) {
            continue;
        }
        
        // Try different token types
        if (read_comment(state)) {
            continue; // Comments are skipped in basic implementation
        }
        
        if (read_identifier_or_keyword(state)) {
            continue;
        }
        
        if (read_string_literal(state)) {
            continue;
        }
        
        if (read_number_literal(state)) {
            continue;
        }
        
        if (read_operator_or_punctuation(state)) {
            continue;
        }
        
        // Unknown character
        handle_unknown_character(state);
    }
    
    // Add EOF token
    add_token(state, TokenKind::EOF_TOKEN);
    
    return Result<std::vector<Token>>(std::move(state.tokens));
}

// Character inspection methods
char Tokenizer::peek(const TokenizerState& state) {
    if (is_at_end(state)) return '\0';
    return state.source[state.position];
}

char Tokenizer::peek_next(const TokenizerState& state) {
    if (state.position + 1 >= state.source.length()) return '\0';
    return state.source[state.position + 1];
}

void Tokenizer::advance(TokenizerState& state) {
    if (!is_at_end(state)) {
        if (state.source[state.position] == '\n') {
            state.line++;
            state.column = 1;
        } else {
            state.column++;
        }
        state.position++;
    }
}

bool Tokenizer::is_at_end(const TokenizerState& state) {
    return state.position >= state.source.length();
}

// Token reading methods
bool Tokenizer::skip_whitespace(TokenizerState& state) {
    char c = peek(state);
    if (!is_whitespace(c)) return false;
    
    while (!is_at_end(state) && is_whitespace(peek(state))) {
        advance(state);
    }
    return true;
}

bool Tokenizer::read_comment(TokenizerState& state) {
    if (peek(state) != '/') return false;
    
    // Line comment
    if (peek_next(state) == '/') {
        advance(state); // consume '/'
        advance(state); // consume '/'
        
        while (!is_at_end(state) && peek(state) != '\n') {
            advance(state);
        }
        return true;
    }
    
    // Block comment
    if (peek_next(state) == '*') {
        advance(state); // consume '/'
        advance(state); // consume '*'
        
        while (!is_at_end(state)) {
            if (peek(state) == '*' && peek_next(state) == '/') {
                advance(state); // consume '*'
                advance(state); // consume '/'
                break;
            }
            advance(state);
        }
        return true;
    }
    
    return false;
}

bool Tokenizer::read_identifier_or_keyword(TokenizerState& state) {
    char c = peek(state);
    if (!is_alpha(c) && c != '_') return false;
    
    size_t start = state.position;
    
    while (!is_at_end(state) && (is_alphanumeric(peek(state)) || peek(state) == '_')) {
        advance(state);
    }
    
    std::string text = state.source.substr(start, state.position - start);
    
    // Check if it's a keyword
    TokenKind kind = classify_identifier(text);
    
    if (kind == TokenKind::BOOL_LITERAL) {
        // Handle boolean literals specially
        bool value = (text == "true");
        add_token(state, kind, value);
    } else if (kind == TokenKind::IDENTIFIER) {
        add_token(state, kind, text);
    } else {
        add_token(state, kind); // Keyword
    }
    
    return true;
}

bool Tokenizer::read_string_literal(TokenizerState& state) {
    if (peek(state) != '"') return false;
    
    advance(state); // consume opening quote
    size_t start = state.position;
    
    while (!is_at_end(state) && peek(state) != '"') {
        if (peek(state) == '\\') {
            advance(state); // skip escape character
            if (!is_at_end(state)) {
                advance(state); // skip escaped character
            }
        } else {
            advance(state);
        }
    }
    
    if (is_at_end(state)) {
        // Unterminated string - for now, just treat as unknown
        return false;
    }
    
    std::string value = state.source.substr(start, state.position - start);
    advance(state); // consume closing quote
    
    add_token(state, TokenKind::STRING_LITERAL, value);
    return true;
}

bool Tokenizer::read_number_literal(TokenizerState& state) {
    if (!is_digit(peek(state))) return false;
    
    size_t start = state.position;
    bool is_float = false;
    
    // Read digits
    while (!is_at_end(state) && is_digit(peek(state))) {
        advance(state);
    }
    
    // Check for decimal point
    if (!is_at_end(state) && peek(state) == '.' && is_digit(peek_next(state))) {
        is_float = true;
        advance(state); // consume '.'
        
        while (!is_at_end(state) && is_digit(peek(state))) {
            advance(state);
        }
    }
    
    std::string text = state.source.substr(start, state.position - start);
    
    if (is_float) {
        double value = std::stod(text);
        add_token(state, TokenKind::DOUBLE_LITERAL, value);
    } else {
        int32_t value = std::stoi(text);
        add_token(state, TokenKind::INT_LITERAL, value);
    }
    
    return true;
}

bool Tokenizer::read_operator_or_punctuation(TokenizerState& state) {
    char c = peek(state);
    
    // Multi-character operators
    if (c == '=' && peek_next(state) == '=') {
        advance(state);
        advance(state);
        add_token(state, TokenKind::EQUALS);
        return true;
    }
    
    if (c == '!' && peek_next(state) == '=') {
        advance(state);
        advance(state);
        add_token(state, TokenKind::NOT_EQUALS);
        return true;
    }
    
    if (c == '<' && peek_next(state) == '=') {
        advance(state);
        advance(state);
        add_token(state, TokenKind::LESS_EQUAL);
        return true;
    }
    
    if (c == '>' && peek_next(state) == '=') {
        advance(state);
        advance(state);
        add_token(state, TokenKind::GREATER_EQUAL);
        return true;
    }
    
    if (c == '-' && peek_next(state) == '>') {
        advance(state);
        advance(state);
        add_token(state, TokenKind::ARROW);
        return true;
    }
    
    // Single character tokens
    TokenKind kind = TokenKind::UNKNOWN;
    switch (c) {
        case '+': kind = TokenKind::PLUS; break;
        case '-': kind = TokenKind::MINUS; break;
        case '*': kind = TokenKind::MULTIPLY; break;
        case '/': kind = TokenKind::DIVIDE; break;
        case '%': kind = TokenKind::MODULO; break;
        case '=': kind = TokenKind::ASSIGN; break;
        case '<': kind = TokenKind::LESS_THAN; break;
        case '>': kind = TokenKind::GREATER_THAN; break;
        case '!': kind = TokenKind::LOGICAL_NOT; break;
        case '(': kind = TokenKind::LEFT_PAREN; break;
        case ')': kind = TokenKind::RIGHT_PAREN; break;
        case '{': kind = TokenKind::LEFT_BRACE; break;
        case '}': kind = TokenKind::RIGHT_BRACE; break;
        case '[': kind = TokenKind::LEFT_BRACKET; break;
        case ']': kind = TokenKind::RIGHT_BRACKET; break;
        case ';': kind = TokenKind::SEMICOLON; break;
        case ',': kind = TokenKind::COMMA; break;
        case '.': kind = TokenKind::DOT; break;
        case ':': kind = TokenKind::COLON; break;
        default: return false;
    }
    
    advance(state);
    add_token(state, kind);
    return true;
}

// Utility methods
bool Tokenizer::is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool Tokenizer::is_digit(char c) {
    return c >= '0' && c <= '9';
}

bool Tokenizer::is_alphanumeric(char c) {
    return is_alpha(c) || is_digit(c);
}

bool Tokenizer::is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

TokenKind Tokenizer::classify_identifier(const std::string& identifier) {
    auto it = KEYWORDS.find(identifier);
    return (it != KEYWORDS.end()) ? it->second : TokenKind::IDENTIFIER;
}

// Token creation helpers
void Tokenizer::add_token(TokenizerState& state, TokenKind kind) {
    Token token(kind, state.line, state.column, static_cast<uint32_t>(state.position));
    token.source_file = state.source_file;
    token.tokenized = true;
    state.tokens.push_back(std::move(token));
}

void Tokenizer::add_token(TokenizerState& state, TokenKind kind, const std::string& value) {
    Token token(kind, value, state.line, state.column, static_cast<uint32_t>(state.position));
    token.source_file = state.source_file;
    token.tokenized = true;
    state.tokens.push_back(std::move(token));
}

template<typename T>
void Tokenizer::add_token(TokenizerState& state, TokenKind kind, T literal_value) {
    Token token(kind, state.line, state.column, static_cast<uint32_t>(state.position));
    token.literal_value = literal_value;
    token.source_file = state.source_file;
    token.tokenized = true;
    state.tokens.push_back(std::move(token));
}

void Tokenizer::handle_unknown_character(TokenizerState& state) {
    // For unknown characters, just skip them and continue
    // In a production compiler, this would be an error
    advance(state);
}

} // namespace cprime