#pragma once

#include <cstdint>

namespace cprime {

enum class ContextualizationErrorType : uint32_t {
    UNSUPPORTED_TOKEN_PATTERN = 1,
    AMBIGUOUS_OPERATOR_CONTEXT = 2,
    UNRESOLVED_IDENTIFIER = 3,
    INVALID_EXPRESSION_STRUCTURE = 4,
    MISSING_TYPE_INFORMATION = 5,
    INCOMPLETE_STATEMENT = 6,
    INVALID_FUNCTION_CALL = 7,
    TYPE_MISMATCH = 8,
    UNDECLARED_VARIABLE = 9
};

enum class ErrorSeverity : uint32_t {
    SUPPRESS = 0,
    WARNING = 1,
    ERROR = 2
};

enum class InstructionType : uint32_t {
    HEADER = 0,
    BODY = 1,
    FOOTER = 2
};

} // namespace cprime