#pragma once

#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#include "enum/contextualizationError.h"

namespace cprime {

// Forward declaration for source location
struct SourceLocation {
    std::string file_name;
    uint32_t line = 0;
    uint32_t column = 0;
    uint32_t offset = 0;
};

struct ContextualizationError {
    ContextualizationErrorType error_type;
    std::string extra_info;
    std::vector<uint32_t> token_indices;
    uint32_t scope_index;
    uint32_t instruction_index;
    InstructionType instruction_type;
    // Source location resolved later by orchestrator
    SourceLocation source_location;
};

// Function pointer type for error reporting
using ErrorReporter = std::function<void(ContextualizationErrorType error_type,
                                        const std::string& extra_info,
                                        const std::vector<uint32_t>& token_indices)>;

} // namespace cprime