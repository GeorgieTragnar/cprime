#pragma once

#include <string>
#include <sstream>
#include <vector>
#include "../commons/rawToken.h"

// Layer 1 validation interface for testing
// This header provides validation and serialization functions for layer 1 testing

namespace layer1_sublayers {
namespace validation {

/**
 * Serialize layer 1 output (RawToken vector) to string for testing
 * This function converts the output of layer1 to a string representation for comparison
 */
inline std::string serialize(const std::vector<cprime::RawToken>& tokens) {
    // TODO: Implement proper serialization
    // For now, return a simple placeholder to avoid compilation errors
    return "STUB_SERIALIZED_TOKENS_COUNT_" + std::to_string(tokens.size());
}

/**
 * Deserialize test input for layer 1
 * This function converts test case input to the format expected by layer1
 * For layer 1, this should return the same stringstream that was passed in
 */
inline std::stringstream& deserialize(std::stringstream& input) {
    // Layer 1 takes stringstream directly, so no conversion needed
    return input;
}

} // namespace validation
} // namespace layer1_sublayers