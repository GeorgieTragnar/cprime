#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace cprime::cli {

/**
 * CLIOptions - Clean command line option parsing for cprime_cli
 * 
 * Handles parsing and validation of command line arguments for the
 * CPrime development CLI tool.
 */
struct CLIOptions {
    // Layer 0 debugging options
    bool debug_input_processing = false;  // Show detailed input file processing
    bool analyze_streams = false;         // Analyze processed stringstreams
    bool show_file_validation = false;    // Show file validation details
    
    // Future layer options (placeholder)
    bool dump_tokens = false;             // Layer 1: Show tokenization
    bool debug_context = false;           // Layer 2: Show context resolution
    bool build_ast = false;               // Layer 3: Build AST
    bool dump_ast = false;                // Layer 3: Show AST structure
    
    // General options
    bool interactive_mode = false;        // Interactive REPL mode
    bool show_help = false;               // Show help message
    bool verbose = false;                 // Verbose output
    
    // Input/Output
    std::vector<std::filesystem::path> input_files;
    std::string output_file;              // Empty = stdout
    
    // Utility methods
    bool has_layer0_operations() const {
        return debug_input_processing || analyze_streams || show_file_validation;
    }
    
    bool has_any_operations() const {
        return has_layer0_operations() || dump_tokens || debug_context || 
               build_ast || dump_ast || interactive_mode;
    }
    
    void validate();
    std::string to_string() const;
};

/**
 * CLIParser - Command line argument parser
 */
class CLIParser {
public:
    static CLIOptions parse(int argc, char* argv[]);
    static void print_help(const char* program_name);
    static void print_version();

private:
    static void handle_unknown_option(const std::string& option);
};

} // namespace cprime::cli