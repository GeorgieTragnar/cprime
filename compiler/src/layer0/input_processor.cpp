#include "input_processor.h"
#include <fstream>
#include <sstream>
#include "../commons/logger.h"

namespace cprime {

std::map<std::string, std::stringstream> InputProcessor::process_input_files(
    const CompilationParameters& params
) {
    // Create layer0 logger for input processing
    auto logger = cprime::LoggerFactory::get_logger("layer0");
    std::map<std::string, std::stringstream> input_streams;
    
    logger.debug("Processing {} input files", params.input_files.size());
    
    // Process each input file
    for (const auto& file_path : params.input_files) {
        logger.debug("Processing file: {}", file_path.string());
        
        // Validate file
        if (!is_file_readable(file_path)) {
            // TODO: Implement proper error handling with Result<T> type system
            logger.error("File not readable: {}", file_path.string());
            return {}; // Return empty map to signal error
        }
        
        if (!has_valid_extension(file_path)) {
            // TODO: Implement proper error handling with Result<T> type system
            logger.error("Invalid file extension: {} (expected .cp or .cprime)", file_path.string());
            return {}; // Return empty map to signal error
        }
        
        // Read file
        std::stringstream file_stream = read_file(file_path);
        if (file_stream.str().empty() && file_stream.fail()) {
            // TODO: Implement proper error handling with Result<T> type system
            logger.error("Failed to read file {}", file_path.string());
            return {}; // Return empty map to signal error
        }
        
        // Generate stream ID and store
        std::string stream_id = generate_stream_id(file_path);
        logger.debug("Generated stream ID '{}' for file {}", stream_id, file_path.string());
        input_streams[stream_id] = std::move(file_stream);
    }
    
    logger.info("Successfully processed {} input files", input_streams.size());
    return input_streams;
}

std::stringstream InputProcessor::read_file(const std::filesystem::path& file_path) {
    // Create layer0 logger for file reading operations
    auto logger = cprime::LoggerFactory::get_logger("layer0");
    
    std::ifstream file(file_path);
    std::stringstream stream;
    
    if (!file.is_open()) {
        // TODO: Implement proper error handling with Result<T> type system
        logger.error("Cannot open file: {}", file_path.string());
        stream.setstate(std::ios::failbit);
        return stream;
    }
    
    stream << file.rdbuf();
    
    if (file.bad()) {
        // TODO: Implement proper error handling with Result<T> type system
        logger.error("Error reading file: {}", file_path.string());
        stream.setstate(std::ios::failbit);
    } else {
        logger.debug("Successfully read {} characters from {}", 
                    stream.str().length(), file_path.string());
    }
    
    return stream;
}

std::string InputProcessor::generate_stream_id(const std::filesystem::path& file_path) {
    // Use filename without directory path
    std::string filename = file_path.filename().string();
    
    // For now, just use the filename as-is
    // Future: Could add hash suffix for collision avoidance if needed
    return filename;
}

bool InputProcessor::is_file_readable(const std::filesystem::path& file_path) {
    std::error_code ec;
    
    // Check if file exists
    if (!std::filesystem::exists(file_path, ec) || ec) {
        return false;
    }
    
    // Check if it's a regular file (not directory, etc.)
    if (!std::filesystem::is_regular_file(file_path, ec) || ec) {
        return false;
    }
    
    // Try to open for reading
    std::ifstream file(file_path);
    return file.is_open();
}

bool InputProcessor::has_valid_extension(const std::filesystem::path& file_path) {
    std::string ext = file_path.extension().string();
    return ext == ".cp" || ext == ".cprime";
}

std::size_t InputProcessor::get_file_size(const std::filesystem::path& file_path) {
    std::error_code ec;
    auto size = std::filesystem::file_size(file_path, ec);
    return ec ? 0 : size;
}

} // namespace cprime