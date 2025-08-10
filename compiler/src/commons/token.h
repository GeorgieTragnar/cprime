#pragma once

#include "enum/token.h"
#include <cstdint>

namespace cprime {

struct Token {
	uint32_t _stringstreamId = UINT32_MAX;
	uint32_t _tokenIndex = UINT32_MAX;
	EToken _token = EToken::INVALID;
};

} // namespace cprime