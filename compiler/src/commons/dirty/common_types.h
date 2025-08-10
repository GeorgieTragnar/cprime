#pragma once

#include <string>
#include <memory>

namespace cprime {

/**
 * Version information for the compiler.
 */
struct VersionInfo {
    static constexpr int MAJOR = 3;
    static constexpr int MINOR = 0; 
    static constexpr int PATCH = 0;
    
    static std::string get_version_string() {
        return std::to_string(MAJOR) + "." + std::to_string(MINOR) + "." + std::to_string(PATCH);
    }
    
    static std::string get_full_version_string() {
        return "CPrime Compiler v" + get_version_string() + " - Orchestrator-Based Multi-Layer Architecture";
    }
};

/**
 * Result type for operations that may succeed or fail.
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

} // namespace cprime