#include "input_processor.h"
#include <fstream>
#include <sstream>

namespace cprime {

VoidResult InputProcessor::process_input_files(
    const CompilationParameters& params, 
    CompilationContext& context
) {
    // Clear any existing input streams
    context.input_streams.clear();
    
    // Process each input file
    for (const auto& file_path : params.input_files) {
        // Validate file
        if (!is_file_readable(file_path)) {
            return failure<bool>("File not readable: " + file_path.string());
        }
        
        if (!has_valid_extension(file_path)) {
            return failure<bool>("Invalid file extension: " + file_path.string() + 
                               " (expected .cp or .cprime)");
        }
        
        // Read file
        auto file_result = read_file(file_path);
        if (!file_result.success()) {
            return failure<bool>("Failed to read file " + file_path.string() + 
                               ": " + file_result.error());
        }
        
        // Generate stream ID and store
        std::string stream_id = generate_stream_id(file_path);
        context.input_streams[stream_id] = std::move(file_result.value());
    }
    
    return success();
}

Result<std::stringstream> InputProcessor::read_file(const std::filesystem::path& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return failure<std::stringstream>("Cannot open file: " + file_path.string());
    }
    
    std::stringstream stream;
    stream << file.rdbuf();
    
    if (file.bad()) {
        return failure<std::stringstream>("Error reading file: " + file_path.string());
    }
    
    return Result<std::stringstream>(std::move(stream));
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