#include "instruction_contextualizer.h"
#include "../../commons/logger.h"
#include "../../commons/enum/token.h"
#include <algorithm>
#include <cassert>

namespace cprime::layer2_contextualization {

void InstructionContextualizer::register_pattern(const ContextualizationPattern& pattern) {
    auto logger = cprime::LoggerFactory::get_logger("instruction_contextualizer");
    
    // Insert pattern in priority order (higher priority first)
    auto insert_pos = std::upper_bound(patterns_.begin(), patterns_.end(), pattern,
        [](const ContextualizationPattern& a, const ContextualizationPattern& b) {
            return a.priority > b.priority;
        });
    
    patterns_.insert(insert_pos, pattern);
    
    LOG_DEBUG("Registered pattern '{}' with priority {} (pattern #{} of {})", 
              pattern.pattern_name, pattern.priority, 
              std::distance(patterns_.begin(), insert_pos) + 1, patterns_.size());
}

std::vector<ContextualToken> InstructionContextualizer::contextualize_instruction(const std::vector<Token>& tokens) {
    auto logger = cprime::LoggerFactory::get_logger("instruction_contextualizer");
    
    if (tokens.empty()) {
        LOG_DEBUG("Empty token sequence - no contextualization needed");
        return {};
    }
    
    LOG_DEBUG("Contextualizing instruction with {} tokens using {} patterns", 
              tokens.size(), patterns_.size());
    
    std::vector<ContextualToken> result;
    size_t pos = 0;
    
    while (pos < tokens.size()) {
        bool pattern_found = false;
        
        // Skip whitespace tokens (spaces, newlines) - they don't contribute to contextualization
        if (tokens[pos]._token == EToken::SPACE || tokens[pos]._token == EToken::NEWLINE) {
            pos++;
            continue;
        }
        
        // Try each pattern at current position (ordered by priority)
        for (const auto& pattern : patterns_) {
            PatternMatchResult match_result = try_match_pattern(tokens, pos, pattern);
            
            if (match_result.matched) {
                LOG_DEBUG("Pattern '{}' matched at position {} consuming {} tokens", 
                          pattern.pattern_name, pos, match_result.tokens_consumed);
                
                // Add generated contextual tokens to result
                result.insert(result.end(), 
                             match_result.contextual_tokens.begin(), 
                             match_result.contextual_tokens.end());
                
                pos += match_result.tokens_consumed;
                pattern_found = true;
                break;
            }
        }
        
        if (!pattern_found) {
            // No pattern matched - create INVALID contextual token for this token
            LOG_DEBUG("No pattern matched at position {} (token type: {})", 
                      pos, static_cast<uint32_t>(tokens[pos]._token));
            
            ContextualToken invalid_token;
            invalid_token._contextualToken = EContextualToken::INVALID;
            invalid_token._parentTokenIndices.push_back(tokens[pos]._tokenIndex);
            result.push_back(invalid_token);
            
            pos++;
        }
    }
    
    LOG_DEBUG("Contextualization complete: {} input tokens → {} contextual tokens", 
              tokens.size(), result.size());
    
    return result;
}

PatternMatchResult InstructionContextualizer::try_match_pattern(const std::vector<Token>& tokens, 
                                                               size_t start_pos, 
                                                               const ContextualizationPattern& pattern) {
    auto logger = cprime::LoggerFactory::get_logger("instruction_contextualizer");
    
    // Check if we have enough tokens remaining
    if (start_pos >= tokens.size()) {
        return PatternMatchResult::failure("Start position beyond token sequence");
    }
    
    // Simple sequential matching for now (no complex pattern elements yet)
    size_t pattern_pos = 0;
    size_t token_pos = start_pos;
    
    LOG_DEBUG("Trying pattern '{}' at token position {}", pattern.pattern_name, start_pos);
    
    // Match each element in the pattern
    while (pattern_pos < pattern.token_pattern.size() && token_pos < tokens.size()) {
        PatternElement element = pattern.token_pattern[pattern_pos];
        
        // Check if this is a whitespace pattern element
        if (element == PatternElement::OPTIONAL_WHITESPACE ||
            element == PatternElement::REQUIRED_WHITESPACE ||
            element == PatternElement::SINGLE_WHITESPACE ||
            element == PatternElement::MERGED_WHITESPACE) {
            
            // Handle whitespace pattern matching
            WhitespaceMatchResult whitespace_result = try_match_whitespace_pattern(tokens, token_pos, element);
            
            if (!whitespace_result.matched) {
                LOG_DEBUG("Whitespace pattern element {} failed to match at position {}", 
                          static_cast<uint32_t>(element), token_pos);
                return PatternMatchResult::failure("Whitespace pattern does not match");
            }
            
            LOG_DEBUG("Whitespace pattern element {} matched {} tokens at position {}", 
                      static_cast<uint32_t>(element), whitespace_result.tokens_consumed, token_pos);
            
            token_pos += whitespace_result.tokens_consumed;
            pattern_pos++;
            continue;
        }
        
        // Skip whitespace in token stream while matching non-whitespace patterns
        while (token_pos < tokens.size() && is_whitespace_token(tokens[token_pos])) {
            token_pos++;
        }
        
        if (token_pos >= tokens.size()) {
            return PatternMatchResult::failure("Ran out of tokens while matching pattern");
        }
        
        if (!token_matches_element(tokens[token_pos], element)) {
            LOG_DEBUG("Pattern element {} failed to match token at position {} (type: {})", 
                      static_cast<uint32_t>(element), token_pos, static_cast<uint32_t>(tokens[token_pos]._token));
            return PatternMatchResult::failure("Token does not match pattern element");
        }
        
        LOG_DEBUG("Pattern element {} matched token at position {}", 
                  static_cast<uint32_t>(element), token_pos);
        
        pattern_pos++;
        token_pos++;
    }
    
    // Check if we matched the entire pattern
    if (pattern_pos != pattern.token_pattern.size()) {
        return PatternMatchResult::failure("Pattern not fully matched");
    }
    
    // Generate contextual tokens from the matched pattern
    std::vector<ContextualToken> contextual_tokens;
    
    for (const auto& template_def : pattern.output_templates) {
        ContextualToken ctx_token = create_contextual_token(template_def, tokens, start_pos);
        contextual_tokens.push_back(ctx_token);
        
        LOG_INFO("Generated contextual token type {} referencing {} source tokens: [{}]", 
                  static_cast<uint32_t>(ctx_token._contextualToken), 
                  ctx_token._parentTokenIndices.size(),
                  [&](){
                      std::string indices;
                      for (size_t i = 0; i < ctx_token._parentTokenIndices.size(); ++i) {
                          if (i > 0) indices += ", ";
                          indices += std::to_string(ctx_token._parentTokenIndices[i]);
                      }
                      return indices;
                  }());
    }
    
    size_t tokens_consumed = token_pos - start_pos;
    LOG_DEBUG("Pattern '{}' successfully matched, consuming {} tokens", 
              pattern.pattern_name, tokens_consumed);
    
    return PatternMatchResult::success(tokens_consumed, contextual_tokens);
}

bool InstructionContextualizer::token_matches_element(const Token& token, PatternElement element) {
    switch (element) {
        // Specific keywords
        case PatternElement::KEYWORD_INT:
            return token._token == EToken::IDENTIFIER; // TODO: Check actual content is "int"
        case PatternElement::KEYWORD_FUNC:
            return token._token == EToken::IDENTIFIER; // TODO: Check actual content is "func"
        case PatternElement::KEYWORD_AUTO:
            return token._token == EToken::IDENTIFIER; // TODO: Check actual content is "auto"
        case PatternElement::KEYWORD_IF:
            return token._token == EToken::IDENTIFIER; // TODO: Check actual content is "if"
        case PatternElement::KEYWORD_WHILE:
            return token._token == EToken::IDENTIFIER; // TODO: Check actual content is "while"
        case PatternElement::KEYWORD_FOR:
            return token._token == EToken::IDENTIFIER; // TODO: Check actual content is "for"
        case PatternElement::KEYWORD_RETURN:
            return token._token == EToken::IDENTIFIER; // TODO: Check actual content is "return"
        case PatternElement::KEYWORD_EXEC:
            return token._token == EToken::IDENTIFIER; // TODO: Check actual content is "exec"
            
        // Generic token types
        case PatternElement::ANY_IDENTIFIER:
            return token._token == EToken::IDENTIFIER;
        case PatternElement::ANY_LITERAL:
            return token._token == EToken::STRING_LITERAL || 
                   token._token == EToken::INT_LITERAL ||
                   token._token == EToken::FLOAT_LITERAL;
        case PatternElement::ANY_STRING_LITERAL:
            return token._token == EToken::STRING_LITERAL;
        case PatternElement::ANY_INT_LITERAL:
            return token._token == EToken::INT_LITERAL;
            
        // Specific operators and punctuation
        case PatternElement::LITERAL_ASSIGN:
            return token._token == EToken::ASSIGN;
        case PatternElement::LITERAL_PLUS:
            return token._token == EToken::PLUS;
        case PatternElement::LITERAL_MINUS:
            return token._token == EToken::MINUS;
        case PatternElement::LITERAL_MULTIPLY:
            return token._token == EToken::MULTIPLY;
        case PatternElement::LITERAL_DIVIDE:
            return token._token == EToken::DIVIDE;
        case PatternElement::LITERAL_SEMICOLON:
            return token._token == EToken::SEMICOLON;
        case PatternElement::LITERAL_COLON:
            return token._token == EToken::COLON;
        case PatternElement::LITERAL_COMMA:
            return token._token == EToken::COMMA;
        case PatternElement::LITERAL_DOT:
            return token._token == EToken::DOT;
            
        // Brackets and delimiters
        case PatternElement::LITERAL_PAREN_L:
            return token._token == EToken::LEFT_PAREN;
        case PatternElement::LITERAL_PAREN_R:
            return token._token == EToken::RIGHT_PAREN;
        case PatternElement::LITERAL_BRACE_L:
            return token._token == EToken::LEFT_BRACE;
        case PatternElement::LITERAL_BRACE_R:
            return token._token == EToken::RIGHT_BRACE;
        case PatternElement::LITERAL_BRACKET_L:
            return token._token == EToken::LEFT_BRACKET;
        case PatternElement::LITERAL_BRACKET_R:
            return token._token == EToken::RIGHT_BRACKET;
        case PatternElement::LITERAL_LESS:
            return token._token == EToken::LESS_THAN;
        case PatternElement::LITERAL_GREATER:
            return token._token == EToken::GREATER_THAN;
            
        // TODO: Implement compound operators and special pattern elements
        case PatternElement::LITERAL_DOUBLE_COLON:
        case PatternElement::LITERAL_ARROW:
        case PatternElement::LITERAL_PLUS_ASSIGN:
        case PatternElement::LITERAL_MINUS_ASSIGN:
            return false; // Not implemented yet
            
        // Whitespace pattern elements handled separately in try_match_whitespace_pattern
        case PatternElement::OPTIONAL_WHITESPACE:
        case PatternElement::REQUIRED_WHITESPACE:
        case PatternElement::SINGLE_WHITESPACE:
        case PatternElement::MERGED_WHITESPACE:
            return false; // Handled by whitespace matching logic
            
        case PatternElement::EXPRESSION_TOKENS:
        case PatternElement::TYPE_TOKEN_LIST:
            return false; // Not implemented yet
            
        default:
            return false;
    }
}

ContextualToken InstructionContextualizer::create_contextual_token(const ContextualTokenTemplate& token_template,
                                                                   const std::vector<Token>& source_tokens,
                                                                   size_t pattern_start_pos) {
    ContextualToken ctx_token;
    ctx_token._contextualToken = token_template.contextual_type;
    
    // Convert relative indices to absolute token indices
    for (uint32_t relative_index : token_template.source_token_indices) {
        size_t absolute_pos = pattern_start_pos + relative_index;
        
        // Skip whitespace tokens when building parent token indices (unless this is a whitespace contextual token)
        bool skip_whitespace = (token_template.contextual_type != EContextualToken::WHITESPACE &&
                               token_template.contextual_type != EContextualToken::FORMATTING);
        
        while (skip_whitespace && absolute_pos < source_tokens.size() && is_whitespace_token(source_tokens[absolute_pos])) {
            absolute_pos++;
        }
        
        if (absolute_pos < source_tokens.size()) {
            ctx_token._parentTokenIndices.push_back(source_tokens[absolute_pos]._tokenIndex);
        }
    }
    
    // TODO: Populate _contexts if needed
    
    return ctx_token;
}

bool InstructionContextualizer::is_whitespace_token(const Token& token) {
    return token._token == EToken::SPACE || 
           token._token == EToken::NEWLINE ||
           token._token == EToken::TAB ||
           token._token == EToken::CARRIAGE_RETURN;
}

WhitespaceMatchResult InstructionContextualizer::try_match_whitespace_pattern(const std::vector<Token>& tokens,
                                                                             size_t start_pos,
                                                                             PatternElement whitespace_element) {
    auto logger = cprime::LoggerFactory::get_logger("instruction_contextualizer");
    
    if (start_pos >= tokens.size()) {
        return WhitespaceMatchResult::failure();
    }
    
    std::vector<uint32_t> whitespace_indices;
    size_t pos = start_pos;
    
    // Count consecutive whitespace tokens
    while (pos < tokens.size() && is_whitespace_token(tokens[pos])) {
        whitespace_indices.push_back(tokens[pos]._tokenIndex);
        pos++;
    }
    
    size_t whitespace_count = pos - start_pos;
    
    switch (whitespace_element) {
        case PatternElement::OPTIONAL_WHITESPACE:
            // Always succeeds, consumes 0 or more whitespace tokens
            LOG_DEBUG("OPTIONAL_WHITESPACE matched {} tokens", whitespace_count);
            return WhitespaceMatchResult::success(whitespace_count, whitespace_indices, "optional");
            
        case PatternElement::REQUIRED_WHITESPACE:
            // Requires at least 1 whitespace token
            if (whitespace_count > 0) {
                LOG_DEBUG("REQUIRED_WHITESPACE matched {} tokens", whitespace_count);
                return WhitespaceMatchResult::success(whitespace_count, whitespace_indices, "required");
            } else {
                LOG_DEBUG("REQUIRED_WHITESPACE failed - no whitespace found");
                return WhitespaceMatchResult::failure();
            }
            
        case PatternElement::SINGLE_WHITESPACE:
            // Requires exactly 1 whitespace token
            if (whitespace_count == 1) {
                LOG_DEBUG("SINGLE_WHITESPACE matched 1 token");
                return WhitespaceMatchResult::success(1, whitespace_indices, "single");
            } else {
                LOG_DEBUG("SINGLE_WHITESPACE failed - found {} tokens, expected 1", whitespace_count);
                return WhitespaceMatchResult::failure();
            }
            
        case PatternElement::MERGED_WHITESPACE:
            // Consumes all consecutive whitespace as single unit, requires at least 1
            if (whitespace_count > 0) {
                LOG_DEBUG("MERGED_WHITESPACE matched {} tokens as single unit", whitespace_count);
                return WhitespaceMatchResult::success(whitespace_count, whitespace_indices, "merged");
            } else {
                LOG_DEBUG("MERGED_WHITESPACE failed - no whitespace found");
                return WhitespaceMatchResult::failure();
            }
            
        default:
            LOG_ERROR("Unknown whitespace pattern element: {}", static_cast<uint32_t>(whitespace_element));
            return WhitespaceMatchResult::failure();
    }
}

} // namespace cprime::layer2_contextualization