#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace cprime {

/**
 * Common utility types and constants used throughout the CPrime compiler.
 */

/**
 * Compiler options and configuration.
 */
struct CompilerOptions {
    std::string input_file;
    std::string output_file;
    bool debug_mode = false;
    bool verbose = false;
    bool optimize = false;
    int optimization_level = 0;
    
    // Validation options
    bool enable_all_warnings = false;
    bool warnings_as_errors = false;
    
    // Output options
    bool generate_ast_dump = false;
    bool generate_ir_dump = false;
    bool generate_debug_info = false;
};

/**
 * Version information for the compiler.
 */
struct VersionInfo {
    static constexpr int MAJOR = 2;
    static constexpr int MINOR = 0; 
    static constexpr int PATCH = 0;
    
    static std::string get_version_string() {
        return std::to_string(MAJOR) + "." + std::to_string(MINOR) + "." + std::to_string(PATCH);
    }
    
    static std::string get_full_version_string() {
        return "CPrime Compiler v" + get_version_string() + " - Multi-Layer GPU-Ready Architecture";
    }
};

/**
 * Result type for compiler operations that may succeed or fail.
 */
template<typename T>
class Result {
public:
    Result(T&& value) : value_(std::make_unique<T>(std::forward<T>(value))), has_value_(true) {}
    Result(const std::string& error) : error_(error), has_value_(false) {}
    
    bool success() const { return has_value_; }
    bool has_error() const { return !has_value_; }
    
    const T& value() const { 
        if (!has_value_) {
            throw std::runtime_error("Attempted to access value of failed Result: " + error_);
        }
        return *value_; 
    }
    
    T& value() { 
        if (!has_value_) {
            throw std::runtime_error("Attempted to access value of failed Result: " + error_);
        }
        return *value_; 
    }
    
    const std::string& error() const { return error_; }
    
    // Convenience operator for success checking
    explicit operator bool() const { return success(); }
    
private:
    std::unique_ptr<T> value_;
    std::string error_;
    bool has_value_;
};

/**
 * Specialized Result for operations that don't return a value.
 */
using VoidResult = Result<bool>;

/**
 * Create a successful VoidResult.
 */
inline VoidResult success() {
    return VoidResult(true);
}

/**
 * Create a failed Result with an error message.
 */
template<typename T>
Result<T> failure(const std::string& error) {
    return Result<T>(error);
}

/**
 * File utilities for the compiler.
 */
namespace file_utils {
    
    /**
     * Check if a file exists and is readable.
     */
    bool file_exists(const std::string& path);
    
    /**
     * Read entire file contents into a string.
     */
    Result<std::string> read_file(const std::string& path);
    
    /**
     * Write string contents to a file.
     */
    VoidResult write_file(const std::string& path, const std::string& content);
    
    /**
     * Get file extension from path.
     */
    std::string get_extension(const std::string& path);
    
    /**
     * Get filename without directory path.
     */
    std::string get_filename(const std::string& path);
    
} // namespace file_utils

} // namespace cprime