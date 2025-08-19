#include "base_contextualizer.h"
#include "context_pattern_elements.h"
#include "../../commons/logger.h"
#include "../../commons/enum/token.h"
#include <algorithm>
#include <cassert>

namespace cprime::layer2_contextualization {

template<typename PatternElementType>
void BaseContextualizer<PatternElementType>::register_pattern(const BaseContextualizationPattern<PatternElementType>& pattern) {
    auto logger = cprime::LoggerFactory::get_logger("base_contextualizer");
    
    // Insert pattern in priority order (higher priority first)
    auto insert_pos = std::upper_bound(patterns_.begin(), patterns_.end(), pattern,
        [](const BaseContextualizationPattern<PatternElementType>& a, const BaseContextualizationPattern<PatternElementType>& b) {
            return a.priority > b.priority;
        });
    
    patterns_.insert(insert_pos, pattern);
    
    LOG_DEBUG("Registered pattern '{}' with priority {} (pattern #{} of {})", 
              pattern.pattern_name, pattern.priority, 
              std::distance(patterns_.begin(), insert_pos) + 1, patterns_.size());
}

template<typename PatternElementType>
std::vector<ContextualToken> BaseContextualizer<PatternElementType>::contextualize(const std::vector<Token>& tokens) {
    auto logger = cprime::LoggerFactory::get_logger("base_contextualizer");
    
    if (tokens.empty()) {
        LOG_DEBUG("Empty token sequence - no contextualization needed");
        return {};
    }
    
    LOG_DEBUG("Contextualizing with {} tokens using {} patterns", 
              tokens.size(), patterns_.size());
    
    std::vector<ContextualToken> result;
    size_t pos = 0;
    
    while (pos < tokens.size()) {
        bool pattern_found = false;
        
        // Skip whitespace tokens (spaces, newlines) - they don't contribute to contextualization unless explicitly matched
        if (is_whitespace_token(tokens[pos])) {
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

template<typename PatternElementType>
PatternMatchResult BaseContextualizer<PatternElementType>::try_match_pattern(const std::vector<Token>& tokens, 
                                                                            size_t start_pos, 
                                                                            const BaseContextualizationPattern<PatternElementType>& pattern) {
    auto logger = cprime::LoggerFactory::get_logger("base_contextualizer");
    
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
        PatternElementType element = pattern.token_pattern[pattern_pos];
        
        // Check if this is a whitespace pattern element
        if (is_whitespace_pattern_element(element)) {
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
        
        LOG_DEBUG("Generated contextual token type {} referencing {} source tokens", 
                  static_cast<uint32_t>(ctx_token._contextualToken), 
                  ctx_token._parentTokenIndices.size());
    }
    
    size_t tokens_consumed = token_pos - start_pos;
    LOG_DEBUG("Pattern '{}' successfully matched, consuming {} tokens", 
              pattern.pattern_name, tokens_consumed);
    
    return PatternMatchResult::success(tokens_consumed, contextual_tokens);
}

template<typename PatternElementType>
bool BaseContextualizer<PatternElementType>::is_whitespace_token(const Token& token) {
    return token._token == EToken::SPACE || 
           token._token == EToken::NEWLINE ||
           token._token == EToken::TAB ||
           token._token == EToken::CARRIAGE_RETURN;
}

template<typename PatternElementType>
WhitespaceMatchResult BaseContextualizer<PatternElementType>::try_match_whitespace_pattern(const std::vector<Token>& tokens,
                                                                                          size_t start_pos,
                                                                                          PatternElementType whitespace_element) {
    auto logger = cprime::LoggerFactory::get_logger("base_contextualizer");
    
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
    
    // Cast to base pattern element for whitespace comparison
    uint32_t element_value = static_cast<uint32_t>(whitespace_element);
    
    if (element_value == static_cast<uint32_t>(BasePatternElement::OPTIONAL_WHITESPACE)) {
        // Always succeeds, consumes 0 or more whitespace tokens
        LOG_DEBUG("OPTIONAL_WHITESPACE matched {} tokens", whitespace_count);
        return WhitespaceMatchResult::success(whitespace_count, whitespace_indices, "optional");
    } else if (element_value == static_cast<uint32_t>(BasePatternElement::REQUIRED_WHITESPACE)) {
        // Requires at least 1 whitespace token
        if (whitespace_count > 0) {
            LOG_DEBUG("REQUIRED_WHITESPACE matched {} tokens", whitespace_count);
            return WhitespaceMatchResult::success(whitespace_count, whitespace_indices, "required");
        } else {
            LOG_DEBUG("REQUIRED_WHITESPACE failed - no whitespace found");
            return WhitespaceMatchResult::failure();
        }
    } else if (element_value == static_cast<uint32_t>(BasePatternElement::SINGLE_WHITESPACE)) {
        // Requires exactly 1 whitespace token
        if (whitespace_count == 1) {
            LOG_DEBUG("SINGLE_WHITESPACE matched 1 token");
            return WhitespaceMatchResult::success(1, whitespace_indices, "single");
        } else {
            LOG_DEBUG("SINGLE_WHITESPACE failed - found {} tokens, expected 1", whitespace_count);
            return WhitespaceMatchResult::failure();
        }
    } else if (element_value == static_cast<uint32_t>(BasePatternElement::MERGED_WHITESPACE)) {
        // Consumes all consecutive whitespace as single unit, requires at least 1
        if (whitespace_count > 0) {
            LOG_DEBUG("MERGED_WHITESPACE matched {} tokens as single unit", whitespace_count);
            return WhitespaceMatchResult::success(whitespace_count, whitespace_indices, "merged");
        } else {
            LOG_DEBUG("MERGED_WHITESPACE failed - no whitespace found");
            return WhitespaceMatchResult::failure();
        }
    }
    
    LOG_ERROR("Unknown whitespace pattern element: {}", element_value);
    return WhitespaceMatchResult::failure();
}

template<typename PatternElementType>
ContextualToken BaseContextualizer<PatternElementType>::create_contextual_token(const ContextualTokenTemplate& token_template,
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

template<typename PatternElementType>
bool BaseContextualizer<PatternElementType>::is_whitespace_pattern_element(PatternElementType element) {
    uint32_t element_value = static_cast<uint32_t>(element);
    
    return element_value == static_cast<uint32_t>(BasePatternElement::OPTIONAL_WHITESPACE) ||
           element_value == static_cast<uint32_t>(BasePatternElement::REQUIRED_WHITESPACE) ||
           element_value == static_cast<uint32_t>(BasePatternElement::SINGLE_WHITESPACE) ||
           element_value == static_cast<uint32_t>(BasePatternElement::MERGED_WHITESPACE);
}

// Explicit template instantiation for the types we'll use
template class BaseContextualizer<BasePatternElement>;

// Explicit template instantiations for the pattern element types
template class BaseContextualizer<cprime::layer2_contextualization::HeaderPatternElement>;
template class BaseContextualizer<cprime::layer2_contextualization::FooterPatternElement>;
template class BaseContextualizer<cprime::layer2_contextualization::InstructionPatternElement>;

} // namespace cprime::layer2_contextualization