#include "contextual_token.h"
#include <stdexcept>
#include <sstream>

namespace cprime {

// ContextualToken implementation
std::string ContextualToken::to_string() const {
    std::stringstream ss;
    ss << "ContextualToken(";
    
    // Raw token info
    switch (raw_token.type) {
        case RawTokenType::KEYWORD: ss << "KEYWORD"; break;
        case RawTokenType::IDENTIFIER: ss << "IDENTIFIER"; break;
        case RawTokenType::OPERATOR: ss << "OPERATOR"; break;
        case RawTokenType::LITERAL: ss << "LITERAL"; break;
        case RawTokenType::PUNCTUATION: ss << "PUNCTUATION"; break;
        case RawTokenType::WHITESPACE: ss << "WHITESPACE"; break;
        case RawTokenType::COMMENT: ss << "COMMENT"; break;
        case RawTokenType::EOF_TOKEN: ss << "EOF"; break;
    }
    
    ss << ", \"" << raw_token.value << "\", " << raw_token.line << ":" << raw_token.column;
    
    // Context info
    if (!context_resolution.empty()) {
        ss << ", resolution=\"" << context_resolution << "\"";
    }
    
    if (!attributes.empty()) {
        ss << ", attrs={";
        bool first = true;
        for (const auto& [key, value] : attributes.data) {
            if (!first) ss << ", ";
            ss << key << "=\"" << value << "\"";
            first = false;
        }
        ss << "}";
    }
    
    ss << ")";
    return ss.str();
}

// ContextualTokenStream implementation
ContextualTokenStream::ContextualTokenStream(std::vector<ContextualToken> tokens)
    : tokens(std::move(tokens)), pos(0) {}

const ContextualToken& ContextualTokenStream::current() const {
    ensure_valid_position();
    return tokens[pos];
}

const ContextualToken& ContextualTokenStream::peek(size_t offset) const {
    size_t peek_pos = pos + offset;
    if (peek_pos >= tokens.size()) {
        // Return EOF-like token if peeking beyond end
        static const RawToken eof_raw(RawTokenType::EOF_TOKEN, "", 0, 0, 0);
        static const ContextualToken eof_contextual(eof_raw, static_cast<ParseContextType>(0));
        return eof_contextual;
    }
    return tokens[peek_pos];
}

const ContextualToken& ContextualTokenStream::previous() const {
    if (pos == 0) {
        throw std::runtime_error("Cannot access previous token at beginning of stream");
    }
    return tokens[pos - 1];
}

void ContextualTokenStream::advance() {
    if (pos < tokens.size()) {
        pos++;
    }
}

void ContextualTokenStream::rewind() {
    if (pos > 0) {
        pos--;
    }
}

bool ContextualTokenStream::is_at_end() const {
    return pos >= tokens.size();
}

void ContextualTokenStream::set_position(size_t new_pos) {
    if (new_pos > tokens.size()) {
        throw std::runtime_error("Invalid token stream position");
    }
    pos = new_pos;
}

std::vector<ContextualToken> ContextualTokenStream::filter_by_resolution(const std::string& resolution) const {
    std::vector<ContextualToken> result;
    for (const auto& token : tokens) {
        if (token.is_resolved_as(resolution)) {
            result.push_back(token);
        }
    }
    return result;
}

std::vector<ContextualToken> ContextualTokenStream::filter_by_context(ParseContextType context) const {
    std::vector<ContextualToken> result;
    for (const auto& token : tokens) {
        if (token.current_context == context) {
            result.push_back(token);
        }
    }
    return result;
}

size_t ContextualTokenStream::count_by_resolution(const std::string& resolution) const {
    size_t count = 0;
    for (const auto& token : tokens) {
        if (token.is_resolved_as(resolution)) {
            count++;
        }
    }
    return count;
}

void ContextualTokenStream::ensure_valid_position() const {
    if (pos >= tokens.size()) {
        throw std::runtime_error("Token stream position out of bounds");
    }
}

} // namespace cprime