#include "../layer2.h"
#include "../../commons/logger.h"

namespace cprime::layer2_contextualization {

// Helper function to detect exec execution patterns in footers
bool is_footer_exec_execution_pattern(const Instruction& footer_instruction) {
    const auto& tokens = footer_instruction._tokens;
    if (tokens.empty()) return false;
    
    // Pattern 1: Noname exec footer execution - "<args>"
    // Look for: LESS_THAN + ... + GREATER_THAN (parameters for noname exec)
    // This is the primary footer execution pattern
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i]._token == EToken::LESS_THAN) {
            // Look for matching GREATER_THAN
            for (size_t j = i + 1; j < tokens.size(); ++j) {
                if (tokens[j]._token == EToken::GREATER_THAN) {
                    // Found <...> pattern - this is a footer exec execution
                    return true;
                }
            }
        }
    }
    
    // Pattern 2: Named exec alias call in footer - "EXEC_ALIAS<params>()"
    // Footer can also contain named exec alias calls
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i]._token == EToken::EXEC_ALIAS) {
            // Found exec alias in footer
            return true;
        }
    }
    
    // Pattern 3: Direct identifier exec call in footer - "identifier<params>()"
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i]._token == EToken::IDENTIFIER && i + 1 < tokens.size() &&
            tokens[i + 1]._token == EToken::LESS_THAN) {
            // Found potential exec call in footer
            return true;
        }
    }
    
    return false;
}

bool contextualize_footer(Instruction& footer_instruction) {
    auto logger = cprime::LoggerFactory::get_logger("footer_contextualization");
    
    if (footer_instruction._tokens.empty()) {
        LOG_DEBUG("Footer is empty - no contextualization needed");
        return false;  // No exec processing needed
    }
    
    LOG_DEBUG("Contextualizing footer with {} tokens", footer_instruction._tokens.size());
    
    // Check for exec execution patterns in footer
    if (is_footer_exec_execution_pattern(footer_instruction)) {
        LOG_INFO("Footer exec execution pattern detected - triggering exec processing");
        return true;  // Signal for exec processing
    }
    
    // TODO: Regular footer contextualization
    // Examples:
    // - Scope closure patterns
    // - Return statements at scope end
    // - Cleanup code before scope exit
    
    return false;  // Regular footer, no exec processing needed
}

} // namespace cprime::layer2_contextualization