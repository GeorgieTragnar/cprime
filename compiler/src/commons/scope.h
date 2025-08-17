#pragma once

#include <variant>
#include <memory>
#include <vector>
#include <string>
#include "instruction.h"
#include "context.h"

namespace cprime {

struct Scope {
	Instruction _header; // it can hold parent var in case of lambda
	std::variant<Instruction, uint32_t> _footer; // instruction or scope index for exec replacement
	uint32_t _parentScopeIndex;
	std::vector<std::variant<Instruction, uint32_t>> _instructions; // it holds instructions or index for nested scope
	std::vector<std::shared_ptr<Context>> _contexts;
	
	// For deferred semantic tokenization - hierarchical namespace context
	std::vector<std::string> namespace_context; // e.g., ["std", "containers"] for nested namespaces
};

} // namespace cprime