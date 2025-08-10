# Commons - Core Data Structures

## Architecture Overview

The commons module provides the core data structures that enable the CPrime compiler's layered architecture. These are pure data structures without constructors or methods, following a clean separation between data and behavior.

## Core Data Pipeline

```
Raw Source → RawToken → Token → Instruction → Scope → ContextualToken
```

## Core Data Structures

### **RawToken** - Layer 1 Output
Pure data structure representing tokenized source code with dual classification:

- **`ERawToken _raw_token`** - Structural category for Layer 2 processing
- **`EToken _token`** - Specific token type for detailed analysis
- **Position data** - `_line`, `_column`, `_position` for source correlation
- **Literal variant** - Type-safe storage for int/float/string/bool values via StringIndex

**Purpose**: Layer 1's output providing both structural hints and detailed token information.

### **Token** - Layer 2+ Reference Structure
Lightweight reference into raw token streams:

- **`uint32_t _stringstreamId`** - Which input stream this references
- **`uint32_t _tokenIndex`** - Index into the raw token stream
- **`EToken _token`** - Token type (duplicated for quick access)

**Purpose**: Efficient token references that maintain source correlation while enabling grouped processing.

### **Instruction** - Token Group Container
Groups related tokens with extensible cache system:

- **`std::vector<Token> _tokens`** - Token sequence
- **`std::vector<ContextualToken> _contextualTokens`** - Semantic abstractions
- **`std::vector<std::shared_ptr<Context>> _contexts`** - Layer-specific cache data

**Purpose**: Fundamental unit of CPrime code organization, representing statements, expressions, and declarations.

### **Scope** - Hierarchical Code Structure
Represents code scope boundaries with nested relationships:

- **`Instruction _header`** - Scope signature (class declaration, function signature, etc.)
- **`uint32_t _parentScopeIndex`** - Parent scope reference
- **`std::variant<Instruction, uint32_t> _instructions`** - Either instruction data or nested scope index
- **`std::vector<std::shared_ptr<Context>> _contexts`** - Scope-specific cache data

**Purpose**: Hierarchical code organization supporting both linear instructions and nested structures.

### **ContextualToken** - Semantic Abstraction
Flexible N:M mapping between original tokens and semantic meaning:

- **`EContextualToken _contextualToken`** - Semantic classification (RAII_ALLOCATION, CLASS_DECLARATION, etc.)
- **`std::vector<uint32_t> _parentTokenIndices`** - References to original tokens in instruction
- **`std::vector<std::shared_ptr<Context>> _contexts`** - Token-level cache data

**Purpose**: Enables semantic substitution while preserving original token structure for reconstruction.

## Enum System

### **EToken** - Detailed Token Classification
Complete CPrime token taxonomy including literals, keywords, operators, punctuation, and special tokens. Used for precise token identification and processing.

### **ERawToken** - Structural Categories  
Simplified classification focused on Layer 2 structural needs:
- **Structural**: `LEFT_BRACE`, `RIGHT_BRACE`, `SEMICOLON`
- **Data-carrying**: `IDENTIFIER`, `LITERAL`  
- **Non-structural**: `KEYWORD`, `WHITESPACE`, `COMMENT`, `NEWLINE`, `EOF_TOKEN`

### **EContextualToken** - Semantic Abstractions
High-level semantic constructs representing CPrime language patterns (RAII_ALLOCATION, TEMPLATE_DECLARATION, CONDITIONAL_EXPRESSION, etc.).

## Context System

### **Abstract Context Class**
Pure abstract base class enabling extensible cache system:

```cpp
class Context {
public:
    virtual ~Context() = default;
};
```

### **Layer-Specific Contexts**
Each layer can define custom Context subclasses for cache data:

```cpp
class StructuralContext : public Context { /* Layer 2 data */ };
class SemanticContext : public Context { /* Layer 3 data */ };
```

### **Context Access Pattern**
Layers iterate context vectors and use `dynamic_pointer_cast` for type discovery:

```cpp
for(auto ctx : instruction._contexts) {
    if(auto structural = std::dynamic_pointer_cast<StructuralContext>(ctx)) {
        // Use structural context data
    }
}
```

## String Table Integration

### **Controlled Access Pattern**
- **Layer 1**: Mutable access for string interning
- **Layers 2+**: Read-only access for string retrieval
- **StringIndex**: Wrapper struct for variant type safety

### **Memory Efficiency**
Global string deduplication eliminates duplicate storage while maintaining fast index-based access.

## Key Design Principles

### **Pure Data Structures**
- No constructors or methods in core structures
- Clean separation between data and behavior
- Underscore prefix convention for all fields

### **Index-Based References**
- Tokens reference raw streams by index
- Scopes reference parents by index
- Contextual tokens reference original tokens by index

### **Lossless Transformation**
- Original data preservation enables reconstruction
- Layer transformations are substitutional, not destructive
- String table + layer outputs can rebuild original source

### **Extensible Cache System**
- shared_ptr<Context> vectors in all major structures
- Layers can attach custom cache data without modifying core structures
- Dynamic typing enables flexible data access

## Memory Management

- **RAII**: shared_ptr ensures automatic cleanup
- **Sharing**: Context data can be shared between structures
- **Performance**: Index-based references minimize memory overhead
- **Thread Safety**: Reference counting handles concurrent access

## Interface

- **Input**: None (pure data structures)
- **Output**: Foundation types for all compiler layers
- **Dependencies**: STL containers, memory management utilities