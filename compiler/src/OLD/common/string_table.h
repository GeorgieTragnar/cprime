#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace cprime {

/**
 * Global string interning table for eliminating duplicate string storage.
 * Stores complete strings only (no substring optimization) for simplicity.
 * Thread-safe after construction is complete (immutable access).
 */
class StringTable {
public:
    using StringIndex = uint32_t;
    static constexpr StringIndex INVALID_INDEX = UINT32_MAX;
    
    StringTable() = default;
    
    /**
     * Intern a complete string, returning its index.
     * If the string already exists, returns the existing index.
     * If the string is new, adds it to the table and returns the new index.
     */
    StringIndex intern(const std::string& str);
    
    /**
     * Get the string associated with the given index.
     * Throws std::out_of_range if index is invalid.
     */
    const std::string& get_string(StringIndex index) const;
    
    /**
     * Check if an index is valid (within bounds).
     */
    bool is_valid_index(StringIndex index) const {
        return index < strings_.size();
    }
    
    /**
     * Get the number of unique strings stored.
     */
    size_t size() const { return strings_.size(); }
    
    /**
     * Check if the table is empty.
     */
    bool empty() const { return strings_.empty(); }
    
    /**
     * Get statistics about the string table.
     */
    struct Statistics {
        size_t unique_strings;
        size_t total_characters;
        size_t average_string_length;
        size_t largest_string_length;
    };
    
    Statistics get_statistics() const;
    
    /**
     * Clear the string table (useful for testing).
     */
    void clear();
    
    /**
     * Reserve space for expected number of strings (optimization).
     */
    void reserve(size_t expected_strings);

private:
    std::vector<std::string> strings_;                           // Indexed string storage
    std::unordered_map<std::string, StringIndex> string_to_index_; // Fast lookup for interning
};

} // namespace cprime