#include "orchestrator.h"
#include "layer0/input_processor.h"
#include "layer1/layer1.h"
#include "layer2/layer2.h"
#include "layer2/token_detokenizer.h"
#include "layer2validation/layer2validation.h"
#include "commons/logger.h"
#include <thread>
#include <chrono>
#include <magic_enum.hpp>

namespace cprime {

CompilerOrchestrator::CompilerOrchestrator(CompilationParameters params)
    : params_(std::move(params)), logger_(cprime::LoggerFactory::get_logger("orchestrator")) {
    
    if (!validate_parameters()) {
        throw std::runtime_error("Invalid compilation parameters provided to orchestrator");
    }
    
    auto& logger = logger_; // Alias for LOG macros
    LOG_DEBUG("CompilerOrchestrator initialized with {} input files", params_.input_files.size());
    LOG_DEBUG("Output file: {}", params_.output_file.string());
    LOG_DEBUG("Verbose: {}, Debug: {}", params_.verbose, params_.debug_mode);
}

bool CompilerOrchestrator::run() {
    log_compilation_start();
    
    // Layer 0: Input Processing
    bool success = run_layer0();
    
    // Layer 1: Tokenization
    if (success) {
        success = run_layer1();
    }
    
    // Layer 2: Structure Building  
    if (success) {
        success = run_layer2();
    }
    
    // TODO: Add Layer 3 (Contextualization)
    // TODO: Add Layer 4 (RAII injection)
    // TODO: Add ErrorHandler execution
    
    log_compilation_end(success);
    
    return success;
}

bool CompilerOrchestrator::run_layer0() {
    log_layer_start("Layer 0 (Input Processing)");
    
    auto input_streams = InputProcessor::process_input_files(params_);
    
    if (input_streams.empty()) {
        auto& logger = logger_; // Alias for LOG macros
        LOG_ERROR("Layer 0 failed: No input streams processed");
        log_layer_end("Layer 0", false);
        return false;
    }
    
    // Store input streams for Layer 1
    input_streams_ = std::move(input_streams);
    
    // Success logging with stream details
    auto& logger = logger_; // Alias for LOG macros
    LOG_INFO("Layer 0 completed: {} input streams processed", input_streams_.size());
    
    LOG_INFO("=== INPUT SOURCE BEFORE LAYER 1 TOKENIZATION ===");
    for (const auto& [stream_id, stream] : input_streams_) {
        std::string content = stream.str();
        LOG_INFO("Stream '{}': {} characters", stream_id, content.length());
        LOG_INFO("Full source content:");
        LOG_INFO("{}", content);
    }
    LOG_INFO("=== END INPUT SOURCE ===");
    
    log_layer_end("Layer 0", true);
    return true;
}

bool CompilerOrchestrator::run_layer1() {
    log_layer_start("Layer 1 (Tokenization)");
    
    if (input_streams_.empty()) {
        auto& logger = logger_; // Alias for LOG macros
        LOG_ERROR("Layer 1 failed: No input streams from Layer 0");
        log_layer_end("Layer 1", false);
        return false;
    }
    
    auto& logger = logger_; // Alias for LOG macros
    LOG_DEBUG("Starting tokenization of {} input streams", input_streams_.size());
    
    // Tokenize each input stream
    for (auto& [stream_id, stream] : input_streams_) {
        LOG_DEBUG("Tokenizing stream: {}", stream_id);
        
        try {
            // Use persistent exec alias registry for exec alias detection
            // Call the new standardized layer1 function
            auto tokens = layer1(stream, string_table_, exec_alias_registry_);
            
            LOG_INFO("Stream '{}' tokenized: {} tokens generated", stream_id, tokens.size());
            
            // Store tokens for Layer 2
            token_streams_[stream_id] = tokens;
            
            // Log some statistics
            size_t literal_count = 0, identifier_count = 0, keyword_count = 0;
            for (const auto& token : tokens) {
                switch (token._raw_token) {
                    case ERawToken::LITERAL: literal_count++; break;
                    case ERawToken::IDENTIFIER: identifier_count++; break;
                    case ERawToken::KEYWORD: keyword_count++; break;
                    default: break;
                }
            }
            
            LOG_DEBUG("  Literals: {}, Identifiers: {}, Keywords: {}", 
                     literal_count, identifier_count, keyword_count);
            
        } catch (const std::exception& e) {
            LOG_ERROR("Tokenization failed for stream '{}': {}", stream_id, e.what());
            log_layer_end("Layer 1", false);
            return false;
        }
    }
    
    LOG_INFO("Layer 1 completed: All streams tokenized successfully");
    LOG_DEBUG("String table now contains {} unique strings", string_table_.size());
    
    log_layer_end("Layer 1", true);
    return true;
}

bool CompilerOrchestrator::run_layer2() {
    log_layer_start("Layer 2 (Structure Building)");
    
    if (token_streams_.empty()) {
        auto& logger = logger_; // Alias for LOG macros
        LOG_ERROR("Layer 2 failed: No token streams from Layer 1");
        log_layer_end("Layer 2", false);
        return false;
    }
    
    auto& logger = logger_; // Alias for LOG macros
    LOG_DEBUG("Starting structure building of {} token streams", token_streams_.size());
    
    try {
        // Use persistent exec alias registry for exec alias processing
        // Call Layer 2 to build scope structure
        auto scopes = layer2(token_streams_, string_table_, exec_alias_registry_);
        
        LOG_INFO("Layer 2 completed: {} scopes built", scopes.size());
        
        // Log basic scope count (detailed type stats belong in later layers)
        LOG_DEBUG("Layer 2 Structure: {} scopes built", scopes.size());
        
        // For development: always log full scope structure  
        std::string full_structure = layer2validation::serialize_scope_vector(scopes);
        LOG_INFO("Layer 2 Full Structure:\n{}", full_structure);
        
        // Log exec scope registration statistics
        LOG_INFO("ExecAliasRegistry Statistics: {} registered exec aliases, {} registered exec scopes, {} alias-to-scope mappings", 
                exec_alias_registry_.size(), exec_alias_registry_.get_exec_scope_count(), exec_alias_registry_.get_alias_to_scope_count());
        
        // Test on-demand execution of prepared Lua scripts
        if (exec_alias_registry_.get_exec_scope_count() > 0) {
            LOG_INFO("Testing on-demand ExecutableLambda execution:");
            for (auto& [scope_index, executable_lambda] : exec_alias_registry_.get_scope_to_lambda_map()) {
                std::vector<std::string> test_params = {"int", "string", "MyClass"};
                LOG_INFO("=== ON-DEMAND EXECUTION TEST ===");
                LOG_INFO("Executing prepared Lua script for scope {}", scope_index);
                try {
                    std::string result = executable_lambda.execute(test_params, &exec_alias_registry_, scope_index);
                    LOG_INFO("Lua script returned: \"{}\"", result);
                    LOG_INFO("Return value length: {} chars", result.length());
                } catch (const std::exception& e) {
                    LOG_ERROR("Execution failed: {}", e.what());
                }
                LOG_INFO("=== END ON-DEMAND EXECUTION ===");
            }
        }
        
        // Note: Lua script detokenization happens in sublayer2b for exec blocks only
        
        // Focus on exec block processing instead of multi-script testing
        // LOG_INFO("=== MULTI-SCRIPT TESTING ===");
        // test_multiple_lua_scripts(exec_alias_registry_);
        
        // TODO: Store scopes for Layer 3
        
    } catch (const std::exception& e) {
        LOG_ERROR("Structure building failed: {}", e.what());
        log_layer_end("Layer 2", false);
        return false;
    }
    
    log_layer_end("Layer 2", true);
    return true;
}

// TODO: Implement ErrorHandler execution
// bool CompilerOrchestrator::run_error_handler() { ... }

bool CompilerOrchestrator::validate_parameters() {
    auto& logger = logger_; // Alias for LOG macros
    if (!params_.validate()) {
        LOG_ERROR("Error: Compilation parameters validation failed");
        return false;
    }
    
    // Additional orchestrator-specific validation
    for (const auto& file : params_.input_files) {
        if (file.empty()) {
            LOG_ERROR("Error: Empty file path in input files");
            return false;
        }
    }
    
    if (params_.output_file.empty()) {
        LOG_ERROR("Error: Output file path is empty");
        return false;
    }
    
    LOG_DEBUG("Compilation parameters validated successfully");
    return true;
}

// TODO: Implement proper logging system setup
// void CompilerOrchestrator::setup_logging() { ... }

void CompilerOrchestrator::log_compilation_start() {
    auto& logger = logger_; // Alias for LOG macros
    LOG_INFO("=== CPrime Compilation Started ===");
    LOG_INFO("Input files: {}", params_.input_files.size());
    
    for (const auto& file : params_.input_files) {
        LOG_INFO("  - {}", file.string());
    }
    
    LOG_INFO("Output file: {}", params_.output_file.string());
    if (params_.verbose) {
        LOG_DEBUG("Verbose: {}", params_.verbose ? "true" : "false");
        LOG_DEBUG("Debug mode: {}", params_.debug_mode ? "true" : "false");
    }
}

void CompilerOrchestrator::log_compilation_end(bool success) {
    auto& logger = logger_; // Alias for LOG macros
    if (success) {
        LOG_INFO("=== CPrime Compilation Completed Successfully ===");
    } else {
        LOG_ERROR("=== CPrime Compilation Failed ===");
    }
    
    // TODO: Log final context state when CompilationContext is implemented
}

void CompilerOrchestrator::log_layer_start(const std::string& layer_name) {
    auto& logger = logger_; // Alias for LOG macros
    LOG_DEBUG("--- Starting {} ---", layer_name);
}

void CompilerOrchestrator::log_layer_end(const std::string& layer_name, bool success) {
    auto& logger = logger_; // Alias for LOG macros
    if (success) {
        LOG_DEBUG("--- {} Completed Successfully ---", layer_name);
    } else {
        LOG_ERROR("--- {} Failed ---", layer_name);
    }
}

// TODO: Implement proper error handling system
// void CompilerOrchestrator::handle_layer_failure(...) { ... }

void CompilerOrchestrator::test_multiple_lua_scripts(ExecAliasRegistry& registry) {
    auto& logger = logger_; // Alias for LOG macros
    
    // Create three different ExecutableLambda instances with different scripts
    ExecutableLambda script1, script2, script3;
    script1.lua_script = TokenDetokenizer::get_test_script_1();
    script2.lua_script = TokenDetokenizer::get_test_script_2(); 
    script3.lua_script = TokenDetokenizer::get_test_script_3();
    
    // Define multiple different parameter sets
    std::vector<std::vector<std::string>> parameter_sets = {
        {"int", "bool", "char"},  // Small set - primitives only
        {"std::string", "MyClass", "CustomType", "template<T>"},  // Medium set - mixed types
        {"int", "float", "double", "vector<int>", "map<string,int>", "unique_ptr<T>", "shared_ptr<Data>"},  // Large set - complex types
        {"bool", "template<typename T>"},  // Minimal set - test edge cases
        {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J"}  // Many parameters - stress test
    };
    
    // Define execution order variations
    std::vector<std::vector<int>> execution_orders = {
        {1, 2, 3},    // Sequential
        {3, 1, 2},    // Reverse start
        {2, 3, 1},    // Different order
        {1, 3, 2},    // Mixed order
        {3, 2, 1}     // Full reverse
    };
    
    LOG_INFO("Testing {} scripts with {} parameter sets in {} different execution orders",
             3, parameter_sets.size(), execution_orders.size());
    
    int test_round = 1;
    for (const auto& order : execution_orders) {
        LOG_INFO("--- Test Round {} (execution order: {}-{}-{}) ---", test_round, order[0], order[1], order[2]);
        
        for (size_t param_set_idx = 0; param_set_idx < parameter_sets.size(); ++param_set_idx) {
            const auto& params = parameter_sets[param_set_idx];
            LOG_INFO("Parameter Set {}: {} parameters", param_set_idx + 1, params.size());
            
            // Execute scripts in the specified order
            for (int script_num : order) {
                ExecutableLambda* current_script = nullptr;
                std::string script_name;
                
                switch (script_num) {
                    case 1: current_script = &script1; script_name = "Type Analysis Engine"; break;
                    case 2: current_script = &script2; script_name = "Code Generator"; break;
                    case 3: current_script = &script3; script_name = "Interface Builder"; break;
                }
                
                LOG_INFO("Executing Script {}: {}", script_num, script_name);
                
                try {
                    std::string result = current_script->execute(params);
                    
                    // Extract just the return value from the full result
                    size_t return_pos = result.find("=== LUA RETURN VALUE ===");
                    std::string return_value = "No return value";
                    if (return_pos != std::string::npos) {
                        size_t start = return_pos + 26; // Length of "=== LUA RETURN VALUE ===\n"
                        size_t end = result.find('\n', start);
                        if (end != std::string::npos) {
                            return_value = result.substr(start, end - start);
                        }
                    }
                    
                    LOG_INFO("Script {} Result: {}", script_num, return_value);
                    
                } catch (const std::exception& e) {
                    LOG_ERROR("Script {} execution failed: {}", script_num, e.what());
                }
                
                // Add small delay to ensure different random seeds
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            LOG_INFO(""); // Empty line for readability
        }
        
        test_round++;
    }
    
    LOG_INFO("=== MULTI-SCRIPT TESTING COMPLETED ===");
    LOG_INFO("Total executions: {} (3 scripts × {} parameter sets × {} orders)", 
             3 * parameter_sets.size() * execution_orders.size(),
             parameter_sets.size(), execution_orders.size());
}


} // namespace cprime