#pragma once

#include "../commons/rawToken.h"
#include "../commons/dirty/string_table.h"
#include "../commons/dirty/exec_alias_registry.h"
#include "tokenizer.h" // For ProcessingChunk definition
#include <vector>
#include <string>
#include <sstream>

namespace cprime {

// Layer 1 main function - what compiler will use (replaces Tokenizer::tokenize_stream)
std::vector<RawToken> layer1(std::stringstream& stream, StringTable& string_table, ExecAliasRegistry& exec_alias_registry);

// Layer 1 sublayer implementations in nested namespace
namespace layer1_sublayers {
    // Layer 1A: Extract unambiguous single-character tokens + state machine + exec alias detection
    std::vector<ProcessingChunk> sublayer1a(std::stringstream& stream, ExecAliasRegistry& exec_alias_registry);
    
    // Layer 1B: Extract string and character literals (prefix-aware)
    std::vector<ProcessingChunk> sublayer1b(const std::vector<ProcessingChunk>& input, StringTable& string_table);
    
    // Layer 1C: Extract operators that can never be part of identifiers
    std::vector<ProcessingChunk> sublayer1c(const std::vector<ProcessingChunk>& input);
    
    // Layer 1D: Extract number literals (suffix-aware)
    std::vector<ProcessingChunk> sublayer1d(const std::vector<ProcessingChunk>& input);
    
    // Layer 1E: Extract keywords and convert remaining strings to identifiers + exec alias recognition
    std::vector<RawToken> sublayer1e(const std::vector<ProcessingChunk>& input, StringTable& string_table, ExecAliasRegistry& exec_alias_registry);
}

} // namespace cprime