#pragma once

#include <vector>
#include <memory>
#include "enum/token.h"
#include "token.h"
#include "contextualToken.h"
#include "context.h"

namespace cprime {

struct Instruction {
	std::vector<Token> _tokens;
	std::vector<ContextualToken> _contextualTokens;
	std::vector<std::shared_ptr<Context>> _contexts;
};

} // namespace cprime