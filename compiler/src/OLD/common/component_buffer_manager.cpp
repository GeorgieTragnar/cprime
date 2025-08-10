#include "component_buffer_manager.h"
#include <algorithm>

namespace cprime {

void ComponentBufferManager::begin_buffering(const std::string& component, spdlog::level::level_enum buffer_level) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& state = get_or_create_buffer_state(component);
    state.is_buffering = true;
    state.buffer_level = buffer_level;
    
    // Clear any existing messages when starting new buffering session
    state.messages.clear();
}

void ComponentBufferManager::end_buffering(const std::string& component) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = component_buffers_.find(component);
    if (it != component_buffers_.end()) {
        it->second.is_buffering = false;
    }
}

bool ComponentBufferManager::should_buffer(const std::string& component, spdlog::level::level_enum msg_level) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = component_buffers_.find(component);
    if (it == component_buffers_.end() || !it->second.is_buffering) {
        return false;
    }
    
    // Buffer if message level is at or above the configured buffer level
    return msg_level >= it->second.buffer_level;
}

void ComponentBufferManager::add_to_buffer(const std::string& component, const spdlog::details::log_msg& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = component_buffers_.find(component);
    if (it != component_buffers_.end() && it->second.is_buffering) {
        // Create a log_msg_buffer to store the message
        it->second.messages.emplace_back(msg);
    }
}

std::vector<spdlog::details::log_msg_buffer> ComponentBufferManager::get_buffer_messages(const std::string& component) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const auto& state = get_buffer_state(component);
    return state.messages; // Return copy
}

void ComponentBufferManager::clear_buffer(const std::string& component) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = component_buffers_.find(component);
    if (it != component_buffers_.end()) {
        it->second.messages.clear();
    }
}

bool ComponentBufferManager::is_buffering(const std::string& component) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = component_buffers_.find(component);
    return it != component_buffers_.end() && it->second.is_buffering;
}

spdlog::level::level_enum ComponentBufferManager::get_buffer_level(const std::string& component) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const auto& state = get_buffer_state(component);
    return state.is_buffering ? state.buffer_level : spdlog::level::off;
}

size_t ComponentBufferManager::get_buffer_count(const std::string& component) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const auto& state = get_buffer_state(component);
    return state.messages.size();
}

std::vector<std::string> ComponentBufferManager::get_buffering_components() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> buffering_components;
    buffering_components.reserve(component_buffers_.size());
    
    for (const auto& [component, state] : component_buffers_) {
        if (state.is_buffering) {
            buffering_components.push_back(component);
        }
    }
    
    return buffering_components;
}

ComponentBufferManager::BufferState& ComponentBufferManager::get_or_create_buffer_state(const std::string& component) {
    auto it = component_buffers_.find(component);
    if (it == component_buffers_.end()) {
        // Create new empty buffer state
        it = component_buffers_.emplace(component, BufferState()).first;
    }
    return it->second;
}

const ComponentBufferManager::BufferState& ComponentBufferManager::get_buffer_state(const std::string& component) const {
    static const BufferState empty_state; // Default empty state
    
    auto it = component_buffers_.find(component);
    if (it == component_buffers_.end()) {
        return empty_state;
    }
    return it->second;
}

} // namespace cprime