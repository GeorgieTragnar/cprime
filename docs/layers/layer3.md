# Layer 3 - Semantic Substitution & Organization

## Responsibilities

- **Context-sensitive keyword resolution**: Disambiguate `runtime`/`defer`/`exposes` based on surrounding context
- **CPrime pattern recognition**: Identify language constructs like RAII allocation, class declarations, template usage
- **Contextual token creation**: Generate ContextualToken abstractions with N:M mapping to original tokens
- **Semantic organization**: Structure data optimally for logical implementation layers (4+)
- **Lossless transformation**: Maintain perfect reconstruction capability while adding semantic abstractions

## Input/Output

- **Input**: Hierarchical `std::vector<Scope>` with Instructions and Token references from Layer 2
- **Output**: Enhanced scope structure with populated ContextualToken vectors
- **Preservation**: All original data remains accessible through Token→RawToken references
- **Dependencies**: Layer 2 output, StringTable (read-only), Context system for semantic cache

## Core Philosophy - Substitutional Transformation

### **Semantic Recognition Without Implementation**
Layer 3 identifies and abstracts CPrime language patterns without implementing their behavior:

- **`runtime defer auto var x`** → `RAII_ALLOCATION` contextual token
- **`class template<T> MyClass`** → `TEMPLATE_CLASS_DECLARATION` contextual token  
- **`if (condition) { ... }`** → `CONDITIONAL_BLOCK` contextual token

### **Lossless Abstraction**
- **Bidirectional mapping**: ContextualToken + StringTable + original tokens = complete source reconstruction
- **No information loss**: All original tokens preserved and referenced
- **Substitutional only**: No new semantics added, only recognition and organization

## ContextualToken Creation Process

### **N:M Token Mapping**
```cpp
struct ContextualToken {
    EContextualToken _contextualToken;              // Semantic meaning
    std::vector<uint32_t> _parentTokenIndices;      // Original token references  
    std::vector<std::shared_ptr<Context>> _contexts; // Token-level cache data
};
```

### **Flexible Transformation Patterns**
- **Compression**: Multiple tokens → single contextual token
  - `["runtime", "defer", "auto", "var"]` → single `RAII_ALLOCATION`
- **Expansion**: Single token → multiple contextual tokens
  - `"class"` → `[CLASS_KEYWORD, SCOPE_DECLARATION, TYPE_DEFINITION]`
- **Identity**: One-to-one mapping for simple constructs
- **Complex**: Any combination based on semantic requirements

## Context-Sensitive Analysis

### **Keyword Disambiguation**
Layer 3 resolves context-sensitive keywords that Layer 1 couldn't distinguish:

#### **Runtime Context Resolution**
- **Allocation context**: `runtime auto var x` → RUNTIME_ALLOCATION
- **Function context**: `runtime function foo()` → RUNTIME_FUNCTION_DECLARATION  
- **Type context**: `runtime<T> class MyClass` → RUNTIME_TEMPLATE_CLASS

#### **Defer Context Resolution**  
- **Statement context**: `defer cleanup();` → DEFER_STATEMENT
- **Modifier context**: `runtime defer auto var` → part of RAII_ALLOCATION pattern
- **Function context**: `defer function destructor()` → DEFER_FUNCTION_DECLARATION

#### **Exposes Context Resolution**
- **Interface context**: `class MyClass exposes IInterface` → INTERFACE_IMPLEMENTATION
- **Access context**: `private exposes internal_method` → ACCESS_MODIFICATION
- **Template context**: `template<T> exposes concept Constraint` → CONCEPT_CONSTRAINT

### **Scope-Aware Analysis**
- **Signature vs content**: Different interpretation based on scope position
- **Nesting context**: Template/class/function nesting affects meaning
- **Parent scope influence**: Inherited context from parent scopes

## EContextualToken Taxonomy

### **Core Language Constructs**
- **CLASS_DECLARATION**: Class/struct/union declarations
- **FUNCTION_DECLARATION**: Function signatures and definitions
- **TEMPLATE_DECLARATION**: Template class/function definitions
- **INTERFACE_DECLARATION**: Interface definitions and implementations

### **Memory Management Patterns**
- **RAII_ALLOCATION**: `runtime defer auto var` patterns
- **STACK_ALLOCATION**: `auto var` without runtime
- **HEAP_ALLOCATION**: `runtime var` without defer
- **MANUAL_ALLOCATION**: Explicit memory management

### **Control Flow Abstractions**
- **CONDITIONAL_BLOCK**: if/else/switch constructs
- **LOOP_BLOCK**: for/while/do-while constructs
- **TRY_BLOCK**: Exception handling blocks
- **NAKED_BLOCK**: Plain scope blocks

