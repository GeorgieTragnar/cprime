#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <csignal>

#include "component_buffer_manager.h"
#include "selective_buffer_sink.h"

#ifdef __INTELLISENSE__
#define __FILE_NAME__ __FILE__
#endif

namespace cprime {

class Logger {
public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }

    std::shared_ptr<spdlog::logger> get(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = loggers_.find(name);
        if (it != loggers_.end()) {
            return it->second;
        }
        auto logger = create_logger(name);
        loggers_[name] = logger;
        return logger;
    }

    void set_level(const std::string& name, spdlog::level::level_enum level) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (auto it = loggers_.find(name); it != loggers_.end()) {
            it->second->set_level(level);
        }
        else
        {
            std::raise(SIGTERM);
        }
    }

    void set_global_level(spdlog::level::level_enum level) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [_, logger] : loggers_) {
            logger->set_level(level);
        }
    }

    // ============================================================================
    // Component Buffer Control Methods
    // ============================================================================
    
    /**
     * Start buffering messages for a component at the specified level and above.
     * @param component Component name (use CPRIME_COMPONENT_* macros)
     * @param buffer_level Minimum level to buffer (e.g., debug level buffers debug+)
     */
    void begin_component_buffering(const std::string& component, spdlog::level::level_enum buffer_level = spdlog::level::debug) {
        if (buffer_manager_) {
            buffer_manager_->begin_buffering(component, buffer_level);
        }
    }
    
    /**
     * Stop buffering messages for a component.
     * @param component Component name
     */
    void end_component_buffering(const std::string& component) {
        if (buffer_manager_) {
            buffer_manager_->end_buffering(component);
        }
    }
    
    /**
     * Dump all buffered messages for a component with critical announcement.
     * @param component Component name
     */
    void dump_component_buffer(const std::string& component) {
        if (!buffer_manager_) {
            return;
        }
        
        auto logger = get(component);
        
        // 1. Log CRITICAL announcement
        logger->critical("=== DUMPING BUFFER FOR COMPONENT: {} ===", component);
        
        // 2. Get all buffered messages
        auto messages = buffer_manager_->get_buffer_messages(component);
        
        // 3. Output each message with original timestamp/level to ALL sinks
        for (const auto& msg_buf : messages) {
            // Force output to all sinks regardless of current log level
            for (auto& sink : logger->sinks()) {
                sink->log(msg_buf);
            }
        }
        
        logger->critical("=== END BUFFER DUMP FOR COMPONENT: {} ({} messages) ===", 
                        component, messages.size());
    }
    
    /**
     * Clear all buffered messages for a component.
     * @param component Component name
     */
    void clear_component_buffer(const std::string& component) {
        if (buffer_manager_) {
            buffer_manager_->clear_buffer(component);
        }
    }
    
    /**
     * Check if a component is currently buffering.
     * @param component Component name
     * @return true if component is actively buffering
     */
    bool is_component_buffering(const std::string& component) const {
        return buffer_manager_ && buffer_manager_->is_buffering(component);
    }
    
    /**
     * Get the buffer level for a component.
     * @param component Component name
     * @return Buffer level (or off if not buffering)
     */
    spdlog::level::level_enum get_component_buffer_level(const std::string& component) const {
        return buffer_manager_ ? buffer_manager_->get_buffer_level(component) : spdlog::level::off;
    }
    
    /**
     * Get count of buffered messages for a component.
     * @param component Component name
     * @return Number of messages in buffer
     */
    size_t get_component_buffer_count(const std::string& component) const {
        return buffer_manager_ ? buffer_manager_->get_buffer_count(component) : 0;
    }

private:
    Logger() {
        // Initialize buffer management
        buffer_manager_ = std::make_shared<ComponentBufferManager>();
        buffer_sink_ = std::make_shared<SelectiveBufferSink_mt>(buffer_manager_);
        
        // Initialize regular sinks
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/cprime.log", 10 * 1024 * 1024, 5);

        std::string pattern = "%^%L%C%m%d|%H%M| %v%$";
		
		console_sink->set_pattern(pattern);
		file_sink->set_pattern(pattern);

        // Include buffer sink along with regular sinks
        sinks_ = {console_sink, file_sink, buffer_sink_};
    }

    std::shared_ptr<spdlog::logger> create_logger(const std::string& name) {
        auto logger = std::make_shared<spdlog::logger>(name, sinks_.begin(), sinks_.end());
        logger->set_level(spdlog::level::trace);
        return logger;
    }

    std::vector<spdlog::sink_ptr> sinks_;
    std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers_;
    std::mutex mutex_;
    
    // Buffer management
    std::shared_ptr<ComponentBufferManager> buffer_manager_;
    std::shared_ptr<SelectiveBufferSink_mt> buffer_sink_;
};

} // namespace cprime

