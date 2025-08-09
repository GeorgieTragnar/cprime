// CPrime Compiler - Main Entry Point
// This is the main compiler executable that uses the cprime_compiler library
// to compile CPrime source files into executables.

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <filesystem>

#include "common/logger.h"
#include "common/logger_components.h"
#include "common/common_types.h"
#include "layer1/raw_token.h"
#include "layer1/context_stack.h"
#include "layer2/contextual_token.h"
#include "layer3/ast_builder.h"

namespace fs = std::filesystem;

void print_usage(const char* program_name) {
    std::cout << cprime::VersionInfo::get_full_version_string() << "\n";
    std::cout << "Usage: " << program_name << " [options] <input_file>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -o <file>        Output file name (default: a.out)\n";
    std::cout << "  -c               Compile only, don't link\n";
    std::cout << "  --verbose        Enable verbose output\n";
    std::cout << "  --dump-tokens    Output tokenization results\n";
    std::cout << "  --dump-ast       Output AST structure\n";
    std::cout << "  -h, --help       Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " hello.cprime\n";
    std::cout << "  " << program_name << " -o myprogram hello.cprime\n";
    std::cout << "  " << program_name << " --dump-ast -o test test.cprime\n";
}

cprime::CompilerOptions parse_arguments(int argc, char* argv[]) {
    cprime::CompilerOptions options;
    
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
                options.output_file = argv[++i];
            } else {
                std::cerr << "Error: -o requires an argument\n";
                exit(1);
            }
        } else if (arg == "-c") {
            // Note: compile_only not in CompilerOptions, will be handled locally
            // options.compile_only = true;
        } else if (arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "--dump-tokens") {
            // Note: dump_tokens functionality will be handled by verbose logger output
            options.verbose = true;
        } else if (arg == "--dump-ast") {
            options.generate_ast_dump = true;
        } else if (arg[0] != '-') {
            if (options.input_file.empty()) {
                options.input_file = arg;
            } else {
                std::cerr << "Error: Multiple input files not supported\n";
                exit(1);
            }
        } else {
            std::cerr << "Error: Unknown option: " << arg << "\n";
            exit(1);
        }
    }
    
    if (options.input_file.empty()) {
        std::cerr << "Error: No input file specified\n";
        print_usage(argv[0]);
        exit(1);
    }
    
    // Set default output file if not specified
    if (options.output_file.empty()) {
        options.output_file = "a.out";
    }
    
    return options;
}

