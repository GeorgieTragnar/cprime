#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/details/log_msg_buffer.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace cprime {

/**
 * ComponentBufferManager - Manages selective buffering for different components
 * 
 * This class handles the storage and lifecycle of buffered log messages per component.
 * Each component can independently start/stop buffering at specified log levels.
 * 
 * Key features:
 * - Per-component buffer state management
 * - Thread-safe operations with mutex protection
 * - Manual lifecycle management (no auto-clearing)
 * - Level-based filtering for what gets buffered
 */
class ComponentBufferManager {
public:
    ComponentBufferManager() = default;
    ~ComponentBufferManager() = default;
    
    // Non-copyable, non-movable
    ComponentBufferManager(const ComponentBufferManager&) = delete;
    ComponentBufferManager& operator=(const ComponentBufferManager&) = delete;
    ComponentBufferManager(ComponentBufferManager&&) = delete;
    ComponentBufferManager& operator=(ComponentBufferManager&&) = delete;
    
    /**
     * Start buffering messages for a component at the specified level and above.
     * @param component Component name (use CPRIME_COMPONENT_* macros)
     * @param buffer_level Minimum level to buffer (e.g., debug level buffers debug+)
     */
    void begin_buffering(const std::string& component, spdlog::level::level_enum buffer_level);
    
    /**
     * Stop buffering messages for a component.
     * @param component Component name
     */
    void end_buffering(const std::string& component);
    
    /**
     * Check if a message should be buffered for this component.
     * @param component Component name
     * @param msg_level Level of the message to check
     * @return true if message should be buffered
     */
    bool should_buffer(const std::string& component, spdlog::level::level_enum msg_level) const;
    
    /**
     * Add a message to the component's buffer.
     * @param component Component name
     * @param msg Log message to buffer
     */
    void add_to_buffer(const std::string& component, const spdlog::details::log_msg& msg);
    
    /**
     * Get all buffered messages for a component.
     * @param component Component name
     * @return Vector of buffered messages (empty if no buffer or no messages)
     */
    std::vector<spdlog::details::log_msg_buffer> get_buffer_messages(const std::string& component) const;
    
    /**
     * Clear all buffered messages for a component.
     * @param component Component name
     */
    void clear_buffer(const std::string& component);
    
    /**
     * Check if a component is currently buffering.
     * @param component Component name
     * @return true if component is actively buffering
     */
    bool is_buffering(const std::string& component) const;
    
    /**
     * Get the buffer level for a component.
     * @param component Component name
     * @return Buffer level (or off if not buffering)
     */
    spdlog::level::level_enum get_buffer_level(const std::string& component) const;
    
    /**
     * Get count of buffered messages for a component.
     * @param component Component name
     * @return Number of messages in buffer
     */
    size_t get_buffer_count(const std::string& component) const;
    
    /**
     * Get all component names that are currently buffering.
     * @return Vector of component names
     */
    std::vector<std::string> get_buffering_components() const;

private:
    /**
     * Buffer state for a single component.
     */
    struct BufferState {
        bool is_buffering = false;
        spdlog::level::level_enum buffer_level = spdlog::level::debug;
        std::vector<spdlog::details::log_msg_buffer> messages;
        
        BufferState() = default;
        explicit BufferState(spdlog::level::level_enum level) 
            : is_buffering(true), buffer_level(level) {}
    };
    
    // Thread-safe access to buffer states
    mutable std::mutex mutex_;
    std::unordered_map<std::string, BufferState> component_buffers_;
    
    // Helper to get or create buffer state
    BufferState& get_or_create_buffer_state(const std::string& component);
    const BufferState& get_buffer_state(const std::string& component) const;
};

} // namespace cprime