#include "layer1.h"

namespace cprime {

// Layer 1 main function - orchestrates sublayers
std::vector<RawToken> layer1(std::stringstream& stream, StringTable& string_table, ExecAliasRegistry& exec_alias_registry) {
    // Layer 1A: Extract unambiguous tokens + state machine + exec alias detection
    auto retVal1 = layer1_sublayers::sublayer1a(stream, exec_alias_registry);
    
    // Layer 1B: Extract string/char literals (prefix-aware)
    auto retVal2 = layer1_sublayers::sublayer1b(retVal1, string_table);
    
    // Layer 1C: Extract operators that can't be part of identifiers
    auto retVal3 = layer1_sublayers::sublayer1c(retVal2);
    
    // Layer 1D: Extract number literals (suffix-aware)
    auto retVal4 = layer1_sublayers::sublayer1d(retVal3);
    
    // Layer 1E: Extract keywords and convert remaining to identifiers + exec alias recognition
    auto retVal5 = layer1_sublayers::sublayer1e(retVal4, string_table, exec_alias_registry);
    return retVal5;
}

} // namespace cprime