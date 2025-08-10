#include "input_debug.h"
#include <filesystem>
#include <fstream>

namespace cprime::layer0validation {

std::map<std::string, std::stringstream> InputDebug::debug_process_input_files(
    const CompilationParameters& params,
    cprime::Logger& logger
) {
    auto& log = logger; // Alias for LOG macros
    
    LOG_INFO("=== Layer 0 Input Processing Debug ===");
    LOG_DEBUG("Starting debug analysis of {} input files", params.input_files.size());
    
    std::map<std::string, std::stringstream> debug_streams;
    
    // Process each file with detailed debugging
    for (size_t i = 0; i < params.input_files.size(); ++i) {
        const auto& file_path = params.input_files[i];
        
        LOG_DEBUG("Processing file {} of {}: {}", i + 1, params.input_files.size(), file_path.string());
        
        // Debug file validation first
        debug_file_validation(file_path, logger);
        
        // Process the file with detailed debugging
        auto [stream, success] = debug_process_single_file(file_path, logger);
        
        if (success) {
            // Generate stream ID with debugging
            std::string stream_id = InputProcessor::generate_stream_id(file_path);
            debug_stream_id_generation(file_path, stream_id, logger);
            
            debug_streams[stream_id] = std::move(stream);
            LOG_INFO("Successfully processed file {}: stream '{}' created", file_path.string(), stream_id);
        } else {
            LOG_ERROR("Failed to process file: {}", file_path.string());
            // Return empty map to match InputProcessor behavior
            return {};
        }
    }
    
    LOG_INFO("Layer 0 debug processing completed: {} streams created", debug_streams.size());
    return debug_streams;
}

std::pair<std::stringstream, bool> InputDebug::debug_process_single_file(
    const std::filesystem::path& file_path,
    cprime::Logger& logger
) {
    auto& log = logger; // Alias for LOG macros
    
    LOG_DEBUG("=== Single File Debug: {} ===", file_path.string());
    
    // Show file statistics
    log_file_stats(file_path, logger);
    
    // Debug file reading process
    log_processing_step("File Opening", "Attempting to open file for reading", logger);
    
    std::ifstream file(file_path);
    std::stringstream stream;
    
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file: {}", file_path.string());
        stream.setstate(std::ios::failbit);
        return {std::move(stream), false};
    }
    
    log_processing_step("File Reading", "Reading file content into stringstream", logger);
    
    // Read content and track progress
    stream << file.rdbuf();
    
    if (file.bad()) {
        LOG_ERROR("Error occurred while reading file: {}", file_path.string());
        stream.setstate(std::ios::failbit);
        return {std::move(stream), false};
    }
    
    // Show successful read statistics
    size_t content_size = stream.str().length();
    LOG_INFO("Successfully read {} characters from {}", content_size, file_path.string());
    
    // Show content preview (first 100 characters)
    if (content_size > 0) {
        std::string preview = stream.str().substr(0, std::min(size_t(100), content_size));
        // Replace newlines with \\n for single-line preview
        std::string display_preview;
        for (char c : preview) {
            if (c == '\n') {
                display_preview += "\\n";
            } else if (c == '\t') {
                display_preview += "\\t";
            } else if (c >= 32 && c <= 126) { // Printable ASCII
                display_preview += c;
            } else {
                display_preview += "?";
            }
        }
        if (content_size > 100) {
            display_preview += "...";
        }
        LOG_DEBUG("Content preview: '{}'", display_preview);
    }
    
    return {std::move(stream), true};
}

void InputDebug::debug_file_validation(
    const std::filesystem::path& file_path,
    cprime::Logger& logger
) {
    auto& log = logger; // Alias for LOG macros
    
    LOG_DEBUG("=== File Validation Debug: {} ===", file_path.string());
    
    // Test readability
    bool readable = InputProcessor::is_file_readable(file_path);
    LOG_DEBUG("File readability check: {}", readable ? "PASS" : "FAIL");
    
    if (!readable) {
        // Check specific failure reasons
        std::error_code ec;
        bool exists = std::filesystem::exists(file_path, ec);
        bool is_file = std::filesystem::is_regular_file(file_path, ec);
        
        LOG_DEBUG("  - File exists: {}", exists ? "YES" : "NO");
        LOG_DEBUG("  - Is regular file: {}", is_file ? "YES" : "NO");
    }
    
    // Test extension validation
    bool valid_ext = InputProcessor::has_valid_extension(file_path);
    std::string extension = file_path.extension().string();
    LOG_DEBUG("Extension validation: {} (extension: '{}')", valid_ext ? "PASS" : "FAIL", extension);
    
    if (!valid_ext) {
        LOG_DEBUG("  - Expected: .cp or .cprime");
        LOG_DEBUG("  - Got: '{}'", extension);
    }
}

void InputDebug::debug_stream_id_generation(
    const std::filesystem::path& file_path,
    const std::string& generated_id,
    cprime::Logger& logger
) {
    auto& log = logger; // Alias for LOG macros
    
    LOG_DEBUG("=== Stream ID Generation Debug ===");
    LOG_DEBUG("Original file path: {}", file_path.string());
    LOG_DEBUG("Filename extracted: {}", file_path.filename().string());
    LOG_DEBUG("Generated stream ID: '{}'", generated_id);
    
    // Note: Currently stream ID is just the filename, but this debug function
    // allows us to track the generation process for future enhancements
}

void InputDebug::log_file_stats(
    const std::filesystem::path& file_path,
    cprime::Logger& logger
) {
    auto& log = logger; // Alias for LOG macros
    
    std::error_code ec;
    auto file_size = std::filesystem::file_size(file_path, ec);
    
    if (!ec) {
        LOG_DEBUG("File size: {} bytes", file_size);
    } else {
        LOG_DEBUG("Could not determine file size");
    }
    
    LOG_DEBUG("File extension: '{}'", file_path.extension().string());
    LOG_DEBUG("Filename: '{}'", file_path.filename().string());
    LOG_DEBUG("Full path: '{}'", file_path.string());
}

void InputDebug::log_processing_step(
    const std::string& step_name,
    const std::string& details,
    cprime::Logger& logger
) {
    auto& log = logger; // Alias for LOG macros
    LOG_DEBUG("Step: {} - {}", step_name, details);
}

} // namespace cprime::layer0validation