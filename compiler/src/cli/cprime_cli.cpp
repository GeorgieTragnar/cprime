#include "cli_options.h"
#include "../commons/logger.h"
#include "../layer0/compilation_parameters.h"
#include "../layer0validation/input_debug.h"
#include "../layer0validation/stream_inspector.h"
#include <iostream>
#include <fstream>

using namespace cprime;
using namespace cprime::cli;
using namespace cprime::layer0validation;

/**
 * CPrime CLI - Development debugging tool
 * 
 * This CLI provides layer-by-layer debugging capabilities for the CPrime compiler.
 * Currently supports Layer 0 (input processing) debugging with plans for future layers.
 */

int main(int argc, char* argv[]) {
    try {
        // Initialize logger system
        LoggerFactory::initialize_selective_buffering();
        auto logger = LoggerFactory::get_logger("cli");
        
        // Parse command line options
        CLIOptions options = CLIParser::parse(argc, argv);
        
        // Handle help request
        if (options.show_help) {
            CLIParser::print_help(argv[0]);
            return 0;
        }
        
        // Validate options
        try {
            options.validate();
        } catch (const std::exception& e) {
            LOG_ERROR("Invalid options: {}", e.what());
            return 1;
        }
        
        LOG_DEBUG("CLI started with options: {}", options.to_string());
        
        // Handle interactive mode
        if (options.interactive_mode) {
            LOG_ERROR("Interactive mode not yet implemented");
            return 1;
        }
        
        // Ensure we have operations to perform
        if (!options.has_any_operations()) {
            LOG_ERROR("No operations specified. Use --help for usage information");
            return 1;
        }
        
        // Handle Layer 0 operations
        if (options.has_layer0_operations()) {
            LOG_INFO("=== CPrime CLI - Layer 0 Debug Analysis ===");
            
            // Create compilation parameters from input files
            CompilationParameters params;
            params.input_files = options.input_files;
            params.output_file = "debug_output"; // Not used for debugging
            params.debug_mode = true;
            params.verbose = options.verbose;
            
            // Set appropriate log level
            if (options.verbose) {
                LoggerFactory::set_global_level(LogLevel::Debug);
            } else {
                LoggerFactory::set_global_level(LogLevel::Info);
            }
            
            // Perform Layer 0 debugging operations
            if (options.debug_input_processing || options.analyze_streams) {
                LOG_INFO("Running Layer 0 input processing debug analysis...");
                
                // Use InputDebug for detailed processing analysis
                auto debug_streams = InputDebug::debug_process_input_files(params, logger);
                
                if (options.analyze_streams && !debug_streams.empty()) {
                    LOG_INFO("Performing detailed stream analysis...");
                    StreamInspector::analyze_stream_collection(debug_streams, logger);
                }
                
                if (debug_streams.empty()) {
                    LOG_ERROR("No streams were successfully processed");
                    return 1;
                }
            }
            
            if (options.show_file_validation) {
                LOG_INFO("Showing file validation details...");
                for (const auto& file : options.input_files) {
                    InputDebug::debug_file_validation(file, logger);
                }
            }
        }
        
        // Handle future layer operations (placeholder)
        if (options.dump_tokens) {
            LOG_ERROR("Token dumping (Layer 1) not yet implemented");
            return 1;
        }
        
        if (options.debug_context) {
            LOG_ERROR("Context debugging (Layer 2) not yet implemented");
            return 1;
        }
        
        if (options.build_ast || options.dump_ast) {
            LOG_ERROR("AST operations (Layer 3) not yet implemented");
            return 1;
        }
        
        LOG_INFO("CLI analysis completed successfully");
        return 0;
        
    } catch (const std::exception& e) {
        // Emergency error handling without logger dependency
        std::cerr << "Fatal CLI error: " << e.what() << std::endl;
        return 2;
    } catch (...) {
        std::cerr << "Unknown fatal CLI error occurred" << std::endl;
        return 2;
    }
}