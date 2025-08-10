// CPrime Compiler - Main Entry Point  
// Uses orchestrator-based architecture for coordinated compilation pipeline

#include <filesystem>
#include <exception>

#include "orchestrator.h"
#include "layer0/compilation_parameters.h"
#include "commons/logger.h"

namespace fs = std::filesystem;

void print_usage(const char* program_name, cprime::Logger& logger) {
    // TODO: Implement VersionInfo system for version string display
    logger.info("CPrime Compiler v2.0.0");
    logger.info("Usage: {} [options] <input_file>", program_name);
    logger.info("");
    logger.info("Options:");
    logger.info("  -o <file>        Output file name (default: a.out)");
    logger.info("  --verbose        Enable verbose output");
    logger.info("  --debug          Enable debug mode with detailed logging");
    logger.info("  --dump-ast       Output AST structure");
    logger.info("  --dump-ir        Output IR structure");
    logger.info("  -h, --help       Show this help message");
    logger.info("");
    logger.info("Examples:");
    logger.info("  {} hello.cprime", program_name);
    logger.info("  {} -o myprogram hello.cprime", program_name);
    logger.info("  {} --debug --dump-ast test.cprime", program_name);
}

cprime::CompilationParameters parse_arguments(int argc, char* argv[], cprime::Logger& logger) {
    cprime::CompilationParameters params;
    
    if (argc < 2) {
        print_usage(argv[0], logger);
        exit(1);
    }
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0], logger);
            exit(0);
        } else if (arg == "-o") {
            if (i + 1 < argc) {
                params.output_file = argv[++i];
            } else {
                logger.error("Error: -o requires an argument");
                exit(1);
            }
        } else if (arg == "--verbose") {
            params.verbose = true;
        } else if (arg == "--debug") {
            params.debug_mode = true;
            params.verbose = true; // Debug implies verbose
        } else if (arg == "--dump-ast") {
            params.generate_ast_dump = true;
        } else if (arg == "--dump-ir") {
            params.generate_ir_dump = true;
        } else if (arg[0] != '-') {
            // Input file
            params.input_files.emplace_back(arg);
        } else {
            logger.error("Error: Unknown option: {}", arg);
            exit(1);
        }
    }
    
    if (params.input_files.empty()) {
        logger.error("Error: No input file specified");
        print_usage(argv[0], logger);
        exit(1);
    }
    
    // Set default output file if not specified
    if (params.output_file.empty()) {
        params.output_file = "a.out";
    }
    
    return params;
}

int main(int argc, char* argv[]) {
    try {
        // Create logs directory if it doesn't exist
        fs::create_directories("logs");
        
        // Initialize logger factory early (before parsing to handle errors)
        cprime::LoggerFactory::initialize_selective_buffering();
        cprime::Logger logger = cprime::LoggerFactory::get_logger("main");
        
        // Parse command line arguments into compilation parameters
        cprime::CompilationParameters params = parse_arguments(argc, argv, logger);
        
        // Set global log level based on compilation parameters
        if (params.debug_mode) {
            cprime::LoggerFactory::set_global_level(cprime::LogLevel::Debug);
            logger.debug("Debug mode enabled - setting log level to Debug");
        } else if (params.verbose) {
            cprime::LoggerFactory::set_global_level(cprime::LogLevel::Info);
            logger.debug("Verbose mode enabled - setting log level to Info");
        } else {
            cprime::LoggerFactory::set_global_level(cprime::LogLevel::Warning);
        }
        
        logger.debug("CPrime compiler starting with {} input files", params.input_files.size());
        
        // Create orchestrator with parameters
        cprime::CompilerOrchestrator orchestrator(params);
        
        // Run compilation process
        bool success = orchestrator.run();
        
        logger.debug("Compilation {} with exit code {}", 
                    success ? "succeeded" : "failed", success ? 0 : 1);
        
        return success ? 0 : 1;
        
    } catch (const std::exception& e) {
        // Create emergency logger for fatal errors
        cprime::Logger emergency_logger = cprime::LoggerFactory::get_logger("main");
        emergency_logger.error("Fatal error: {}", e.what());
        return 2;
    } catch (...) {
        // Create emergency logger for unknown errors
        cprime::Logger emergency_logger = cprime::LoggerFactory::get_logger("main");
        emergency_logger.error("Unknown fatal error occurred");
        return 2;
    }
}