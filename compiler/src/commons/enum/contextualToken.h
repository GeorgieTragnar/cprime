#pragma once

#include <cstdint>

namespace cprime {

enum class EContextualToken : uint32_t {
	INVALID = 0,
	VARIABLE_DECLARATION = 1,
	VARIABLE_REFERENCE = 2,
	ASSIGNMENT = 3,
	FUNCTION_CALL = 4,
	CONTROL_FLOW = 5,
	EXPRESSION = 6,
	TYPE_REFERENCE = 7,
	OPERATOR = 8,
	LITERAL_VALUE = 9,
	SCOPE_REFERENCE = 10,
	WHITESPACE = 11,
	FORMATTING = 12,
	RESOURCE_MANAGEMENT = 13
};

} // namespace cprime