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

#include "layer1/raw_token.h"
#include "layer1/context_stack.h"
#include "layer2/contextual_token.h"
#include "layer3/ast_builder.h"

namespace fs = std::filesystem;

struct CompilerOptions {
    std::string input_file;
    std::string output_file;
    bool verbose = false;
    bool dump_tokens = false;
    bool dump_ast = false;
    bool compile_only = false;  // -c flag
};

void print_usage(const char* program_name) {
    std::cout << "CPrime Compiler v2.0\n";
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

CompilerOptions parse_arguments(int argc, char* argv[]) {
    CompilerOptions options;
    
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
            options.compile_only = true;
        } else if (arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "--dump-tokens") {
            options.dump_tokens = true;
        } else if (arg == "--dump-ast") {
            options.dump_ast = true;
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
        if (options.compile_only) {
            // For -c, replace extension with .o
            size_t dot_pos = options.input_file.find_last_of('.');
            if (dot_pos != std::string::npos) {
                options.output_file = options.input_file.substr(0, dot_pos) + ".o";
            } else {
                options.output_file = options.input_file + ".o";
            }
        } else {
            options.output_file = "a.out";
        }
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

bool compile(const CompilerOptions& options) {
    using namespace cprime;
    
    if (options.verbose) {
        std::cout << "Compiling: " << options.input_file << "\n";
        std::cout << "Output: " << options.output_file << "\n";
    }
    
    // Read source file
    std::string source = read_file(options.input_file);
    
    // Layer 1: Tokenization
    if (options.verbose) {
        std::cout << "Layer 1: Tokenizing...\n";
    }
    
    RawTokenizer tokenizer(source);
    auto tokens = tokenizer.tokenize();
    
    if (options.dump_tokens) {
        std::cout << "=== Tokens ===\n";
        for (const auto& token : tokens) {
            std::cout << token.to_string() << "\n";
        }
        std::cout << "=== End Tokens ===\n";
    }
    
    if (tokens.empty()) {
        std::cerr << "Error: No tokens found in source file\n";
        return false;
    }
    
    // Layer 2: Context enrichment
    if (options.verbose) {
        std::cout << "Layer 2: Context enrichment...\n";
    }
    
    // Create ContextualTokenStream with context resolution
    std::vector<ContextualToken> contextual_tokens;
    ContextStack context_stack;
    
    for (const auto& token : tokens) {
        // For now, use GLOBAL context for all tokens
        // A full implementation would properly track context
        contextual_tokens.push_back(ContextualToken(token, ParseContextType::TopLevel));
    }
    
    ContextualTokenStream contextual_stream(contextual_tokens);
    
    // Layer 3: AST Building
    if (options.verbose) {
        std::cout << "Layer 3: Building AST...\n";
    }
    
    ASTBuilder ast_builder;
    auto ast = ast_builder.build(contextual_stream);
    
    if (!ast) {
        std::cerr << "Failed to build AST\n";
        auto errors = ast_builder.get_errors();
        if (!errors.empty()) {
            for (const auto& error : errors) {
                std::cerr << "  " << error.message << "\n";
            }
        }
        return false;
    }
    
    if (options.dump_ast) {
        std::cout << "=== AST ===\n";
        std::cout << ast->to_string();
        std::cout << "=== End AST ===\n";
    }
    
    // TODO: Layer 4: RAII Injection
    // TODO: Layer 5: Code Generation (LLVM IR)
    
    // For now, we'll create a stub executable that just prints a message
    // This will be replaced with LLVM IR generation
    
    if (options.verbose) {
        std::cout << "Code generation (stub)...\n";
    }
    
    // Generate a simple C++ file as a temporary solution
    std::string temp_cpp = options.output_file + ".cpp";
    std::ofstream cpp_file(temp_cpp);
    if (!cpp_file.is_open()) {
        std::cerr << "Error: Cannot create temporary C++ file\n";
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
        std::cerr << "Error: Failed to generate executable\n";
        return false;
    }
    
    if (options.verbose) {
        std::cout << "Compilation successful: " << options.output_file << "\n";
    }
    
    return true;
}

int main(int argc, char* argv[]) {
    CompilerOptions options = parse_arguments(argc, argv);
    
    bool success = compile(options);
    
    return success ? 0 : 1;
}