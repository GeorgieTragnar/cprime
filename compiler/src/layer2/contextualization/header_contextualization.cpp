#include "../layer2.h"
#include "../../commons/logger.h"

namespace cprime::layer2_contextualization {

// Helper function to detect exec execution patterns in headers
bool is_header_exec_execution_pattern(const Instruction& header_instruction) {
    const auto& tokens = header_instruction._tokens;
    if (tokens.empty()) return false;
    
    // Pattern 1: Noname exec header execution - "<args>"
    // Look for: LESS_THAN + ... + GREATER_THAN (parameters for noname exec)
    // This is the primary noname exec execution pattern
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i]._token == EToken::LESS_THAN) {
            // Look for matching GREATER_THAN
            for (size_t j = i + 1; j < tokens.size(); ++j) {
                if (tokens[j]._token == EToken::GREATER_THAN) {
                    // Found <...> pattern - this is a noname exec execution
                    return true;
                }
            }
        }
    }
    
    // Pattern 2: Named exec alias call in header - "EXEC_ALIAS<params>()"
    // Header can also contain named exec alias calls
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i]._token == EToken::EXEC_ALIAS) {
            // Found exec alias in header
            return true;
        }
    }
    
    // Pattern 3: Direct identifier exec call in header - "identifier<params>()"
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i]._token == EToken::IDENTIFIER && i + 1 < tokens.size() &&
            tokens[i + 1]._token == EToken::LESS_THAN) {
            // Found potential exec call in header
            return true;
        }
    }
    
    return false;
}

bool contextualize_header(Instruction& header_instruction) {
    auto logger = cprime::LoggerFactory::get_logger("header_contextualization");
    
    if (header_instruction._tokens.empty()) {
        LOG_DEBUG("Header is empty - no contextualization needed");
        return false;  // No exec processing needed
    }
    
    LOG_DEBUG("Contextualizing header with {} tokens", header_instruction._tokens.size());
    
    // Check for exec execution patterns in header
    if (is_header_exec_execution_pattern(header_instruction)) {
        LOG_INFO("Header exec execution pattern detected - triggering exec processing");
        return true;  // Signal for exec processing
    }
    
    // TODO: Regular header contextualization
    // Examples:
    // - Function declarations: int main(), exec code_gen<...>
    // - Control flow headers: if (...), while (...)
    // - Class/struct declarations
    
    return false;  // Regular header, no exec processing needed
}

} // namespace cprime::layer2_contextualization