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
    
    // Set up default pattern for both sinks
    std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v";
    console_sink->set_pattern(pattern);
    
    // Register both sinks as default for new loggers
    spdlog::sinks_init_list sink_list = {console_sink, selective_sink};
    spdlog::set_default_logger(std::make_shared<spdlog::logger>("default", sink_list));
    
    buffering_initialized_ = true;
}

} // namespace cprime