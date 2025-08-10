#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <memory>

// Support for __FILE_NAME__ macro (for IntelliSense compatibility)
#ifdef __INTELLISENSE__
#define __FILE_NAME__ __FILE__
#endif

// Forward declare selective buffer classes
namespace cprime {
    class ComponentBufferManager;
}

namespace cprime {

/**
 * Enhanced logging system for the compiler with selective buffering support.
 * Integrates spdlog with our custom selective buffer sink for component-based buffering.
 */
enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3
};

class Logger {
private:
    std::shared_ptr<spdlog::logger> spdlog_logger_;
    std::string component_;
    
public:
    // Convert our LogLevel to spdlog::level (made public for LoggerFactory access)
    static spdlog::level::level_enum to_spdlog_level(LogLevel level) {
        switch (level) {
            case LogLevel::Debug: return spdlog::level::debug;
            case LogLevel::Info: return spdlog::level::info;
            case LogLevel::Warning: return spdlog::level::warn;
            case LogLevel::Error: return spdlog::level::err;
            default: return spdlog::level::info;
        }
    }

private:
    
public:
    explicit Logger(const std::string& component) : component_(component) {
        // Create spdlog logger for this component
        spdlog_logger_ = spdlog::get(component);
        if (!spdlog_logger_) {
            // Create with default sinks (which includes our selective buffer sink)
            auto default_sinks = spdlog::default_logger()->sinks();
            spdlog_logger_ = std::make_shared<spdlog::logger>(component, default_sinks.begin(), default_sinks.end());
            spdlog::register_logger(spdlog_logger_);
        }
    }
    
    void set_level(LogLevel level) {
        spdlog_logger_->set_level(to_spdlog_level(level));
    }
    
    LogLevel get_level() const {
        // Convert spdlog level back to our LogLevel
        switch (spdlog_logger_->level()) {
            case spdlog::level::debug: return LogLevel::Debug;
            case spdlog::level::info: return LogLevel::Info;
            case spdlog::level::warn: return LogLevel::Warning;
            case spdlog::level::err: return LogLevel::Error;
            default: return LogLevel::Info;
        }
    }
    
    template<typename... Args>
    void debug(const std::string& format, Args&&... args) {
        spdlog_logger_->debug(format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void info(const std::string& format, Args&&... args) {
        spdlog_logger_->info(format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void warning(const std::string& format, Args&&... args) {
        spdlog_logger_->warn(format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void error(const std::string& format, Args&&... args) {
        spdlog_logger_->error(format, std::forward<Args>(args)...);
    }
    
    // Access to underlying spdlog logger for advanced usage
    std::shared_ptr<spdlog::logger> get_spdlog_logger() const {
        return spdlog_logger_;
    }
};

/**
 * Global logger factory for different components with selective buffering support.
 */
class LoggerFactory {
public:
    static Logger get_logger(const std::string& component) {
        // Initialize selective buffering if not already done
        initialize_selective_buffering();
        return Logger(component);
    }
    
    static void set_global_level(LogLevel level) {
        spdlog::set_level(Logger::to_spdlog_level(level));
    }
    
    static LogLevel get_global_level() {
        // Convert spdlog global level back to our LogLevel
        switch (spdlog::get_level()) {
            case spdlog::level::debug: return LogLevel::Debug;
            case spdlog::level::info: return LogLevel::Info;
            case spdlog::level::warn: return LogLevel::Warning;
            case spdlog::level::err: return LogLevel::Error;
            default: return LogLevel::Info;
        }
    }
    
    // Access to buffer manager for advanced selective buffering control
    static std::shared_ptr<ComponentBufferManager> get_buffer_manager();
    
    // Initialize selective buffering system (called automatically)
    static void initialize_selective_buffering();

private:
    static std::shared_ptr<ComponentBufferManager> buffer_manager_;
    static bool buffering_initialized_;
};

// Convenience macros (legacy - prefer LOG_* macros below)
#define CPRIME_LOGGER(component) cprime::LoggerFactory::get_logger(component)
#define CPRIME_LOG_DEBUG(logger, msg, ...) logger.debug(msg, ##__VA_ARGS__)
#define CPRIME_LOG_INFO(logger, msg, ...) logger.info(msg, ##__VA_ARGS__)
#define CPRIME_LOG_WARN(logger, msg, ...) logger.warning(msg, ##__VA_ARGS__)
#define CPRIME_LOG_ERROR(logger, msg, ...) logger.error(msg, ##__VA_ARGS__)

// Neat-base style LOG macros with two-column layout
// Usage: auto logger = CPRIME_LOGGER("component"); LOG_DEBUG("message with {}", arg);
// Note: These expect a variable named 'logger' of type cprime::Logger in scope
#define LOG_TRACE(...) do { logger.debug(fmt::format("{:<174} | {:>30}", fmt::format(__VA_ARGS__), fmt::format("{}:{}", __FILE_NAME__, __LINE__))); } while(0)
#define LOG_DEBUG(...) do { logger.debug(fmt::format("{:<174} | {:>30}", fmt::format(__VA_ARGS__), fmt::format("{}:{}", __FILE_NAME__, __LINE__))); } while(0)
#define LOG_INFO(...) do { logger.info(fmt::format("{:<174} | {:>30}", fmt::format(__VA_ARGS__), fmt::format("{}:{}", __FILE_NAME__, __LINE__))); } while(0)
#define LOG_WARN(...) do { logger.warning(fmt::format("{:<174} | {:>30}", fmt::format(__VA_ARGS__), fmt::format("{}:{}", __FILE_NAME__, __LINE__))); } while(0)
#define LOG_ERROR(...) do { logger.error(fmt::format("{:<174} | {:>30}", fmt::format(__VA_ARGS__), fmt::format("{}:{}", __FILE_NAME__, __LINE__))); } while(0)
#define LOG_CRITICAL(...) do { logger.error(fmt::format("{:<174} | {:>30}", fmt::format(__VA_ARGS__), fmt::format("{}:{}", __FILE_NAME__, __LINE__))); } while(0)

} // namespace cprime