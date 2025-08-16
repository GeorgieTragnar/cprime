#include "exec_alias_registry.h"
#include <stdexcept>
#include <algorithm>

namespace cprime {

ExecAliasIndex ExecAliasRegistry::register_alias(const std::string& alias_name) {
    // Check for duplicate registration - assert for now with TODO for proper error handling
    auto existing = alias_to_index_.find(alias_name);
    if (existing != alias_to_index_.end()) {
        // TODO: Replace with proper error handling/reporting system
        assert(false && "Duplicate exec alias registration detected! Each exec template name must be unique.");
        return existing->second; // This line won't execute due to assertion
    }
    
    // Add new alias
    ExecAliasIndex new_index;
    new_index.value = static_cast<uint32_t>(aliases_.size());
    
    aliases_.push_back(alias_name);
    alias_to_index_[alias_name] = new_index;
    
    return new_index;
}

bool ExecAliasRegistry::contains_alias(const std::string& alias_name) const {
    return alias_to_index_.find(alias_name) != alias_to_index_.end();
}

ExecAliasIndex ExecAliasRegistry::get_alias_index(const std::string& alias_name) const {
    auto it = alias_to_index_.find(alias_name);
    if (it != alias_to_index_.end()) {
        return it->second;
    }
    // Return invalid index
    ExecAliasIndex invalid_index;
    invalid_index.value = UINT32_MAX;
    return invalid_index;
}

const std::string& ExecAliasRegistry::get_alias(ExecAliasIndex index) const {
    if (!is_valid_index(index)) {
        throw std::out_of_range("ExecAliasRegistry: Invalid alias index");
    }
    return aliases_[index.value];
}

ExecAliasRegistry::Statistics ExecAliasRegistry::get_statistics() const {
    Statistics stats;
    stats.registered_aliases = aliases_.size();
    stats.total_characters = 0;
    stats.longest_alias_length = 0;
    
    for (const auto& alias : aliases_) {
        stats.total_characters += alias.length();
        stats.longest_alias_length = std::max(stats.longest_alias_length, alias.length());
    }
    
    stats.average_alias_length = aliases_.empty() ? 0 : stats.total_characters / aliases_.size();
    
    return stats;
}

void ExecAliasRegistry::clear() {
    aliases_.clear();
    alias_to_index_.clear();
    scope_to_lambda_.clear();
    alias_to_scope_.clear();
}

void ExecAliasRegistry::reserve(size_t expected_aliases) {
    aliases_.reserve(expected_aliases);
    alias_to_index_.reserve(expected_aliases);
}

std::unordered_map<std::string, ExecAliasIndex> ExecAliasRegistry::get_all_aliases() const {
    return alias_to_index_;
}

void ExecAliasRegistry::register_scope_index(uint32_t scope_index) {
    // Create empty ExecutableLambda stub using [] access
    scope_to_lambda_[scope_index] = ExecutableLambda{};
}

void ExecAliasRegistry::register_scope_index_to_exec_alias(ExecAliasIndex alias_idx, uint32_t scope_index) {
    // Register alias index value to scope index mapping
    alias_to_scope_[alias_idx.value] = scope_index;
}

const ExecutableLambda& ExecAliasRegistry::get_executable_lambda(uint32_t scope_index) const {
    auto it = scope_to_lambda_.find(scope_index);
    if (it == scope_to_lambda_.end()) {
        throw std::out_of_range("ExecAliasRegistry: Scope index not registered as exec scope");
    }
    return it->second;
}

const ExecutableLambda& ExecAliasRegistry::get_executable_lambda_by_alias(ExecAliasIndex alias_idx) const {
    // Two-step lookup: alias index -> scope index -> executable lambda
    auto alias_it = alias_to_scope_.find(alias_idx.value);
    if (alias_it == alias_to_scope_.end()) {
        throw std::out_of_range("ExecAliasRegistry: Exec alias index not mapped to any scope");
    }
    
    uint32_t scope_index = alias_it->second;
    return get_executable_lambda(scope_index);
}

} // namespace cprime