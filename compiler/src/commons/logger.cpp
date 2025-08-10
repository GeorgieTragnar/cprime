#include "logger.h"
#include "dirty/selective_buffer_sink.h"
#include "dirty/component_buffer_manager.h"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace cprime {

// Static member definitions
std::shared_ptr<ComponentBufferManager> LoggerFactory::buffer_manager_;
bool LoggerFactory::buffering_initialized_ = false;

std::shared_ptr<ComponentBufferManager> LoggerFactory::get_buffer_manager() {
    initialize_selective_buffering();
    return buffer_manager_;
}

void LoggerFactory::initialize_selective_buffering() {
    if (buffering_initialized_) {
        return;
    }
    
    // Create the buffer manager
    buffer_manager_ = std::make_shared<ComponentBufferManager>();
    
    // Create selective buffer sink
    auto selective_sink = std::make_shared<SelectiveBufferSink<std::mutex>>(buffer_manager_);
    
    // Create console sink for normal output
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    
    // Force color output even when not connected to a TTY
    // This helps when output is being processed by scripts
    console_sink->set_color_mode(spdlog::color_mode::always);
    
    // Set up neat-base style pattern for both sinks
    // %^ and %$ enable/disable automatic color formatting
    // %L = single character log level (D/I/W/E/C)
    // %m%d = month/day (MMDD format), %H%M = hour/minute (HHMM format)
    // %v = message content
    std::string pattern = "%^%L%m%d|%H%M| %v%$";
    console_sink->set_pattern(pattern);
    
    // Register both sinks as default for new loggers
    spdlog::sinks_init_list sink_list = {console_sink, selective_sink};
    spdlog::set_default_logger(std::make_shared<spdlog::logger>("default", sink_list));
    
    buffering_initialized_ = true;
}

} // namespace cprime