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
/**
 * String table index wrapper - distinct type for variant compatibility.
 */
struct StringIndex {
    uint32_t value = UINT32_MAX;
};

class StringTable {
public:
    
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
        return index.value < strings_.size();
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
    std::vector<std::string> strings_;                            // Indexed string storage
    std::unordered_map<std::string, StringIndex> string_to_index_; // Fast lookup for interning
};

} // namespace cprime

// Hash specialization for StringIndex to enable use in std::unordered_map
namespace std {
    template<>
    struct hash<cprime::StringIndex> {
        size_t operator()(const cprime::StringIndex& index) const {
            return hash<uint32_t>{}(index.value);
        }
    };
}

// Equality operator for StringIndex
namespace cprime {
    inline bool operator==(const StringIndex& lhs, const StringIndex& rhs) {
        return lhs.value == rhs.value;
    }
    
    inline bool operator!=(const StringIndex& lhs, const StringIndex& rhs) {
        return lhs.value != rhs.value;
    }
}