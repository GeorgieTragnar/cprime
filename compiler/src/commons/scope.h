#pragma once

#include <variant>
#include <memory>
#include "instruction.h"
#include "context.h"

namespace cprime {

enum class ScopeType {
	FUNCTION,      // Function definition scope
	BLOCK,         // Basic block scope (anonymous)
	EXEC,          // Exec block for comptime execution
	CONDITIONAL,   // If/else scope
	LOOP,          // For/while loop scope
	ROOT           // Top-level/global scope
};

struct Scope {
	Instruction _header; // it can hold parent var in case of lambda
	Instruction _footer; // semicolon-terminated instructions at scope end
	uint32_t _parentScopeIndex;
	ScopeType _scopeType;
	std::variant<Instruction, uint32_t> _instructions; // it holds instructions or index for nested scope
	std::vector<std::shared_ptr<Context>> _contexts;
};

} // namespace cprime