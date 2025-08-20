#include "layer2.h"
#include "../commons/logger.h"
#include "../commons/errorHandler.h"
#include "../commons/contextualizationError.h"
#include "contextual_registration_extractor.h"
#include <sstream>
#include <stdexcept>

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

// Helper function to validate no CHUNK tokens remain
void validate_no_chunk_tokens(const std::vector<Scope>& scopes) {
    auto logger = cprime::LoggerFactory::get_logger("sublayer2d");
    
    uint32_t chunk_count = 0;
    
    for (size_t scope_index = 0; scope_index < scopes.size(); ++scope_index) {
        const Scope& scope = scopes[scope_index];
        
        // Check header tokens
        for (const auto& token : scope._header._tokens) {
            if (token._token == EToken::CHUNK) {
                chunk_count++;
                LOG_ERROR("CHUNK token found in scope {} header at token index {}", scope_index, token._tokenIndex);
            }
        }
        
        // Check instruction tokens
        for (size_t instr_index = 0; instr_index < scope._instructions.size(); ++instr_index) {
            const auto& instruction_variant = scope._instructions[instr_index];
            if (std::holds_alternative<Instruction>(instruction_variant)) {
                const Instruction& instruction = std::get<Instruction>(instruction_variant);
                for (const auto& token : instruction._tokens) {
                    if (token._token == EToken::CHUNK) {
                        chunk_count++;
                        LOG_ERROR("CHUNK token found in scope {} instruction {} at token index {}", 
                                 scope_index, instr_index, token._tokenIndex);
                    }
                }
            }
        }
        
        // Check footer tokens
        if (std::holds_alternative<Instruction>(scope._footer)) {
            const Instruction& footer = std::get<Instruction>(scope._footer);
            for (const auto& token : footer._tokens) {
                if (token._token == EToken::CHUNK) {
                    chunk_count++;
                    LOG_ERROR("CHUNK token found in scope {} footer at token index {}", scope_index, token._tokenIndex);
                }
            }
        }
    }
    
    if (chunk_count > 0) {
        LOG_ERROR("CRITICAL ERROR: {} CHUNK tokens found in sublayer2d input. All CHUNK tokens should have been resolved in sublayer2c.", chunk_count);
        throw std::runtime_error("CHUNK tokens found in sublayer2d - disambiguation failed");
    } else {
        LOG_INFO("Validation passed: No CHUNK tokens found in input");
    }
}

// Forward declarations are in layer2.h, no need to redeclare here
using namespace cprime::layer2_contextualization;

