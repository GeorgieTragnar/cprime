#pragma once

#include <string>
#include <sstream>
#include <vector>
#include "../commons/rawToken.h"
#include "../layer1/tokenizer.h"

// Layer 2 validation interface for testing
// This header provides validation and serialization functions for layer 2 testing

namespace cprime {
namespace layer2_sublayers {
namespace validation {

/**
 * Serialize layer 2 intermediate output (ProcessingChunk vector) to string for testing
 * This function converts intermediate processing chunks to a string representation for comparison
 */
inline std::string serialize(const std::vector<cprime::ProcessingChunk>& chunks) {
    // TODO: Implement proper serialization
    // For now, return a simple placeholder to avoid compilation errors
    return "STUB_SERIALIZED_CHUNKS_COUNT_" + std::to_string(chunks.size());
}

/**
 * Serialize layer 2 output (RawToken vector) to string for testing
 * This function converts the output of layer2 to a string representation for comparison
 */
inline std::string serialize(const std::vector<cprime::RawToken>& tokens) {
    // TODO: Implement proper serialization
    // For now, return a simple placeholder to avoid compilation errors
    return "STUB_SERIALIZED_TOKENS_COUNT_" + std::to_string(tokens.size());
}

/**
 * Deserialize test input for layer 2
 * This function converts test case input to the format expected by layer2
 * For layer 2, this should return the same stringstream that was passed in
 */
inline std::stringstream& deserialize(std::stringstream& input) {
    // Layer 2 takes stringstream directly, so no conversion needed
    return input;
}

} // namespace validation
} // namespace layer2_sublayers
} // namespace cprime