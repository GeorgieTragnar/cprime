#include "string_table.h"
#include <stdexcept>
#include <algorithm>

namespace cprime {

StringTable::StringIndex StringTable::intern(const std::string& str) {
    // Check if string already exists
    auto it = string_to_index_.find(str);
    if (it != string_to_index_.end()) {
        return it->second; // Return existing index
    }
    
    // Add new string to table
    StringIndex index = static_cast<StringIndex>(strings_.size());
    strings_.push_back(str);
    string_to_index_[str] = index;
    
    return index;
}

const std::string& StringTable::get_string(StringIndex index) const {
    if (index >= strings_.size()) {
        throw std::out_of_range("StringTable: Invalid string index " + std::to_string(index) + 
                               ", table size is " + std::to_string(strings_.size()));
    }
    return strings_[index];
}

StringTable::Statistics StringTable::get_statistics() const {
    Statistics stats;
    stats.unique_strings = strings_.size();
    stats.total_characters = 0;
    stats.largest_string_length = 0;
    
    for (const auto& str : strings_) {
        stats.total_characters += str.length();
        stats.largest_string_length = std::max(stats.largest_string_length, str.length());
    }
    
    stats.average_string_length = stats.unique_strings > 0 ? 
        stats.total_characters / stats.unique_strings : 0;
        
    return stats;
}

void StringTable::clear() {
    strings_.clear();
    string_to_index_.clear();
}

void StringTable::reserve(size_t expected_strings) {
    strings_.reserve(expected_strings);
    string_to_index_.reserve(expected_strings);
}

} // namespace cprime