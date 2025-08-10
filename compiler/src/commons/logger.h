#pragma once

#include <iostream>
#include <string>
#include <sstream>

namespace cprime {

/**
 * Simple logging system for the compiler.
 * Simplified version for initial orchestrator implementation.
 */
enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3
};

class Logger {
private:
    LogLevel current_level_ = LogLevel::Info;
    std::string component_;
    
public:
    explicit Logger(const std::string& component) : component_(component) {}
    
    void set_level(LogLevel level) {
        current_level_ = level;
    }
    
    LogLevel get_level() const {
        return current_level_;
    }
    
    template<typename... Args>
    void debug(const std::string& format, Args&&... args) {
        if (current_level_ <= LogLevel::Debug) {
            log(LogLevel::Debug, format, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void info(const std::string& format, Args&&... args) {
        if (current_level_ <= LogLevel::Info) {
            log(LogLevel::Info, format, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void warning(const std::string& format, Args&&... args) {
        if (current_level_ <= LogLevel::Warning) {
            log(LogLevel::Warning, format, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void error(const std::string& format, Args&&... args) {
        if (current_level_ <= LogLevel::Error) {
            log(LogLevel::Error, format, std::forward<Args>(args)...);
        }
    }
    
private:
    template<typename... Args>
    void log(LogLevel level, const std::string& format, Args&&... args) {
        std::string level_str;
        switch (level) {
            case LogLevel::Debug: level_str = "DEBUG"; break;
            case LogLevel::Info: level_str = "INFO"; break;
            case LogLevel::Warning: level_str = "WARN"; break;
            case LogLevel::Error: level_str = "ERROR"; break;
        }
        
        // Simple format replacement (just for basic cases)
        std::string message = format;
        replace_args(message, std::forward<Args>(args)...);
        
        std::cout << "[" << level_str << "][" << component_ << "] " << message << std::endl;
    }
    
    // Simple string replacement for {} placeholders
    template<typename T>
    void replace_args(std::string& str, T&& arg) {
        size_t pos = str.find("{}");
        if (pos != std::string::npos) {
            std::stringstream ss;
            ss << arg;
            str.replace(pos, 2, ss.str());
        }
    }
    
    template<typename T, typename... Args>
    void replace_args(std::string& str, T&& first, Args&&... rest) {
        replace_args(str, std::forward<T>(first));
        replace_args(str, std::forward<Args>(rest)...);
    }
    
    void replace_args(std::string&) {} // Base case
};

/**
 * Global logger factory for different components.
 */
class LoggerFactory {
public:
    static Logger get_logger(const std::string& component) {
        return Logger(component);
    }
    
    static void set_global_level(LogLevel level) {
        global_level_ = level;
    }
    
    static LogLevel get_global_level() {
        return global_level_;
    }

private:
    static LogLevel global_level_;
};

// Convenience macros
#define CPRIME_LOGGER(component) cprime::LoggerFactory::get_logger(component)
#define CPRIME_LOG_DEBUG(logger, msg, ...) logger.debug(msg, ##__VA_ARGS__)
#define CPRIME_LOG_INFO(logger, msg, ...) logger.info(msg, ##__VA_ARGS__)
#define CPRIME_LOG_WARN(logger, msg, ...) logger.warning(msg, ##__VA_ARGS__)
#define CPRIME_LOG_ERROR(logger, msg, ...) logger.error(msg, ##__VA_ARGS__)

} // namespace cprime