#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <unistd.h>

namespace fs = std::filesystem;

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " <input.cprime> [options]\n";
    std::cerr << "Options:\n";
    std::cerr << "  -o <output>    Specify output file (default: a.out)\n";
    std::cerr << "  --emit-llvm    Output LLVM IR instead of executable\n";
    std::cerr << "  -h, --help     Show this help message\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Parse command line arguments
    std::string input_file;
    std::string output_file = "a.out";
    bool emit_llvm = false;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-o" && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg == "--emit-llvm") {
            emit_llvm = true;
        } else if (arg[0] != '-') {
            if (input_file.empty()) {
                input_file = arg;
            } else {
                std::cerr << "Error: Multiple input files specified\n";
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown option " << arg << "\n";
            return 1;
        }
    }
    
    if (input_file.empty()) {
        std::cerr << "Error: No input file specified\n";
        print_usage(argv[0]);
        return 1;
    }
    
    try {
        // Read input file
        std::ifstream file(input_file);
        if (!file) {
            std::cerr << "Error: Cannot open file " << input_file << "\n";
            return 1;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();
        
        // Compile
        std::cout << "Compiling " << input_file << "...\n";
        
        // Lexical analysis
        cprime::Lexer lexer(source);
        auto tokens = lexer.tokenize();
        
        // Parsing
        cprime::Parser parser(std::move(tokens));
        auto ast = parser.parse();
        
        // Code generation
        cprime::CodeGenerator codegen;
        codegen.generate(*ast);
        
        if (emit_llvm) {
            // Output LLVM IR
            std::string ir_file = output_file.empty() ? "output.ll" : output_file;
            codegen.write_ir_to_file(ir_file);
            std::cout << "Generated LLVM IR: " << ir_file << "\n";
        } else {
            // Compile to executable
            // First generate LLVM IR to temp file
            std::string temp_ir = "/tmp/cprime_" + std::to_string(getpid()) + ".ll";
            codegen.write_ir_to_file(temp_ir);
            
            // Use clang to compile IR to executable
            std::string compile_cmd = "clang " + temp_ir + " -o " + output_file;
            int result = std::system(compile_cmd.c_str());
            
            // Clean up temp file
            fs::remove(temp_ir);
            
            if (result == 0) {
                std::cout << "Successfully compiled to: " << output_file << "\n";
                std::cout << "Run with: ./" << output_file << "\n";
            } else {
                std::cerr << "Error: Compilation failed\n";
                return 1;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Compilation error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}