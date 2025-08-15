#include "cli_options.h"
#include "../commons/logger.h"
#include "../layer0/compilation_parameters.h"
#include "../layer0validation/input_debug.h"
#include "../layer0validation/stream_inspector.h"
#include "../layer0/input_processor.h"
#include "../layer1/layer1.h"
#include "../layer1validation/layer1validation.h"
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
        
        // Handle future layer operations
        if (options.dump_tokens) {
            LOG_INFO("=== CPrime CLI - Layer 1 Token Dumping ===");
            
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
            
            // Process input files to get streams
            auto input_streams = InputProcessor::process_input_files(params);
            
            if (input_streams.empty()) {
                LOG_ERROR("No input streams processed for token dumping");
                return 1;
            }
            
            // Create string table for Layer 1
            StringTable string_table;
            
            // Process each stream through Layer 1 and dump tokens
            for (auto& [stream_id, stream] : input_streams) {
                LOG_INFO("Tokenizing stream: {}", stream_id);
                
                try {
                    // Run Layer 1 tokenization
                    auto tokens = layer1(stream, string_table);
                    
                    LOG_INFO("Generated {} tokens for stream '{}'", tokens.size(), stream_id);
                    
                    // Use existing serialization from layer1validation
                    std::string serialized_output = cprime::layer1_sublayers::validation::serialize(tokens);
                    
                    // Output to file or console
                    if (!options.output_file.empty()) {
                        std::ofstream out_file(options.output_file);
                        if (out_file.is_open()) {
                            out_file << serialized_output;  // No trailing newline for file output
                            LOG_INFO("Token output written to: {}", options.output_file);
                        } else {
                            LOG_ERROR("Failed to open output file: {}", options.output_file);
                            return 1;
                        }
                    } else {
                        // Output to console
                        std::cout << serialized_output << std::endl;  // Keep newline for console
                    }
                    
                } catch (const std::exception& e) {
                    LOG_ERROR("Token dumping failed for stream '{}': {}", stream_id, e.what());
                    return 1;
                }
            }
            
            LOG_INFO("Token dumping completed successfully");
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