# CPrime Compiler - Three-Layer Architecture

This directory contains the new three-layer compiler architecture for CPrime, designed to handle context-sensitive keywords and provide a clean separation between syntax, semantics, and code generation.

## Architecture Overview

### Layer 1: Raw Token Parser
- **File**: `raw_token.h/cpp`
- **Purpose**: Pure syntactic tokenization without semantic interpretation
- **Input**: CPrime source code strings
- **Output**: Stream of raw tokens (keywords, identifiers, operators, literals, punctuation)

### Layer 2: Semantic Token Translator
- **Files**: `semantic_translator.h/cpp`, `context_stack.h/cpp`, `semantic_token.h/cpp`
- **Purpose**: Context-sensitive keyword resolution and semantic disambiguation
- **Input**: Raw token stream + parsing context
- **Output**: Semantic tokens with unambiguous meaning

### Layer 3: Code Generator (TODO)
- **Files**: `llvm_codegen_v2.h/cpp` (planned)
- **Purpose**: Generate LLVM IR from semantic tokens
- **Input**: Stream of semantic tokens
- **Output**: LLVM IR bitcode ready for optimization and compilation

## Key Features

### Context-Sensitive Keywords
The new architecture handles keywords that have different meanings based on parsing context:

- **`runtime`**: 
  - In access rights: `runtime exposes UserOps { ... }` (vtable-based polymorphism)
  - In type parameters: `Connection<runtime UserOps>` (runtime dispatch)
  - In unions: `union runtime ConnectionSpace { ... }` (dynamic unions)

- **`defer`**:
  - In function bodies: `defer FileOps::destruct(&mut file)` (RAII cleanup)
  - In coroutines: `co_defer cleanup_resources()` (coroutine-specific defer)

- **`exposes`**:
  - Default: `exposes UserOps { ... }` (compile-time access rights)
  - With runtime: `runtime exposes UserOps { ... }` (runtime polymorphism)

### Incremental Development
The architecture supports incremental feature development:
- **Feature Registry**: Track implementation status of each semantic token type
- **Placeholder Tokens**: Unimplemented features generate helpful error messages
- **Hybrid Compilation**: Can switch between old and new implementations per feature

## Building and Testing

### Build the compiler Components
```bash
cd /path/to/cprime/compiler/src/v2
mkdir build && cd build
cmake ..
make
```

### Run Tests
```bash
./test_three_layer
```

This will test all three layers of the architecture with sample CPrime code.

## Current Implementation Status

### ✅ Implemented
- Raw tokenization with comprehensive keyword support
- Context stack system for parsing state management
- Semantic token definitions with factory methods
- Basic semantic translator with context-sensitive resolution
- Feature registry for tracking implementation progress

### 🚧 In Progress
- LLVM IR generation from semantic tokens
- Complete keyword resolution for all language constructs
- Comprehensive error handling and recovery

### 📋 Planned
- Integration with existing compiler (hybrid mode)
- Advanced language features (coroutines, generics, etc.)
- Optimization passes at semantic level
- LSP integration for IDE support

## Usage Example

```cpp
#include "layer1/raw_token.h"
#include "layer2/semantic_translator.h"

using namespace cprime::v2;

// Sample CPrime code with context-sensitive keywords
std::string cprime_code = R"(
    class Connection {
        handle: DbHandle,
        runtime exposes UserOps { handle }
        exposes AdminOps { handle }
    }
    
    functional class FileOps {
        fn read(data: &FileData) -> Result<usize> {
            defer FileOps::cleanup(&data);
            // implementation
        }
    }
)";

// Layer 1: Raw tokenization
RawTokenizer tokenizer(cprime_code);
auto raw_stream = tokenizer.tokenize_to_stream();

// Layer 2: Semantic translation
SemanticTranslator translator(std::move(raw_stream));
auto semantic_tokens = translator.translate();

// Layer 3: Code generation (when implemented)
// LLVMCodeGencompiler codegen;
// auto llvm_ir = codegen.generate(semantic_tokens);
```

## Architecture Benefits

1. **Clean Separation**: Each layer has a single responsibility
2. **Context Sensitivity**: Keywords are resolved based on parsing context
3. **Incremental Development**: Features can be implemented one semantic token at a time
4. **Extensibility**: New keywords and language constructs are easy to add
5. **Debugging**: Clear separation makes debugging and testing easier
6. **Performance**: Context-sensitive resolution happens once during parsing

## Integration with Existing Compiler

The compiler architecture is designed to coexist with the existing compiler:
- **Feature Flags**: Control which features use V1 vs compiler implementation
- **Backward Compatibility**: V1 compiler remains fully functional
- **Gradual Migration**: Features can be migrated one at a time
- **Testing**: Both implementations can be tested in parallel

This allows for safe, incremental development of the revolutionary three-layer architecture while maintaining full compatibility with existing CPrime code.