// CPrime-specific logging macros
#define CPRIME_LOGGER(name) cprime::Logger::instance().get(name)
#define CPRIME_LOG_TRACE(...) do { if (logger) logger->trace(fmt::runtime("{:<186} | {:>30}"), fmt::format(__VA_ARGS__), fmt::format("{}:{}", __FILE_NAME__, __LINE__)); } while(0)
#define CPRIME_LOG_DEBUG(...) do { if (logger) logger->debug(fmt::runtime("{:<186} | {:>30}"), fmt::format(__VA_ARGS__), fmt::format("{}:{}", __FILE_NAME__, __LINE__)); } while(0)
#define CPRIME_LOG_INFO(...) do { if (logger) logger->info(fmt::runtime("{:<186} | {:>30}"), fmt::format(__VA_ARGS__), fmt::format("{}:{}", __FILE_NAME__, __LINE__)); } while(0)
#define CPRIME_LOG_WARN(...) do { if (logger) logger->warn(fmt::runtime("{:<186} | {:>30}"), fmt::format(__VA_ARGS__), fmt::format("{}:{}", __FILE_NAME__, __LINE__)); } while(0)
#define CPRIME_LOG_ERROR(...) do { if (logger) logger->error(fmt::runtime("{:<186} | {:>30}"), fmt::format(__VA_ARGS__), fmt::format("{}:{}", __FILE_NAME__, __LINE__)); } while(0)
#define CPRIME_LOG_CRITICAL(...) do { if (logger) logger->critical(fmt::runtime("{:<186} | {:>30}"), fmt::format(__VA_ARGS__), fmt::format("{}:{}", __FILE_NAME__, __LINE__)); } while(0)
#define CPRIME_LOG_FATAL(...) do { if (logger) { logger->critical(fmt::runtime("{:<186} | {:>30}"), fmt::format(__VA_ARGS__), fmt::format("{}:{}", __FILE_NAME__, __LINE__)); std::raise(SIGTERM); } } while(0)

// ============================================================================
// Enhanced Buffer Control Macros
// ============================================================================

// Component-specific logger macros (use with CPRIME_COMPONENT_* constants)
#define CPRIME_COMPONENT_LOGGER(component) cprime::Logger::instance().get(component)

// Buffer control macros using component constants
#define CPRIME_BUFFER_BEGIN(component, level) cprime::Logger::instance().begin_component_buffering(component, level)
#define CPRIME_BUFFER_END(component) cprime::Logger::instance().end_component_buffering(component)  
#define CPRIME_BUFFER_DUMP(component) cprime::Logger::instance().dump_component_buffer(component)
#define CPRIME_BUFFER_CLEAR(component) cprime::Logger::instance().clear_component_buffer(component)

// Buffer query macros
#define CPRIME_BUFFER_IS_ACTIVE(component) cprime::Logger::instance().is_component_buffering(component)
#define CPRIME_BUFFER_COUNT(component) cprime::Logger::instance().get_component_buffer_count(component)
#define CPRIME_BUFFER_LEVEL(component) cprime::Logger::instance().get_component_buffer_level(component)

// Convenience macros for common buffer operations
#define CPRIME_BUFFER_BEGIN_DEBUG(component) CPRIME_BUFFER_BEGIN(component, spdlog::level::debug)
#define CPRIME_BUFFER_BEGIN_TRACE(component) CPRIME_BUFFER_BEGIN(component, spdlog::level::trace)
#define CPRIME_BUFFER_BEGIN_INFO(component) CPRIME_BUFFER_BEGIN(component, spdlog::level::info)

// Complete buffer lifecycle macro (for scoped buffering)
#define CPRIME_BUFFER_SCOPED_BEGIN(component, level) \
    struct BufferScope_##component { \
        std::string comp = component; \
        BufferScope_##component() { CPRIME_BUFFER_BEGIN(comp, level); } \
        ~BufferScope_##component() { CPRIME_BUFFER_END(comp); CPRIME_BUFFER_CLEAR(comp); } \
    } buffer_scope_##component;
