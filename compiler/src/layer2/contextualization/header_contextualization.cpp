#include "../layer2.h"
#include "../../commons/logger.h"
#include "../../commons/contextualizationError.h"

namespace cprime::layer2_contextualization {

// Extract header exec alias information for namespace registration
HeaderExecAliasInfo extract_header_exec_alias_info(const Instruction& header_instruction, uint32_t scope_index) {
    HeaderExecAliasInfo info;
    const auto& tokens = header_instruction._tokens;
    
    if (tokens.empty()) {
        return info; // Not a header exec
    }
    
    // Look for pattern: identifier<params> (e.g., scope_analyzer<detailed>)
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i]._token == EToken::IDENTIFIER && i + 1 < tokens.size() &&
            tokens[i + 1]._token == EToken::LESS_THAN) {
            
            info.is_header_exec = true;
            
            // For now, use placeholder values with scope index for uniqueness
            // Proper token detokenization will be implemented later when we have StringTable access
            info.base_alias_name = "header_exec_scope" + std::to_string(scope_index);
            
            // Count parameters between < > for now (actual extraction needs StringTable access)
            size_t start_param = i + 1; // LESS_THAN position
            size_t end_param = start_param;
            
            // Find matching GREATER_THAN
            for (size_t j = start_param + 1; j < tokens.size(); ++j) {
                if (tokens[j]._token == EToken::GREATER_THAN) {
                    end_param = j;
                    break;
                }
            }
            
            // Count parameters for placeholder generation
            size_t param_count = 0;
            for (size_t k = start_param + 1; k < end_param; ++k) {
                if (tokens[k]._token == EToken::IDENTIFIER || 
                    tokens[k]._token == EToken::STRING_LITERAL ||
                    tokens[k]._token == EToken::INT_LITERAL) {
                    param_count++;
                }
            }
            
            // Generate placeholder parameters
            for (size_t p = 0; p < param_count; ++p) {
                info.template_parameters.push_back("param" + std::to_string(p));
            }
            
            // Build namespace path: [namespace..., alias_name, param0, param1, ...]
            // For now, using global scope - namespace context tracking will be added later
            info.namespace_and_alias.push_back(info.base_alias_name);
            for (const auto& param : info.template_parameters) {
                info.namespace_and_alias.push_back(param);
            }
            
            return info;
        }
        
        // Also check for EXEC_ALIAS token pattern
        if (tokens[i]._token == EToken::EXEC_ALIAS && i + 1 < tokens.size() &&
            tokens[i + 1]._token == EToken::LESS_THAN) {
            
            info.is_header_exec = true;
            // Placeholder for EXEC_ALIAS with scope index for uniqueness
            info.base_alias_name = "exec_alias_scope" + std::to_string(scope_index);
            
            // Count parameters similar to IDENTIFIER case
            size_t start_param = i + 1; // LESS_THAN position
            size_t end_param = start_param;
            
            // Find matching GREATER_THAN
            for (size_t j = start_param + 1; j < tokens.size(); ++j) {
                if (tokens[j]._token == EToken::GREATER_THAN) {
                    end_param = j;
                    break;
                }
            }
            
            // Count parameters for placeholder generation
            size_t param_count = 0;
            for (size_t k = start_param + 1; k < end_param; ++k) {
                if (tokens[k]._token == EToken::IDENTIFIER || 
                    tokens[k]._token == EToken::STRING_LITERAL ||
                    tokens[k]._token == EToken::INT_LITERAL) {
                    param_count++;
                }
            }
            
            // Generate placeholder parameters
            for (size_t p = 0; p < param_count; ++p) {
                info.template_parameters.push_back("alias_param" + std::to_string(p));
            }
            
            // Build namespace path for EXEC_ALIAS
            info.namespace_and_alias.push_back(info.base_alias_name);
            for (const auto& param : info.template_parameters) {
                info.namespace_and_alias.push_back(param);
            }
            
            return info;
        }
    }
    
    return info; // Not a header exec
}

// Helper function to detect exec execution patterns in headers
bool is_header_exec_execution_pattern(const Instruction& header_instruction) {
    const auto& tokens = header_instruction._tokens;
    if (tokens.empty()) return false;
    
    // Pattern 1: Noname exec header execution - \"<args>\"
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
    
    // Pattern 2: Named exec alias call in header - \"EXEC_ALIAS<params>()\"
    // Header can also contain named exec alias calls
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i]._token == EToken::EXEC_ALIAS) {
            // Found exec alias in header
            return true;
        }
    }
    
    // Pattern 3: Direct identifier exec call in header - \"identifier<params>()\"
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i]._token == EToken::IDENTIFIER && i + 1 < tokens.size() &&
            tokens[i + 1]._token == EToken::LESS_THAN) {
            // Found potential exec call in header
            return true;
        }
    }
    
    return false;
}

bool contextualize_header(Instruction& header_instruction, ErrorReporter report_error) {
    auto logger = cprime::LoggerFactory::get_logger("header_contextualization");
    
    if (header_instruction._tokens.empty()) {
        LOG_DEBUG("Header is empty - no contextualization needed");
        return false;  // No exec processing needed
    }
    
    LOG_DEBUG("Contextualizing header with {} tokens", header_instruction._tokens.size());
    
    // Extract header exec alias information
    HeaderExecAliasInfo exec_info = extract_header_exec_alias_info(header_instruction);
    
    if (exec_info.is_header_exec) {
        LOG_INFO("Header exec alias pattern detected - will register namespaced alias");
        LOG_INFO("Base alias: '{}', Template params: {}", exec_info.base_alias_name, exec_info.template_parameters.size());
        
        // Signal that this header needs special processing (alias registration, not execution)
        return true;  
    }
    
    // Check for old-style exec execution patterns (backward compatibility)
    if (is_header_exec_execution_pattern(header_instruction)) {
        LOG_INFO("Header exec execution pattern detected - triggering exec processing");
        return true;  // Signal for exec processing
    }
    
    // TODO: Pattern-based header contextualization will be implemented here
    LOG_DEBUG("Header contextualization not yet implemented for this pattern");
    report_error(ContextualizationErrorType::UNSUPPORTED_TOKEN_PATTERN,
                "Header contextualization not yet implemented for this pattern",
                {0}); // Placeholder error position
    
    return false;  // Regular header, no exec processing needed
}

} // namespace cprime::layer2_contextualization