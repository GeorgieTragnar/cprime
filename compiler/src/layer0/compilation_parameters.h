#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace cprime {

/**
 * Compilation parameters structure passed to the orchestrator.
 * Contains all input configuration needed for compilation process.
 */
struct CompilationParameters {
    // Input files to compile
    std::vector<std::filesystem::path> input_files;
    
    // Output configuration
    std::filesystem::path output_file;
    
    // Compilation flags
    bool verbose = false;
    bool debug_mode = false;
    bool generate_ast_dump = false;
    bool generate_ir_dump = false;
    bool warnings_as_errors = false;
    
    // Optimization settings
    bool optimize = false;
    int optimization_level = 0;
    
    /**
     * Validate compilation parameters for basic correctness.
     * @return true if parameters are valid, false otherwise
     */
    bool validate() const {
        if (input_files.empty()) {
            return false;
        }
        
        if (output_file.empty()) {
            return false;
        }
        
        // Check that all input files have valid extensions
        for (const auto& file : input_files) {
            const std::string ext = file.extension().string();
            if (ext != ".cp" && ext != ".cprime") {
                return false;
            }
        }
        
        return true;
    }
    
    /**
     * Get a string representation for logging purposes.
     */
    std::string to_string() const {
        std::string result = "CompilationParameters:\n";
        result += "  Input files: ";
        for (const auto& file : input_files) {
            result += file.string() + " ";
        }
        result += "\n  Output file: " + output_file.string();
        result += "\n  Verbose: " + std::string(verbose ? "true" : "false");
        result += "\n  Debug: " + std::string(debug_mode ? "true" : "false");
        return result;
    }
};

} // namespace cprime