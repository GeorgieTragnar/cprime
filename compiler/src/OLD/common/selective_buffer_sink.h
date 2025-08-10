#pragma once

#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/null_mutex.h>
#include "component_buffer_manager.h"
#include <mutex>
#include <memory>

namespace cprime {

/**
 * SelectiveBufferSink - Custom spdlog sink that selectively buffers messages
 * 
 * This sink intercepts log messages and conditionally buffers them based on:
 * - Component name (extracted from logger name)
 * - Message log level vs configured buffer level for that component
 * 
 * Key behaviors:
 * - Always passes through to next sink (doesn't interfere with normal logging)
 * - Only buffers if component is actively buffering AND message meets level criteria
 * - Thread-safe using the Mutex template parameter
 */
template<typename Mutex>
class SelectiveBufferSink : public spdlog::sinks::base_sink<Mutex> {
public:
    /**
     * Constructor
     * @param buffer_manager Shared buffer manager instance
     */
    explicit SelectiveBufferSink(std::shared_ptr<ComponentBufferManager> buffer_manager)
        : buffer_manager_(std::move(buffer_manager)) {}
    
    /**
     * Set the buffer manager (useful for lazy initialization)
     * @param buffer_manager Shared buffer manager instance
     */
    void set_buffer_manager(std::shared_ptr<ComponentBufferManager> buffer_manager) {
        buffer_manager_ = std::move(buffer_manager);
    }

protected:
    /**
     * Called by spdlog for each log message.
     * This implementation selectively buffers messages based on component and level.
     */
    void sink_it_(const spdlog::details::log_msg& msg) override {
        if (!buffer_manager_) {
            return; // No buffer manager available
        }
        
        // Extract component name from logger name
        std::string component = extract_component_name(msg);
        
        // Check if this component should buffer this message level
        if (buffer_manager_->should_buffer(component, msg.level)) {
            buffer_manager_->add_to_buffer(component, msg);
        }
        
        // Note: This sink only buffers, it doesn't output anything directly.
        // Normal log output happens through other sinks in the logger.
    }
    
    /**
     * Called by spdlog to flush any pending data.
     * For a buffer sink, there's nothing to flush.
     */
    void flush_() override {
        // Buffer sink doesn't need to flush anything
    }

private:
    std::shared_ptr<ComponentBufferManager> buffer_manager_;
    
    /**
     * Extract component name from log message.
     * The component name is the logger name.
     * 
     * @param msg Log message containing logger name
     * @return Component name as string
     */
    std::string extract_component_name(const spdlog::details::log_msg& msg) const {
        // Logger name is stored as string_view in spdlog::details::log_msg
        return std::string(msg.logger_name.data(), msg.logger_name.size());
    }
};

// Convenience type aliases for multi-threaded and single-threaded versions
using SelectiveBufferSink_mt = SelectiveBufferSink<std::mutex>;
using SelectiveBufferSink_st = SelectiveBufferSink<spdlog::details::null_mutex>;

} // namespace cprime