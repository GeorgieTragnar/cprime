#include "../layer2.h"
#include "../../commons/logger.h"

namespace cprime::layer2_contextualization {

// Forward declarations for helper functions
bool is_exec_execution_pattern(const Instruction& instruction);
void mark_as_exec_execution(Instruction& instruction);

bool contextualize_instruction(Instruction& body_instruction) {
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
    
    return false;  // Regular instruction, no exec processing needed
}

// Helper function to detect exec execution patterns
bool is_exec_execution_pattern(const Instruction& instruction) {
    const auto& tokens = instruction._tokens;
    if (tokens.empty()) return false;
    
    // Pattern 1: Noname exec execution - "exec { ... }"
    // Look for: EXEC + LEFT_BRACE
    if (tokens.size() >= 2 && 
        tokens[0]._token == EToken::EXEC && 
        tokens[1]._token == EToken::LEFT_BRACE) {
        return true;
    }
    
    // Pattern 2: Exec alias call - "EXEC_ALIAS<params>()"
    // Look for: EXEC_ALIAS + LESS_THAN + ... + GREATER_THAN + LEFT_PAREN + RIGHT_PAREN
    if (tokens.size() >= 4 && 
        tokens[0]._token == EToken::EXEC_ALIAS) {
        // Find template parameters and function call syntax
        for (size_t i = 1; i < tokens.size() - 1; ++i) {
            if (tokens[i]._token == EToken::LEFT_PAREN && 
                i > 0 && tokens[i-1]._token == EToken::GREATER_THAN) {
                return true;
            }
        }
    }
    
    // Pattern 3: Direct identifier exec call - "identifier<params>()"
    // Look for: IDENTIFIER + LESS_THAN + ... + GREATER_THAN + LEFT_PAREN + RIGHT_PAREN
    // Note: This requires checking if identifier exists in exec registry (will be done in process_exec_execution)
    if (tokens.size() >= 4 && 
        tokens[0]._token == EToken::IDENTIFIER &&
        tokens[1]._token == EToken::LESS_THAN) {
        // Find template parameters and function call syntax
        for (size_t i = 2; i < tokens.size() - 1; ++i) {
            if (tokens[i]._token == EToken::LEFT_PAREN && 
                i > 0 && tokens[i-1]._token == EToken::GREATER_THAN) {
                return true;  // Potential exec call (will validate in processing)
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