#pragma once

#include "../layer0/input_processor.h"
#include "../commons/logger.h"
#include <map>
#include <string>
#include <sstream>

namespace cprime::layer0validation {

/**
 * InputDebug - Debug utilities for Layer 0 input processing
 * 
 * Provides debugging and inspection capabilities for the InputProcessor class.
 * This class has friend access to InputProcessor internals for detailed debugging.
 * 
 * Purpose: CLI debugging support for Layer 0 development and troubleshooting.
 */
class InputDebug {
public:
    /**
     * Debug the complete input processing pipeline
     * @param params Compilation parameters to process
     * @param logger Logger instance for debug output
     * @return Map of processed input streams (same as InputProcessor output)
     */
    static std::map<std::string, std::stringstream> debug_process_input_files(
        const CompilationParameters& params,
        cprime::Logger& logger
    );
    
    /**
     * Debug individual file processing with detailed output
     * @param file_path Path to file being processed
     * @param logger Logger instance for debug output
     * @return Processed stringstream and success status
     */
    static std::pair<std::stringstream, bool> debug_process_single_file(
        const std::filesystem::path& file_path,
        cprime::Logger& logger
    );
    
    /**
     * Show detailed file validation information
     * @param file_path Path to file being validated
     * @param logger Logger instance for debug output
     */
    static void debug_file_validation(
        const std::filesystem::path& file_path,
        cprime::Logger& logger
    );
    
    /**
     * Show stream ID generation process
     * @param file_path Original file path
     * @param generated_id Generated stream ID
     * @param logger Logger instance for debug output
     */
    static void debug_stream_id_generation(
        const std::filesystem::path& file_path,
        const std::string& generated_id,
        cprime::Logger& logger
    );

private:
    // Helper methods for internal debugging
    static void log_file_stats(
        const std::filesystem::path& file_path,
        cprime::Logger& logger
    );
    
    static void log_processing_step(
        const std::string& step_name,
        const std::string& details,
        cprime::Logger& logger
    );
};

} // namespace cprime::layer0validation