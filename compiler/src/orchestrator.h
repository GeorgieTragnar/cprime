#pragma once

#include "layer0/compilation_parameters.h"
#include "commons/dirty/string_table.h"
#include "commons/rawToken.h"
#include "commons/logger.h"
#include <map>
#include <sstream>

namespace cprime {

/**
 * CompilerOrchestrator - Central coordinator for the compilation process.
 * 
 * Responsibilities:
 * - Coordinates execution of layers in sequence
 * - Manages logging across layers
 * - Provides single entry point for compilation via run() method
 * 
 * Design Philosophy:
 * - Incremental: Start with Layer 0, add more layers over time
 * - Simple interface: Only run() is public
 * - TODO: Implement CompilationContext data ownership and management
 * - TODO: Implement proper error handling system
 */
class CompilerOrchestrator {
public:
    /**
     * Construct orchestrator with compilation parameters.
     * 
     * @param params Compilation parameters (input files, options, etc.)
     */
    explicit CompilerOrchestrator(CompilationParameters params);
    
    /**
     * Run the complete compilation process.
     * Currently only executes Layer 0 (input processing).
     * 
     * @return true if compilation succeeded, false if any layer failed
     */
    bool run();

private:
    // Configuration
    CompilationParameters params_;
    
    // Logger for orchestrator component
    cprime::Logger logger_;
    
    // String table for interning strings across all layers
    StringTable string_table_;
    
    // Input streams from Layer 0 (passed to Layer 1)
    std::map<std::string, std::stringstream> input_streams_;
    
    // Token data from Layer 1 (passed to Layer 2)
    std::map<std::string, std::vector<RawToken>> token_streams_;
    
    // TODO: Add CompilationContext ownership and management
    // TODO: Add proper error handling system
    
    // Layer execution methods (private - called by run())
    bool run_layer0();  // Input processing
    bool run_layer1();  // Tokenization
    bool run_layer2();  // Structure building
    
    // TODO: Add future layer methods
    // bool run_layer3();  // Contextualization
    // bool run_layer4();  // RAII injection
    
    // TODO: Add error handler execution
    // bool run_error_handler();  // Process all collected errors
    
    // Utility methods
    bool validate_parameters();
    void log_compilation_start();
    void log_compilation_end(bool success);
    void log_layer_start(const std::string& layer_name);
    void log_layer_end(const std::string& layer_name, bool success);
};

} // namespace cprime