// Sublayer 2D: Sequential instruction iteration and contextualization
std::vector<Scope> sublayer2d(const std::vector<Scope>& input_scopes, 
                              const StringTable& string_table,
                              const std::map<std::string, std::vector<RawToken>>& streams,
                              ExecAliasRegistry& exec_registry,
                              ErrorHandler& error_handler,
                              TypeRegistry& type_registry,
                              FunctionRegistry& function_registry) {
    
    // Create a mutable copy of the input scopes for processing
    std::vector<Scope> scopes = input_scopes;
    
    auto logger = cprime::LoggerFactory::get_logger("sublayer2d");
    
    LOG_INFO("=== Sublayer 2D: Instruction Contextualization ===");
    LOG_INFO("Processing {} scopes sequentially", scopes.size());
    
    // Validate that no CHUNK tokens remain (should have been resolved in sublayer2c)
    validate_no_chunk_tokens(scopes);
    
    // Sequential iteration through all scopes
    for (size_t scope_index = 0; scope_index < scopes.size(); ++scope_index) {
        Scope& scope = scopes[scope_index];  // Mutable reference
        
        LOG_INFO("Processing scope {}:", scope_index);
        
        // Log scope header
        log_scope_header(scope, logger);
        
        // Create ErrorReporter lambda for header contextualization
        auto report_header_error = [&](ContextualizationErrorType error_type,
                                       const std::string& extra_info,
                                       const std::vector<uint32_t>& token_indices) {
            ContextualizationError error;
            error.error_type = error_type;
            error.extra_info = extra_info;
            error.token_indices = token_indices;
            error.scope_index = static_cast<uint32_t>(scope_index);
            error.instruction_index = 0; // Header is always instruction 0
            error.instruction_type = InstructionType::HEADER;
            error_handler.register_contextualization_error(error);
        };
        
        // Contextualize scope header
        bool header_needs_exec = contextualize_header(scope._header, report_header_error);
        if (header_needs_exec) {
            // Extract header exec alias information
            HeaderExecAliasInfo exec_info = extract_header_exec_alias_info(scope._header, scope_index);
            
            if (exec_info.is_header_exec) {
                LOG_INFO("Header exec alias registration - creating namespaced alias");
                LOG_INFO("Base alias: '{}', Namespaced alias components: {}", 
                         exec_info.base_alias_name, exec_info.namespace_and_alias.size());
                
                // Register the new namespaced alias in the registry
                ExecAliasIndex new_alias_index = exec_registry.register_namespaced_alias(exec_info.namespace_and_alias);
                
                // TODO: Store prefilled parameters and scope content requirement
                // TODO: Link new alias to base executable lambda
                
                LOG_INFO("Registered namespaced alias with index: {}", new_alias_index.value);
                LOG_INFO("Header exec alias registration completed successfully");
            } else {
                // Fallback to old execution logic for backward compatibility
                LOG_INFO("exec execution detected in header - processing...");
                
                // Use enhanced exec processing to get both scope index and exec result
                ExecProcessingResult exec_processing_result = process_exec_execution_with_result(
                    scope._header, scopes, string_table, exec_registry, streams, scope_index, true);
                
                if (!exec_processing_result.is_valid) {
                    LOG_ERROR("Header exec processing failed for scope {}", scope_index);
                } else {
                    LOG_INFO("header exec execution: generated scope {}", exec_processing_result.generated_scope_index);
                    
                    // NEW: Handle post-exec re-contextualization for header
                    handle_post_exec_recontextualization(
                        scopes, scope_index, 0, InstructionType::HEADER, 
                        exec_processing_result, string_table, streams, 
                        exec_registry, error_handler);
                    
                    LOG_INFO("Header exec processing completed - scope generated successfully with re-contextualization");
                }
            }
        }
        
        // Log scope body start
        LOG_INFO("body:");
        
        // Iterate through all instructions in the scope
        for (size_t instr_index = 0; instr_index < scope._instructions.size(); ++instr_index) {
            auto& instruction_variant = scope._instructions[instr_index];  // Mutable reference
            
            if (std::holds_alternative<Instruction>(instruction_variant)) {
                Instruction& instruction = std::get<Instruction>(instruction_variant);  // Mutable reference
                log_instruction(instruction, logger);
                
                // Create ErrorReporter lambda for body instruction contextualization
                auto report_instruction_error = [&](ContextualizationErrorType error_type,
                                                    const std::string& extra_info,
                                                    const std::vector<uint32_t>& token_indices) {
                    ContextualizationError error;
                    error.error_type = error_type;
                    error.extra_info = extra_info;
                    error.token_indices = token_indices;
                    error.scope_index = static_cast<uint32_t>(scope_index);
                    error.instruction_index = static_cast<uint32_t>(instr_index);
                    error.instruction_type = InstructionType::BODY;
                    error_handler.register_contextualization_error(error);
                };
                
                // Contextualize body instruction and check for exec processing
                bool needs_exec_processing = contextualize_instruction(instruction, report_instruction_error);
                
                if (needs_exec_processing) {
                    LOG_INFO("exec execution detected - processing...");
                    
                    // Store original scope count for tracking new scopes
                    size_t original_scope_count = scopes.size();
                    
                    // Use enhanced exec processing to get both scope index and exec result
                    ExecProcessingResult exec_processing_result = process_exec_execution_with_result(
                        instruction, scopes, string_table, exec_registry, streams, scope_index, false);
                    
                    if (!exec_processing_result.is_valid) {
                        LOG_ERROR("Exec processing failed for instruction in scope {}", scope_index);
                        continue; // Skip to next instruction
                    }
                    
                    // Replace instruction with scope reference if needed (scope_insert case)
                    if (exec_processing_result.exec_result.integration_type == "scope_insert") {
                        instruction_variant = exec_processing_result.generated_scope_index;
                        LOG_INFO("exec execution: replaced with generated scope {}", exec_processing_result.generated_scope_index);
                    } else {
                        LOG_INFO("exec execution: completed with integration type '{}', generated scope {}", 
                                 exec_processing_result.exec_result.integration_type, exec_processing_result.generated_scope_index);
                    }
                    
                    // NEW: Handle post-exec re-contextualization
                    handle_post_exec_recontextualization(
                        scopes, scope_index, instr_index, InstructionType::BODY, 
                        exec_processing_result, string_table, streams, 
                        exec_registry, error_handler);
                    
                    LOG_INFO("Post-exec re-contextualization completed for instruction {}", instr_index);
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
            
            // Create ErrorReporter lambda for footer contextualization
            auto report_footer_error = [&](ContextualizationErrorType error_type,
                                           const std::string& extra_info,
                                           const std::vector<uint32_t>& token_indices) {
                ContextualizationError error;
                error.error_type = error_type;
                error.extra_info = extra_info;
                error.token_indices = token_indices;
                error.scope_index = static_cast<uint32_t>(scope_index);
                error.instruction_index = static_cast<uint32_t>(scope._instructions.size()); // Footer is after all body instructions
                error.instruction_type = InstructionType::FOOTER;
                error_handler.register_contextualization_error(error);
            };
            
            bool footer_needs_exec = contextualize_footer(footer_instruction, report_footer_error);
            
            if (footer_needs_exec) {
                LOG_INFO("exec execution detected in footer - processing...");
                
                // Use enhanced exec processing to get both scope index and exec result
                ExecProcessingResult exec_processing_result = process_exec_execution_with_result(
                    footer_instruction, scopes, string_table, exec_registry, streams, scope_index, false);
                
                if (!exec_processing_result.is_valid) {
                    LOG_ERROR("Footer exec processing failed for scope {}", scope_index);
                } else {
                    // Replace footer instruction with scope reference if needed (scope_insert case)
                    if (exec_processing_result.exec_result.integration_type == "scope_insert") {
                        scope._footer = exec_processing_result.generated_scope_index;
                        LOG_INFO("footer exec execution: replaced footer with generated scope {}", exec_processing_result.generated_scope_index);
                    } else {
                        LOG_INFO("footer exec execution: completed with integration type '{}', generated scope {}", 
                                 exec_processing_result.exec_result.integration_type, exec_processing_result.generated_scope_index);
                    }
                    
                    // NEW: Handle post-exec re-contextualization for footer
                    handle_post_exec_recontextualization(
                        scopes, scope_index, scope._instructions.size(), InstructionType::FOOTER, 
                        exec_processing_result, string_table, streams, 
                        exec_registry, error_handler);
                    
                    LOG_INFO("Footer exec processing completed with re-contextualization");
                }
            }
        } else {
            // Footer is already a scope index (previously replaced)
            uint32_t footer_scope_index = std::get<uint32_t>(scope._footer);
            LOG_INFO("footer already references scope {}", footer_scope_index);
        }
        
        // Add blank line for readability between scopes
        LOG_INFO("");
    }
    
    // After all contextualization is complete, extract and register types and functions
    LOG_INFO("=== Post-Contextualization Registration Extraction ===");
    
    // Note: We need to cast the const StringTable to mutable for the extractor
    // This is safe because the extractor only interns new strings, not modifying existing ones
    StringTable& mutable_string_table = const_cast<StringTable&>(string_table);
    
    ContextualRegistrationExtractor extractor(type_registry, function_registry, mutable_string_table);
    extractor.extract_and_register_from_scopes(scopes, streams);
    
    LOG_INFO("=== Registration Extraction Complete ===");
    LOG_INFO("=== Sublayer 2D Complete ===");
    return scopes;
}

} // namespace cprime::layer2_sublayers