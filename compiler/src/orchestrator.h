#pragma once

#include "layer0/compilation_parameters.h"
#include "commons/compilation_context.h"
#include "commons/logger.h"
#include "commons/common_types.h"

namespace cprime {

/**
 * CompilerOrchestrator - Central coordinator for the compilation process.
 * 
 * Responsibilities:
 * - Owns all compilation data through CompilationContext
 * - Coordinates execution of all layers in sequence
 * - Manages error handling and logging across layers
 * - Provides single entry point for compilation via run() method
 * 
 * Design Philosophy:
 * - Single data owner: All compilation data flows through context
 * - Layer APIs: Each layer takes context and modifies it in-place
 * - Clean interface: Only run() is public, all layer coordination is private
 * - Incremental: Start with Layer 0, add more layers over time
 */
class CompilerOrchestrator {
public:
    /**
     * Construct orchestrator with compilation parameters.
     * Validates parameters and sets up logging.
     * 
     * @param params Compilation parameters (input files, options, etc.)
     */
    explicit CompilerOrchestrator(CompilationParameters params);
    
    /**
     * Run the complete compilation process.
     * Executes all layers in sequence, handling errors appropriately.
     * 
     * @return true if compilation succeeded, false if any layer failed
     */
    bool run();

private:
    // Configuration
    CompilationParameters params_;
    
    // Owned compilation data
    CompilationContext context_;
    
    // Logging
    Logger logger_;
    
    // Layer execution methods (private - called by run())
    bool run_layer0();  // Input processing
    bool run_layer1();  // Tokenization
    bool run_layer2();  // Structure building  
    // Future: bool run_layer3();  // Contextualization
    // Future: bool run_layer4();  // RAII injection
    
    // Orthogonal component execution
    bool run_error_handler();  // Process all collected errors
    
    // Utility methods
    bool validate_parameters();
    void setup_logging();
    void log_compilation_start();
    void log_compilation_end(bool success);
    void log_layer_start(const std::string& layer_name);
    void log_layer_end(const std::string& layer_name, bool success);
    
    // Error handling
    void handle_layer_failure(const std::string& layer_name, const std::string& error_message);
};

} // namespace cprime