std::string read_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file: " << filename << "\n";
        exit(1);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool compile(const cprime::CompilerOptions& options) {
    using namespace cprime;
    
    // Initialize logger for the compiler
    auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_COMPILER);
    
    if (options.verbose) {
        logger->set_level(spdlog::level::debug);
        
        // Test buffer functionality in verbose mode
        std::cout << "\n=== TESTING BUFFER FUNCTIONALITY ===\n";
        
        // Start buffering debug messages for tokenizer
        CPRIME_BUFFER_BEGIN_DEBUG(CPRIME_COMPONENT_TOKENIZER);
        
        std::cout << "Tokenizer buffer active: " << CPRIME_BUFFER_IS_ACTIVE(CPRIME_COMPONENT_TOKENIZER) << "\n";
    } else {
        logger->set_level(spdlog::level::warn);
    }
    
    CPRIME_LOG_INFO("Starting compilation of {}", options.input_file);
    CPRIME_LOG_DEBUG("Output file: {}", options.output_file);
    
    // Read source file
    std::string source = read_file(options.input_file);
    
    // Layer 1: Tokenization
    // Use tokenizer component logger for this section
    auto tokenizer_logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_TOKENIZER);
    if (options.verbose) {
        tokenizer_logger->set_level(spdlog::level::debug);
    }
    
    CPRIME_LOG_DEBUG("Layer 1: Starting tokenization");
    
    RawTokenizer tokenizer(source);
    auto tokens = tokenizer.tokenize();
    
    // This will be logged with tokenizer logger
    tokenizer_logger->debug("Tokenization complete, {} tokens generated", tokens.size());
    
    if (options.verbose) {
        tokenizer_logger->debug("=== Tokens ===");
        for (const auto& token : tokens) {
            tokenizer_logger->debug("  {}", token.to_string());
        }
        tokenizer_logger->debug("=== End Tokens ===");
        
        // Test buffer functionality
        std::cout << "Buffer count after tokenization: " << CPRIME_BUFFER_COUNT(CPRIME_COMPONENT_TOKENIZER) << " messages\n";
    }
    
    if (tokens.empty()) {
        CPRIME_LOG_ERROR("No tokens found in source file");
        
        // Demo error case - dump tokenizer buffer
        if (options.verbose && CPRIME_BUFFER_IS_ACTIVE(CPRIME_COMPONENT_TOKENIZER)) {
            std::cout << "\n=== ERROR CONDITION - DUMPING TOKENIZER BUFFER ===\n";
            CPRIME_BUFFER_DUMP(CPRIME_COMPONENT_TOKENIZER);
        }
        
        return false;
    }
    
    // Layer 2: Context enrichment
    CPRIME_LOG_DEBUG("Layer 2: Starting context enrichment");
    
    // Create ContextualTokenStream with context resolution
    std::vector<ContextualToken> contextual_tokens;
    ContextStack context_stack;
    
    for (const auto& token : tokens) {
        // For now, use GLOBAL context for all tokens
        // A full implementation would properly track context
        contextual_tokens.push_back(ContextualToken(token, ParseContextType::TopLevel));
    }
    
    ContextualTokenStream contextual_stream(contextual_tokens);
    CPRIME_LOG_DEBUG("Context enrichment complete, {} contextual tokens created", contextual_tokens.size());
    
    // Layer 3: AST Building
    CPRIME_LOG_DEBUG("Layer 3: Starting AST building");
    
    ASTBuilder ast_builder;
    auto ast = ast_builder.build(contextual_stream);
    
    if (!ast) {
        CPRIME_LOG_ERROR("Failed to build AST");
        auto errors = ast_builder.get_errors();
        if (!errors.empty()) {
            for (const auto& error : errors) {
                CPRIME_LOG_ERROR("  AST Error: {}", error.message);
            }
        }
        return false;
    }
    
    CPRIME_LOG_DEBUG("AST building complete, {} declarations found", ast->get_declarations().size());
    
    if (options.generate_ast_dump) {
        CPRIME_LOG_DEBUG("=== AST ===");
        CPRIME_LOG_DEBUG("{}", ast->to_string());
        CPRIME_LOG_DEBUG("=== End AST ===");
    }
    
    // TODO: Layer 4: RAII Injection
    // TODO: Layer 5: Code Generation (LLVM IR)
    
    // For now, we'll create a stub executable that just prints a message
    // This will be replaced with LLVM IR generation
    
    CPRIME_LOG_DEBUG("Code generation (stub implementation)");
    
    // Generate a simple C++ file as a temporary solution
    std::string temp_cpp = options.output_file + ".cpp";
    std::ofstream cpp_file(temp_cpp);
    if (!cpp_file.is_open()) {
        CPRIME_LOG_ERROR("Cannot create temporary C++ file: {}", temp_cpp);
        return false;
    }
    
    // Generate stub C++ code
    cpp_file << "// Generated from: " << options.input_file << "\n";
    cpp_file << "#include <iostream>\n\n";
    cpp_file << "int main() {\n";
    cpp_file << "    std::cout << \"CPrime compiled program (stub)\\n\";\n";
    cpp_file << "    std::cout << \"Source: " << options.input_file << "\\n\";\n";
    cpp_file << "    std::cout << \"AST nodes: " << ast->get_declarations().size() << "\\n\";\n";
    cpp_file << "    return 0;\n";
    cpp_file << "}\n";
    cpp_file.close();
    
    // Compile the C++ file to create the executable
    std::string compile_cmd = "g++ -o " + options.output_file + " " + temp_cpp + " 2>/dev/null";
    int result = std::system(compile_cmd.c_str());
    
    // Clean up temporary file
    std::remove(temp_cpp.c_str());
    
    if (result != 0) {
        CPRIME_LOG_ERROR("Failed to generate executable");
        return false;
    }
    
    CPRIME_LOG_INFO("Compilation successful: {}", options.output_file);
    
    // Clean up buffer at end of compilation
    if (options.verbose && CPRIME_BUFFER_IS_ACTIVE(CPRIME_COMPONENT_TOKENIZER)) {
        std::cout << "\n=== CLEANING UP BUFFERS ===\n";
        CPRIME_BUFFER_END(CPRIME_COMPONENT_TOKENIZER);
        CPRIME_BUFFER_CLEAR(CPRIME_COMPONENT_TOKENIZER);
        std::cout << "Tokenizer buffer cleared. Final buffer count: " << CPRIME_BUFFER_COUNT(CPRIME_COMPONENT_TOKENIZER) << "\n";
    }
    
    return true;
}

int main(int argc, char* argv[]) {
    // Create logs directory if it doesn't exist
    std::filesystem::create_directories("logs");
    
    cprime::CompilerOptions options = parse_arguments(argc, argv);
    
    bool success = compile(options);
    
    return success ? 0 : 1;
}