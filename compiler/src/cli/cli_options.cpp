#include "cli_options.h"
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <getopt.h>

namespace cprime::cli {

void CLIOptions::validate() {
    // Check that we have input files if we're doing file-based operations
    if ((has_layer0_operations() || dump_tokens) && input_files.empty()) {
        throw std::runtime_error("Input files required for file-based operations");
    }
    
    // Validate input files exist and are readable
    for (const auto& file : input_files) {
        if (!std::filesystem::exists(file)) {
            throw std::runtime_error("Input file does not exist: " + file.string());
        }
        if (!std::filesystem::is_regular_file(file)) {
            throw std::runtime_error("Input path is not a regular file: " + file.string());
        }
    }
}

std::string CLIOptions::to_string() const {
    std::stringstream ss;
    ss << "CLIOptions{";
    
    // Layer 0 operations
    if (debug_input_processing) ss << "debug_input_processing ";
    if (analyze_streams) ss << "analyze_streams ";
    if (show_file_validation) ss << "show_file_validation ";
    
    // Future layer operations
    if (dump_tokens) ss << "dump_tokens ";
    if (debug_context) ss << "debug_context ";
    if (build_ast) ss << "build_ast ";
    if (dump_ast) ss << "dump_ast ";
    
    // General options
    if (interactive_mode) ss << "interactive ";
    if (verbose) ss << "verbose ";
    
    ss << "files:[";
    for (size_t i = 0; i < input_files.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << input_files[i].string();
    }
    ss << "]";
    
    if (!output_file.empty()) {
        ss << " output:" << output_file;
    }
    
    ss << "}";
    return ss.str();
}

CLIOptions CLIParser::parse(int argc, char* argv[]) {
    CLIOptions options;
    
    // Define long options
    static struct option long_options[] = {
        // Layer 0 debugging
        {"debug-input",         no_argument,       0, 'I'},
        {"analyze-streams",     no_argument,       0, 'S'},
        {"show-file-validation", no_argument,      0, 'V'},
        
        // Future layer options
        {"dump-tokens",         no_argument,       0, 't'},
        {"debug-context",       no_argument,       0, 'c'},
        {"build-ast",           no_argument,       0, 'a'},
        {"dump-ast",            no_argument,       0, 'A'},
        
        // General options
        {"interactive",         no_argument,       0, 'i'},
        {"verbose",             no_argument,       0, 'v'},
        {"help",                no_argument,       0, 'h'},
        {"output",              required_argument, 0, 'o'},
        
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    // Parse options
    while ((opt = getopt_long(argc, argv, "ISVtcaAivho:", long_options, &option_index)) != -1) {
        switch (opt) {
            // Layer 0 debugging
            case 'I':
                options.debug_input_processing = true;
                break;
            case 'S':
                options.analyze_streams = true;
                break;
            case 'V':
                options.show_file_validation = true;
                break;
                
            // Future layer operations
            case 't':
                options.dump_tokens = true;
                break;
            case 'c':
                options.debug_context = true;
                break;
            case 'a':
                options.build_ast = true;
                break;
            case 'A':
                options.dump_ast = true;
                break;
                
            // General options
            case 'i':
                options.interactive_mode = true;
                break;
            case 'v':
                options.verbose = true;
                break;
            case 'h':
                options.show_help = true;
                return options; // Early return for help
            case 'o':
                options.output_file = optarg;
                break;
                
            case '?':
                // getopt_long already printed an error message
                throw std::runtime_error("Invalid command line option");
            default:
                throw std::runtime_error("Unexpected option parsing result");
        }
    }
    
    // Parse remaining arguments as input files
    for (int i = optind; i < argc; ++i) {
        options.input_files.emplace_back(argv[i]);
    }
    
    return options;
}

void CLIParser::print_help(const char* program_name) {
    print_version();
    std::cout << "\n";
    std::cout << "Usage: " << program_name << " [OPTIONS] [input_files...]\n\n";
    
    std::cout << "LAYER 0 DEBUGGING (Input Processing):\n";
    std::cout << "  -I, --debug-input         Debug input file processing pipeline\n";
    std::cout << "  -S, --analyze-streams     Analyze processed stringstreams in detail\n";
    std::cout << "  -V, --show-file-validation Show file validation process\n\n";
    
    std::cout << "FUTURE LAYER DEBUGGING (Not Yet Implemented):\n";
    std::cout << "  -t, --dump-tokens         Dump raw tokens (Layer 1)\n";
    std::cout << "  -c, --debug-context       Debug context resolution (Layer 2)\n";
    std::cout << "  -a, --build-ast           Build AST structure (Layer 3)\n";
    std::cout << "  -A, --dump-ast            Show AST structure (Layer 3)\n\n";
    
    std::cout << "GENERAL OPTIONS:\n";
    std::cout << "  -i, --interactive         Interactive debugging mode (not implemented)\n";
    std::cout << "  -v, --verbose             Enable verbose debug output\n";
    std::cout << "  -o, --output FILE         Write output to FILE (default: stdout)\n";
    std::cout << "  -h, --help                Show this help message\n\n";
    
    std::cout << "EXAMPLES:\n";
    std::cout << "  # Debug input processing for a single file\n";
    std::cout << "  " << program_name << " --debug-input examples/hello.cprime\n\n";
    
    std::cout << "  # Analyze processed streams with verbose output\n";
    std::cout << "  " << program_name << " -I -S -v examples/hello.cprime examples/simple.cp\n\n";
    
    std::cout << "  # Show file validation details only\n";
    std::cout << "  " << program_name << " --show-file-validation examples/*.cprime\n\n";
    
    std::cout << "  # Combined Layer 0 analysis\n";
    std::cout << "  " << program_name << " -I -S -V examples/class_test.cprime\n\n";
    
    std::cout << "INPUT FILES:\n";
    std::cout << "  Supports .cp and .cprime file extensions\n";
    std::cout << "  Multiple files can be analyzed simultaneously\n";
    std::cout << "  Files must be readable and exist\n\n";
    
    std::cout << "LAYER 0 FOCUS:\n";
    std::cout << "  Layer 0 handles input file processing - converting files to stringstreams\n";
    std::cout << "  This is the foundation for all subsequent compilation layers\n";
    std::cout << "  Debug capabilities help understand file processing and stream creation\n";
}

void CLIParser::print_version() {
    // TODO: Integrate with proper version system
    std::cout << "CPrime Development CLI v2.0.0 - Layer 0 Debug Edition";
}

void CLIParser::handle_unknown_option(const std::string& option) {
    std::cerr << "Unknown option: " << option << "\n";
    std::cerr << "Use --help for usage information\n";
}

} // namespace cprime::cli