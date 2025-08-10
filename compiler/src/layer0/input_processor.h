#pragma once

#include "compilation_parameters.h"
#include <filesystem>
#include <map>
#include <sstream>
#include <string>

// Forward declaration for friend access
namespace cprime::layer0validation {
    class InputDebug;
}

namespace cprime {

/**
 * Layer 0: Input Processing
 * 
 * Responsibilities:
 * - Read input files from filesystem
 * - Convert file contents to stringstreams
 * - Generate unique stream IDs for each input
 * - Handle file reading errors gracefully
 * - Return named stringstream map for Layer 1
 * 
 * Design:
 * - Static methods (stateless processing)
 * - Takes file paths, returns stream map
 * - Stream ID generation based on file paths
 * - Basic error reporting
 */
class InputProcessor {
    // Allow validation layer friend access for debugging
    friend class cprime::layer0validation::InputDebug;
public:
    /**
     * Process all input files specified in compilation parameters.
     * Reads files and returns input streams map.
     * 
     * @param params Compilation parameters containing input file paths
     * @return Map of stream_id -> stringstream, or empty map on error
     * TODO: Implement proper error handling with Result<T> type system
     */
    static std::map<std::string, std::stringstream> process_input_files(
        const CompilationParameters& params
    );
    
    /**
     * Read a single file into a stringstream.
     * 
     * @param file_path Path to the file to read
     * @return stringstream with file contents, or empty stream on error
     * TODO: Implement proper error handling with Result<T> type system
     */
    static std::stringstream read_file(const std::filesystem::path& file_path);
    
    /**
     * Generate unique stream ID from file path.
     * Uses filename without directory path as basis for ID.
     * 
     * @param file_path Path to generate ID for
     * @return Unique string ID for the stream
     */
    static std::string generate_stream_id(const std::filesystem::path& file_path);

private:
    /**
     * Check if file exists and is readable.
     * 
     * @param file_path Path to check
     * @return true if file exists and is readable, false otherwise
     */
    static bool is_file_readable(const std::filesystem::path& file_path);
    
    /**
     * Validate file extension is supported.
     * 
     * @param file_path File to validate
     * @return true if extension is .cp or .cprime, false otherwise
     */
    static bool has_valid_extension(const std::filesystem::path& file_path);
    
    /**
     * Get file size in bytes for logging purposes.
     * 
     * @param file_path Path to get size for
     * @return File size in bytes, or 0 if file doesn't exist
     */
    static std::size_t get_file_size(const std::filesystem::path& file_path);
};

} // namespace cprime