#include "../layer2.h"
#include "../../commons/logger.h"
#include "../../commons/contextualizationError.h"

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
    
    // TODO: Regular contextualization - populate body_instruction._contextualTokens
    // All needed data is in the _tokens vector already
    // Examples:
    // - Variable declarations: int a = 1;
    // - Assignments: a = b + c;
    // - Function calls: print("hello");
    // - Expressions: calculations, operations
    // - Control flow statements: break, continue, return
    
    // For now, generate INVALID contextual tokens for all non-exec instruction patterns
    // This demonstrates the error reporting system
    if (!body_instruction._tokens.empty()) {
        // Collect token indices for error reporting
        std::vector<uint32_t> token_indices;
        for (size_t i = 0; i < body_instruction._tokens.size(); ++i) {
            token_indices.push_back(body_instruction._tokens[i]._tokenIndex);
        }
        
        // Report unsupported pattern error
        report_error(ContextualizationErrorType::UNSUPPORTED_TOKEN_PATTERN,
                    "Instruction contextualization not yet implemented for this pattern",
                    token_indices);
        
        // Generate INVALID contextual tokens for all tokens
        body_instruction._contextualTokens.clear();
        for (const auto& token : body_instruction._tokens) {
            ContextualToken ctx_token;
            ctx_token._contextualToken = EContextualToken::INVALID;
            ctx_token._parentTokenIndices.push_back(token._tokenIndex);
            body_instruction._contextualTokens.push_back(ctx_token);
        }
    }
    
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