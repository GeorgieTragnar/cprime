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
    if (scope._footer._tokens.empty()) {
        LOG_INFO("footer: EMPTY");
    } else {
        std::string footer_desc = create_instruction_description(scope._footer);
        LOG_INFO("footer: {}", footer_desc);
    }
}

// Sublayer 2C: Sequential instruction iteration and contextualization
void sublayer2c(std::vector<Scope>& scopes, 
                const StringTable& /* string_table */,
                const std::map<std::string, std::vector<RawToken>>& /* streams */) {
    
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
        layer2_contextualization::contextualize_header(scope._header);
        
        // Log scope body start
        LOG_INFO("body:");
        
        // Iterate through all instructions in the scope
        for (size_t instr_index = 0; instr_index < scope._instructions.size(); ++instr_index) {
            auto& instruction_variant = scope._instructions[instr_index];  // Mutable reference
            
            if (std::holds_alternative<Instruction>(instruction_variant)) {
                Instruction& instruction = std::get<Instruction>(instruction_variant);  // Mutable reference
                log_instruction(instruction, logger);
                // Contextualize body instruction
                layer2_contextualization::contextualize_instruction(instruction);
            } else if (std::holds_alternative<uint32_t>(instruction_variant)) {
                // No contextualization for nested scope references
                uint32_t nested_scope_index = std::get<uint32_t>(instruction_variant);
                LOG_INFO("nested scope: {}", nested_scope_index);
            }
        }
        
        // Log scope footer
        log_scope_footer(scope, logger);
        // Contextualize scope footer
        layer2_contextualization::contextualize_footer(scope._footer);
        
        // Add blank line for readability between scopes
        LOG_INFO("");
    }
    
    LOG_INFO("=== Sublayer 2C Complete ===");
}

} // namespace cprime::layer2_sublayers