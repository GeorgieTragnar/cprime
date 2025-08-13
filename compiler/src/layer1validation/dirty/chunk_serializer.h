#pragma once

#include "../layer1/tokenizer.h"
#include "../commons/dirty/string_table.h"
#include <string>
#include <vector>

namespace cprime::layer1validation {

/**
 * ProcessingChunk serialization utilities for Layer 1 validation and testing.
 * 
 * Provides human-readable serialization of ProcessingChunk objects for:
 * - Integration test expected output comparison
 * - CLI debugging utilities  
 * - Test failure log generation
 * 
 * Serialization Format:
 * CHUNK[UNPROCESSED]: content="main()", start=0, end=6, line=1, col=1
 * CHUNK[PROCESSED]: raw=IDENTIFIER, token=IDENTIFIER, pos=0, line=1, col=1, value=StringIndex[0]:"main"
 */
class ChunkSerializer {
public:
    // ========================================================================
    // Single Chunk Serialization
    // ========================================================================
    
    /**
     * Serialize ProcessingChunk to human-readable format.
     * 
     * For unprocessed chunks:
     * CHUNK[UNPROCESSED]: content="some code", start=10, end=19, line=2, col=5
     * 
     * For processed chunks (with RawToken):
     * CHUNK[PROCESSED]: raw=IDENTIFIER, token=IDENTIFIER, pos=15, line=2, col=10, value=StringIndex[3]:"variable"
     * 
     * @param chunk ProcessingChunk to serialize
     * @param string_table StringTable for resolving StringIndex values
     * @return Human-readable string representation
     */
    static std::string serialize(const ProcessingChunk& chunk, const StringTable& string_table);
    
    /**
     * Deserialize ProcessingChunk from string format.
     * Used for loading test case expected output files.
     * 
     * @param serialized Serialized chunk string
     * @param string_table StringTable for interning strings during deserialization
     * @return ProcessingChunk object reconstructed from string
     * @throws std::invalid_argument if format is invalid
     */
    static ProcessingChunk deserialize(const std::string& serialized, StringTable& string_table);
    
    // ========================================================================
    // Batch Chunk Serialization
    // ========================================================================
    
    /**
     * Serialize vector of ProcessingChunks to multiline string.
     * Each chunk is serialized on a separate line.
     * 
     * @param chunks Vector of ProcessingChunk objects to serialize
     * @param string_table StringTable for resolving StringIndex values
     * @return Multiline string with one chunk per line
     */
    static std::string serialize_chunks(const std::vector<ProcessingChunk>& chunks, const StringTable& string_table);
    
    /**
     * Parse multiline chunk serialization back to vector.
     * Used for loading test case expected output files.
     * 
     * @param serialized Multiline serialized chunks string
     * @param string_table StringTable for interning strings during parsing
     * @return Vector of ProcessingChunk objects
     * @throws std::invalid_argument if any line has invalid format
     */
    static std::vector<ProcessingChunk> parse_chunks(const std::string& serialized, StringTable& string_table);
    
    // ========================================================================
    // Validation Utilities
    // ========================================================================
    
    /**
     * Check if a string is a valid chunk serialization format.
     * @param serialized String to validate
     * @return true if format is valid (parseable)
     */
    static bool is_valid_chunk_format(const std::string& serialized);
    
    /**
     * Compare two ProcessingChunk vectors for equality.
     * Provides detailed diff information on mismatch.
     * 
     * @param expected Expected chunks
     * @param actual Actual chunks  
     * @param string_table StringTable for error message formatting
     * @return Empty string if equal, detailed diff message if different
     */
    static std::string compare_chunks(const std::vector<ProcessingChunk>& expected, 
                                     const std::vector<ProcessingChunk>& actual,
                                     const StringTable& string_table);

private:
    // Helper functions for parsing
    static std::string escape_string(const std::string& str);
    static std::string unescape_string(const std::string& str);
    static ProcessingChunk parse_unprocessed_chunk(const std::string& line);
    static ProcessingChunk parse_processed_chunk(const std::string& line, StringTable& string_table);
};

} // namespace cprime::layer1validation