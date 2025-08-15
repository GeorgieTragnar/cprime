#pragma once

#include "../commons/rawToken.h"
#include "../commons/dirty/string_table.h"
#include "../layer1/processing_chunk.h" // For ProcessingChunk definition
#include <vector>
#include <string>
#include <sstream>

namespace cprime {

// Layer 2 main function - what compiler will use (replaces Tokenizer::tokenize_stream)
std::vector<RawToken> layer2(std::stringstream& stream, StringTable& string_table);

// Layer 2 sublayer implementations in nested namespace
namespace layer2_sublayers {
    // Layer 2A: Extract unambiguous single-character tokens + state machine
    std::vector<ProcessingChunk> sublayer2a(std::stringstream& stream);
    
    // Layer 2B: Extract string and character literals (prefix-aware)
    std::vector<ProcessingChunk> sublayer2b(const std::vector<ProcessingChunk>& input, StringTable& string_table);
    
    // Layer 2C: Extract operators that can never be part of identifiers
    std::vector<ProcessingChunk> sublayer2c(const std::vector<ProcessingChunk>& input);
    
    // Layer 2D: Extract number literals (suffix-aware)
    std::vector<ProcessingChunk> sublayer2d(const std::vector<ProcessingChunk>& input);
    
    // Layer 2E: Extract keywords and convert remaining strings to identifiers
    std::vector<RawToken> sublayer2e(const std::vector<ProcessingChunk>& input, StringTable& string_table);
}

} // namespace cprime