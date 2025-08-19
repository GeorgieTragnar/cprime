#pragma once

#include <cstdint>

namespace cprime {

enum class EContextualToken : uint32_t {
	INVALID = 0,
	VARIABLE_DECLARATION = 1,
	ASSIGNMENT = 2,
	FUNCTION_CALL = 3,
	CONTROL_FLOW = 4,
	EXPRESSION = 5,
	TYPE_REFERENCE = 6,
	OPERATOR = 7,
	LITERAL_VALUE = 8,
	SCOPE_REFERENCE = 9,
	WHITESPACE = 10,
	FORMATTING = 11
};

} // namespace cprime