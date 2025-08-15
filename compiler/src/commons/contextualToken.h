#pragma once

#include <vector>
#include <memory>
#include "enum/contextualToken.h"

namespace cprime {

// Forward declaration to avoid circular includes
class Context;

struct ContextualToken {
	EContextualToken _contextualToken;
	std::vector<uint32_t> _parentTokenIndices;
	std::vector<std::shared_ptr<Context>> _contexts;
};

} // namespace cprime