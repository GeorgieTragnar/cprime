#pragma once

#include "enum/token.h"
#include "enum/rawToken.h"
#include "dirty/string_table.h"
#include "dirty/exec_alias_registry.h"
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
	
	// For deferred semantic tokenization - StringTable index for unresolved chunk content
	StringIndex chunk_content_index;
	
	std::variant<
		std::monostate,                // No value (for non-literal tokens)
		// Integer literal types
		int32_t,                       // INT_LITERAL
		uint32_t,                      // UINT_LITERAL  
		int64_t,                       // LONG_LITERAL
		uint64_t,                      // ULONG_LITERAL
		long long,                     // LONG_LONG_LITERAL
		unsigned long long,            // ULONG_LONG_LITERAL
		// Floating-point literal types
		float,                         // FLOAT_LITERAL
		double,                        // DOUBLE_LITERAL
		long double,                   // LONG_DOUBLE_LITERAL
		// Character literal types
		char,                          // CHAR_LITERAL
		wchar_t,                       // WCHAR_LITERAL
		char16_t,                      // CHAR16_LITERAL
		char32_t,                      // CHAR32_LITERAL
		// Boolean literal
		bool,                          // TRUE_LITERAL, FALSE_LITERAL
		// String reference (all string types, identifiers, comments use StringIndex)
		StringIndex,                   // IDENTIFIER, STRING_LITERAL, WSTRING_LITERAL, etc., COMMENT
		// Exec alias reference
		ExecAliasIndex                 // EXEC_ALIAS
		> _literal_value;
};

} // namespace cprime