#pragma once

// CPrime Compiler - Component Name Constants
// These macros prevent typos when specifying component names for logging and buffering

namespace cprime {

// Component name constants to prevent typos and ensure consistency
#define CPRIME_COMPONENT_COMPILER "compiler"
#define CPRIME_COMPONENT_TOKENIZER "tokenizer" 
#define CPRIME_COMPONENT_AST_BUILDER "ast_builder"
#define CPRIME_COMPONENT_CONTEXT_ENRICHER "context_enricher"
#define CPRIME_COMPONENT_VALIDATOR "validator"
#define CPRIME_COMPONENT_CLI "cli"
#define CPRIME_COMPONENT_RAII_INJECTOR "raii_injector"
#define CPRIME_COMPONENT_SEMANTIC_TRANSLATOR "semantic_translator"
#define CPRIME_COMPONENT_SYMBOL_TABLE "symbol_table"
#define CPRIME_COMPONENT_PIPELINE "pipeline"

// Layer-specific components
#define CPRIME_COMPONENT_LAYER1 "layer1"
#define CPRIME_COMPONENT_LAYER2 "layer2" 
#define CPRIME_COMPONENT_LAYER3 "layer3"
#define CPRIME_COMPONENT_LAYER4 "layer4"

// Validation components
#define CPRIME_COMPONENT_TOKEN_VALIDATOR "token_validator"
#define CPRIME_COMPONENT_CONTEXT_VALIDATOR "context_validator"
#define CPRIME_COMPONENT_AST_VALIDATOR "ast_validator"
#define CPRIME_COMPONENT_RAII_VALIDATOR "raii_validator"

// Testing components  
#define CPRIME_COMPONENT_TESTS "tests"
#define CPRIME_COMPONENT_UNIT_TESTS "unit_tests"
#define CPRIME_COMPONENT_INTEGRATION_TESTS "integration_tests"

} // namespace cprime