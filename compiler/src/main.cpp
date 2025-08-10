// CPrime Compiler - Main Entry Point  
// Uses orchestrator-based architecture for coordinated compilation pipeline

#include <iostream>
#include <filesystem>
#include <exception>

#include "orchestrator.h"
#include "layer0/compilation_parameters.h"
#include "commons/common_types.h"

namespace fs = std::filesystem;

void print_usage(const char* program_name) {
    std::cout << cprime::VersionInfo::get_full_version_string() << "\n";
    std::cout << "Usage: " << program_name << " [options] <input_file>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -o <file>        Output file name (default: a.out)\n";
    std::cout << "  --verbose        Enable verbose output\n";
    std::cout << "  --debug          Enable debug mode with detailed logging\n";
    std::cout << "  --dump-ast       Output AST structure\n";
    std::cout << "  --dump-ir        Output IR structure\n";
    std::cout << "  -h, --help       Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " hello.cprime\n";
    std::cout << "  " << program_name << " -o myprogram hello.cprime\n";
    std::cout << "  " << program_name << " --debug --dump-ast test.cprime\n";
}

cprime::CompilationParameters parse_arguments(int argc, char* argv[]) {
    cprime::CompilationParameters params;
    
    if (argc < 2) {
        print_usage(argv[0]);
        exit(1);
    }
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            exit(0);
        } else if (arg == "-o") {
            if (i + 1 < argc) {
                params.output_file = argv[++i];
            } else {
                std::cerr << "Error: -o requires an argument\n";
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
            std::cerr << "Error: Unknown option: " << arg << "\n";
            exit(1);
        }
    }
    
    if (params.input_files.empty()) {
        std::cerr << "Error: No input file specified\n";
        print_usage(argv[0]);
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
        
        // Parse command line arguments into compilation parameters
        cprime::CompilationParameters params = parse_arguments(argc, argv);
        
        // Create orchestrator with parameters
        cprime::CompilerOrchestrator orchestrator(params);
        
        // Run compilation process
        bool success = orchestrator.run();
        
        return success ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 2;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 2;
    }
}