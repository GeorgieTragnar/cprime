// CPrime Compiler - Main Entry Point  
// Uses orchestrator-based architecture for coordinated compilation pipeline

#include <filesystem>
#include <exception>
#include <spdlog/spdlog.h>

#include "orchestrator.h"
#include "layer0/compilation_parameters.h"
#include "commons/logger.h"

namespace fs = std::filesystem;

void print_usage(const char* program_name, cprime::Logger& logger) {
    // TODO: Implement VersionInfo system for version string display
    LOG_INFO("CPrime Compiler v2.0.0");
    LOG_INFO("Usage: {} [options] <input_file>", program_name);
    LOG_INFO("");
    LOG_INFO("Options:");
    LOG_INFO("  -o <file>        Output file name (default: a.out)");
    LOG_INFO("  --verbose        Enable verbose output");
    LOG_INFO("  --debug          Enable debug mode with detailed logging");
    LOG_INFO("  --dump-ast       Output AST structure");
    LOG_INFO("  --dump-ir        Output IR structure");
    LOG_INFO("  -h, --help       Show this help message");
    LOG_INFO("");
    LOG_INFO("Examples:");
    LOG_INFO("  {} hello.cprime", program_name);
    LOG_INFO("  {} -o myprogram hello.cprime", program_name);
    LOG_INFO("  {} --debug --dump-ast test.cprime", program_name);
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
                LOG_ERROR("Error: -o requires an argument");
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
            LOG_ERROR("Error: Unknown option: {}", arg);
            exit(1);
        }
    }
    
    if (params.input_files.empty()) {
        LOG_ERROR("Error: No input file specified");
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
            // Also set spdlog to trace level for maximum verbosity
            spdlog::set_level(spdlog::level::trace);
            LOG_DEBUG("Debug mode enabled - setting log level to Debug/Trace");
        } else if (params.verbose) {
            cprime::LoggerFactory::set_global_level(cprime::LogLevel::Info);
            LOG_DEBUG("Verbose mode enabled - setting log level to Info");
        } else {
            cprime::LoggerFactory::set_global_level(cprime::LogLevel::Warning);
        }
        
        LOG_DEBUG("CPrime compiler starting with {} input files", params.input_files.size());
        
        // Create orchestrator with parameters
        cprime::CompilerOrchestrator orchestrator(params);
        
        // Run compilation process
        bool success = orchestrator.run();
        
        LOG_DEBUG("Compilation {} with exit code {}", 
                    success ? "succeeded" : "failed", success ? 0 : 1);
        
        return success ? 0 : 1;
        
    } catch (const std::exception& e) {
        // Create emergency logger for fatal errors
        auto logger = cprime::LoggerFactory::get_logger("main");
        LOG_ERROR("Fatal error: {}", e.what());
        return 2;
    } catch (...) {
        // Create emergency logger for unknown errors
        auto logger = cprime::LoggerFactory::get_logger("main");
        LOG_ERROR("Unknown fatal error occurred");
        return 2;
    }
}