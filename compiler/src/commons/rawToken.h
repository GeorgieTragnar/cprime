#pragma once

#include "enum/token.h"
#include "enum/rawToken.h"
#include "dirty/string_table.h"
#include <variant>
#include <string>
#include <cstdint>

namespace cprime {

struct RawToken {
	ERawToken _raw_token = ERawToken::INVALID;
	EToken _token = EToken::INVALID;
	
	uint32_t _line = UINT32_MAX;
	uint32_t _column = UINT32_MAX;
	uint32_t _position = UINT32_MAX;
	
	std::variant<
		std::monostate,                // No value
		int32_t,                       // INT_LITERAL
		uint32_t,                      // UINT_LITERAL  
		int64_t,                       // LONG_LITERAL
		uint64_t,                      // ULONG_LITERAL
		float,                         // FLOAT_LITERAL
		double,                        // DOUBLE_LITERAL
		bool,                          // BOOL_LITERAL
		StringIndex                    // IDENTIFIER, STRING_LITERAL, COMMENT, etc.
		> _literal_value;
};

} // namespace cprime