#pragma once

#include <vector>
#include <memory>
#include "enum/contextualToken.h"
#include "instruction.h"

namespace cprime {

struct ContextualToken {
	EContextualToken _contextualToken;
	std::vector<uint32_t> _parentTokenIndices;
	std::vector<std::shared_ptr<Context>> _contexts;
};

} // namespace cprime