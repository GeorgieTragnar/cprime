#include "layer2.h"
#include "../commons/logger.h"
#include <sstream>

namespace cprime::layer2_sublayers {

// Helper function to create a simple description of an instruction
std::string create_instruction_description(const Instruction& instruction) {
    if (instruction._tokens.empty()) {
        return "EMPTY";
    }
    return std::to_string(instruction._tokens.size()) + " tokens";
}

// Helper function to log scope header
void log_scope_header(const Scope& scope, cprime::Logger& logger) {
    if (scope._header._tokens.empty()) {
        LOG_INFO("header: EMPTY");
    } else {
        std::string header_desc = create_instruction_description(scope._header);
        LOG_INFO("header: {}", header_desc);
    }
}

// Helper function to log individual instruction
void log_instruction(const Instruction& instruction, cprime::Logger& logger) {
    std::string instr_desc = create_instruction_description(instruction);
    LOG_INFO("instruction: {}", instr_desc);
}

// Helper function to log scope footer
void log_scope_footer(const Scope& scope, cprime::Logger& logger) {
    if (std::holds_alternative<Instruction>(scope._footer)) {
        const Instruction& footer_instruction = std::get<Instruction>(scope._footer);
        if (footer_instruction._tokens.empty()) {
            LOG_INFO("footer: EMPTY");
        } else {
            std::string footer_desc = create_instruction_description(footer_instruction);
            LOG_INFO("footer: {}", footer_desc);
        }
    } else if (std::holds_alternative<uint32_t>(scope._footer)) {
        uint32_t footer_scope_index = std::get<uint32_t>(scope._footer);
        LOG_INFO("footer: NESTED_SCOPE[{}]", footer_scope_index);
    }
}

// Forward declarations are in layer2.h, no need to redeclare here
using namespace cprime::layer2_contextualization;

// Sublayer 2C: Sequential instruction iteration and contextualization
void sublayer2c(std::vector<Scope>& scopes, 
                const StringTable& string_table,
                const std::map<std::string, std::vector<RawToken>>& streams,
                ExecAliasRegistry& exec_registry) {
    
    auto logger = cprime::LoggerFactory::get_logger("sublayer2c");
    
    LOG_INFO("=== Sublayer 2C: Instruction Contextualization ===");
    LOG_INFO("Processing {} scopes sequentially", scopes.size());
    
    // Sequential iteration through all scopes
    for (size_t scope_index = 0; scope_index < scopes.size(); ++scope_index) {
        Scope& scope = scopes[scope_index];  // Mutable reference
        
        LOG_INFO("Processing scope {}:", scope_index);
        
        // Log scope header
        log_scope_header(scope, logger);
        // Contextualize scope header
        bool header_needs_exec = contextualize_header(scope._header);
        if (header_needs_exec) {
            LOG_INFO("exec execution detected in header - processing...");
            
            uint32_t generated_scope_index = process_exec_execution(
                scope._header, scopes, string_table, exec_registry, streams, scope_index, true);
            
            // For header execution, this typically means the current scope should
            // be replaced with or modified to reference the generated code.
            // For noname exec headers, this is the primary execution pattern.
            LOG_INFO("header exec execution: generated scope {}", generated_scope_index);
            LOG_INFO("Header exec processing completed - scope generated successfully");
        }
        
        // Log scope body start
        LOG_INFO("body:");
        
        // Iterate through all instructions in the scope
        for (size_t instr_index = 0; instr_index < scope._instructions.size(); ++instr_index) {
            auto& instruction_variant = scope._instructions[instr_index];  // Mutable reference
            
            if (std::holds_alternative<Instruction>(instruction_variant)) {
                Instruction& instruction = std::get<Instruction>(instruction_variant);  // Mutable reference
                log_instruction(instruction, logger);
                
                // Contextualize body instruction and check for exec processing
                bool needs_exec_processing = contextualize_instruction(instruction);
                
                if (needs_exec_processing) {
                    LOG_INFO("exec execution detected - processing...");
                    
                    uint32_t generated_scope_index = process_exec_execution(
                        instruction, scopes, string_table, exec_registry, streams, scope_index, false);
                    
                    // Replace instruction with scope reference to generated code
                    instruction_variant = generated_scope_index;
                    LOG_INFO("exec execution: replaced with generated scope {}", generated_scope_index);
                }
            } else if (std::holds_alternative<uint32_t>(instruction_variant)) {
                // No contextualization for nested scope references
                uint32_t nested_scope_index = std::get<uint32_t>(instruction_variant);
                LOG_INFO("nested scope: {}", nested_scope_index);
            }
        }
        
        // Log scope footer
        log_scope_footer(scope, logger);
        
        // Contextualize scope footer (only if it's an instruction)
        if (std::holds_alternative<Instruction>(scope._footer)) {
            Instruction& footer_instruction = std::get<Instruction>(scope._footer);
            bool footer_needs_exec = contextualize_footer(footer_instruction);
            
            if (footer_needs_exec) {
                LOG_INFO("exec execution detected in footer - processing...");
                
                uint32_t generated_scope_index = process_exec_execution(
                    footer_instruction, scopes, string_table, exec_registry, streams, scope_index, false);
                
                // Replace footer instruction with scope reference (core semantic change!)
                scope._footer = generated_scope_index;
                LOG_INFO("footer exec execution: replaced footer with generated scope {}", generated_scope_index);
            }
        } else {
            // Footer is already a scope index (previously replaced)
            uint32_t footer_scope_index = std::get<uint32_t>(scope._footer);
            LOG_INFO("footer already references scope {}", footer_scope_index);
        }
        
        // Add blank line for readability between scopes
        LOG_INFO("");
    }
    
    LOG_INFO("=== Sublayer 2C Complete ===");
}

} // namespace cprime::layer2_sublayers