### **Access Control & Visibility**
- **ACCESS_MODIFIER**: private/protected/public declarations
- **EXPOSES_CLAUSE**: Interface exposure patterns
- **FRIEND_DECLARATION**: Friend class/function declarations

### **Template System**
- **TEMPLATE_PARAMETER**: Template parameter declarations
- **TEMPLATE_INSTANTIATION**: Template usage and instantiation
- **CONCEPT_CONSTRAINT**: Template concept applications
- **TEMPLATE_SPECIALIZATION**: Template specializations

## Semantic Context Integration

### **Layer 3 Semantic Contexts**
```cpp
class SemanticContext : public Context {
public:
    CPrimePattern detected_pattern;
    float confidence_score;
    std::vector<std::string> alternative_interpretations;
    uint32_t template_nesting_depth;
    ScopeSemantics parent_scope_influence;
    // Other semantic metadata
};
```

### **Context-Driven Processing**
- **Pattern confidence**: Track certainty of semantic interpretations
- **Alternative meanings**: Store multiple possible interpretations for ambiguous constructs
- **Template context**: Track template nesting and parameter propagation
- **Scope semantics**: Maintain semantic context from parent scopes

## CPrime Language Coverage

### **Complete Language Support**
Layer 3 handles all designed CPrime constructs:

- **Three-class system**: Data/Functional/Danger class recognition
- **Capability security**: Access rights and permission patterns  
- **Template system**: Full template syntax including concepts
- **Memory management**: All RAII patterns and manual management
- **Concurrency**: Channel and coroutine pattern recognition
- **C++ compatibility**: Interop patterns and danger zone identification

### **Error Detection and Warnings**
- **Semantic errors**: Illegal keyword combinations, invalid patterns
- **Usage warnings**: Deprecated patterns, potential issues
- **Style guidance**: CPrime idiom recommendations
- **Context violations**: Misuse of context-sensitive keywords

## Processing Algorithm

### **Multi-Pass Analysis**
1. **Context establishment**: Determine scope and parent context for each instruction
2. **Pattern matching**: Identify CPrime constructs in token sequences
3. **Disambiguation**: Resolve context-sensitive keywords using scope analysis
4. **ContextualToken creation**: Generate semantic abstractions with token mappings
5. **Validation**: Verify semantic correctness and generate warnings/errors

### **Embarrassingly Parallel Processing**
- **Scope independence**: Each scope can be processed independently after context establishment
- **Instruction parallelism**: Instructions within scopes can be processed concurrently
- **GPU-friendly**: Flat scope vector enables efficient parallel processing

## Layer 4+ Interface Preparation

### **Implementation-Ready Organization**
Layer 3 provides perfectly organized data for implementation layers:

- **Semantic abstractions**: High-level constructs ready for implementation
- **Original preservation**: All implementation details accessible through references  
- **Context attachment**: Rich cache system for implementation-specific data
- **Error correlation**: Precise source mapping for implementation error reporting

### **Implementation Filter Foundation**
Layer 3 creates the foundation for Layer 4's implementation filtering:

- **Complete semantic validation**: All CPrime constructs recognized and validated
- **Implementation mapping**: ContextualTokens map directly to implementation requirements
- **Feature detection**: Clear identification of which language features are used

## Performance Characteristics

### **Memory Efficiency**
- **Reference-based**: ContextualTokens reference original data, minimal duplication
- **Context sharing**: shared_ptr enables efficient context data sharing
- **Incremental processing**: Can process scopes incrementally as needed

### **Processing Speed**
- **Pattern-driven**: Efficient pattern matching using token sequence analysis
- **Context caching**: Reuse scope context for multiple instructions
- **Parallel-friendly**: Independent scope processing enables concurrency

## Key Design Principles

### **Lossless Semantic Enhancement**
- **Complete preservation**: Original tokens + ContextualTokens + StringTable = full reconstruction
- **Substitutional transformation**: Recognition without modification
- **Bidirectional mapping**: Forward and reverse transformations supported

### **Implementation Preparation**
- **Semantic completeness**: Full CPrime language recognition
- **Implementation independence**: No implementation logic mixed with recognition
- **Context richness**: Comprehensive semantic information for implementation layers

### **Error Excellence** 
- **Precise correlation**: Every semantic error tied to exact source locations
- **Context-aware messages**: Error messages understand CPrime semantic context
- **Developer-friendly**: Clear explanations of semantic violations and suggestions

### **Extensible Architecture**
- **Context system**: Layers can extend semantic analysis through contexts
- **Pattern extensibility**: New CPrime constructs can be added without core changes
- **Implementation flexibility**: Multiple implementation strategies supported by same semantic layer