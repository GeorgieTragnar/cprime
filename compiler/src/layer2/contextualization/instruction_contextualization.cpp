#include "../layer2.h"
#include "../../commons/logger.h"
#include "../../commons/contextualizationError.h"
#include "contextualization_pattern_matcher.h"
#include <algorithm>

namespace cprime::layer2_contextualization {

// Forward declarations for helper functions
bool is_exec_execution_pattern(const Instruction& instruction);
void mark_as_exec_execution(Instruction& instruction);

bool contextualize_instruction(Instruction& body_instruction, ErrorReporter report_error) {
    auto logger = cprime::LoggerFactory::get_logger("instruction_contextualization");
    
    if (body_instruction._tokens.empty()) {
        LOG_DEBUG("Instruction is empty - no contextualization needed");
        return false;  // No exec processing needed
    }
    
    LOG_DEBUG("Contextualizing instruction with {} tokens", body_instruction._tokens.size());
    
    // Check for exec execution patterns (single pass only)
    if (is_exec_execution_pattern(body_instruction)) {
        LOG_INFO("Exec execution pattern detected - triggering exec processing");
        mark_as_exec_execution(body_instruction);
        return true;  // Signal for exec processing
    }
    
    // Try pattern-based body instruction contextualization
    LOG_INFO("🎯 Attempting pattern-based body instruction contextualization with {} tokens", body_instruction._tokens.size());
    for (size_t i = 0; i < body_instruction._tokens.size(); ++i) {
        LOG_INFO("  Token[{}]: {}", i, static_cast<int>(body_instruction._tokens[i]._token));
    }
    
    ContextualizationPatternMatcher& pattern_matcher = ContextualizationPatternMatcher::getInstance();
    LOG_INFO("Got pattern matcher instance, {} body patterns available", pattern_matcher.get_body_pattern_count());
    
    PatternMatchResult match_result = pattern_matcher.match_body_pattern(body_instruction);
    LOG_INFO("Pattern matching result: success = {}", match_result.success);
    
    if (match_result.success) {
        LOG_INFO("Body pattern matched: {}", match_result.matched_pattern->pattern_name);
        LOG_INFO("Generated {} contextual tokens", match_result.contextual_tokens.size());
        
        // Apply the contextual tokens to the body instruction
        for (const auto& contextual_result : match_result.contextual_tokens) {
            ContextualToken ctx_token;
            ctx_token._contextualToken = contextual_result.contextual_token;
            // Convert size_t indices to uint32_t
            for (size_t idx : contextual_result.token_indices) {
                ctx_token._parentTokenIndices.push_back(static_cast<uint32_t>(idx));
            }
            body_instruction._contextualTokens.push_back(ctx_token);
            
            LOG_INFO("Added contextual token {} with {} token indices", 
                     static_cast<int>(contextual_result.contextual_token),
                     contextual_result.token_indices.size());
        }
        
        return false;  // Successful pattern matching, no exec processing needed
    }
    
    // No pattern matched - report error
    LOG_INFO("No body pattern matched for this instruction");
    report_error(ContextualizationErrorType::UNSUPPORTED_TOKEN_PATTERN,
                "Some token patterns not yet implemented in instruction contextualization",
                {0}); // Placeholder error position
    
    return false;  // Regular instruction, no exec processing needed
}

// Helper function to detect exec execution patterns
bool is_exec_execution_pattern(const Instruction& instruction) {
    const auto& tokens = instruction._tokens;
    if (tokens.empty()) return false;
    
    // Scan through all tokens to find exec patterns (skip whitespace/comments)
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        
        // Pattern 1: Noname exec execution - "exec { ... }"
        // Look for: EXEC + LEFT_BRACE
        if (token._token == EToken::EXEC && i + 1 < tokens.size()) {
            // Look for LEFT_BRACE after EXEC (may have whitespace in between)
            for (size_t j = i + 1; j < tokens.size(); ++j) {
                if (tokens[j]._token == EToken::LEFT_BRACE) {
                    return true;
                }
                // Stop if we hit non-whitespace/non-space tokens
                if (tokens[j]._token != EToken::SPACE && 
                    tokens[j]._token != EToken::NEWLINE) {
                    break;
                }
            }
        }
        
        // Pattern 2: Exec alias call - "EXEC_ALIAS<params>()"
        // Look for: EXEC_ALIAS + LESS_THAN + ... + GREATER_THAN + LEFT_PAREN + RIGHT_PAREN
        if (token._token == EToken::EXEC_ALIAS) {
            // Find template parameters and function call syntax
            for (size_t j = i + 1; j < tokens.size() - 1; ++j) {
                if (tokens[j]._token == EToken::LEFT_PAREN && 
                    j > 0 && tokens[j-1]._token == EToken::GREATER_THAN) {
                    return true;
                }
            }
        }
        
        // Pattern 3: Direct identifier exec call - "identifier<params>()"
        // Look for: IDENTIFIER + LESS_THAN + ... + GREATER_THAN + LEFT_PAREN + RIGHT_PAREN
        // Note: This requires checking if identifier exists in exec registry (will be done in process_exec_execution)
        if (token._token == EToken::IDENTIFIER && i + 1 < tokens.size() &&
            tokens[i + 1]._token == EToken::LESS_THAN) {
            // Find template parameters and function call syntax
            for (size_t j = i + 2; j < tokens.size() - 1; ++j) {
                if (tokens[j]._token == EToken::LEFT_PAREN && 
                    j > 0 && tokens[j-1]._token == EToken::GREATER_THAN) {
                    return true;  // Potential exec call (will validate in processing)
                }
            }
        }
    }
    
    return false;
}

// Helper function to mark instruction as exec execution
void mark_as_exec_execution(Instruction& /* instruction */) {
    // TODO: Populate _contextualTokens with exec execution metadata
    // For now, just log that it's marked
    auto logger = cprime::LoggerFactory::get_logger("instruction_contextualization");
    LOG_DEBUG("Marked instruction as exec execution for processing");
}

} // namespace cprime::layer2_contextualization