# Layer 2 - Structure Building

## Responsibilities

- **Cache-and-boundary methodology**: Accumulate tokens until structural delimiters (`{`, `}`, `;`)
- **Token reference creation**: Convert RawTokens into lightweight Token references
- **Instruction grouping**: Create Instruction structures containing related token sequences
- **Scope hierarchy building**: Construct flat Scope vector with index-based parent-child relationships
- **Pure structural analysis**: Focus on code organization without semantic interpretation

## Input/Output

- **Input**: `std::map<std::string, std::vector<RawToken>>` from Layer 1 + StringTable (read-only)
- **Output**: Hierarchical `std::vector<Scope>` containing structured Instructions and Tokens
- **Dependencies**: Layer 1 output, StringTable for identifier resolution, Context system for cache data

## Core Algorithm - Cache-and-Boundary

### **Token Accumulation Strategy**
Layer 2 processes tokens using a cache-and-boundary approach:

1. **Cache Phase**: Accumulate tokens into temporary buffer
2. **Boundary Detection**: Hit structural delimiter (`;`, `{`, `}`)
3. **Instruction Creation**: Convert cached tokens into Instruction
4. **Scope Management**: Handle scope entry/exit based on brace tokens

### **Boundary Token Handling**
- **SEMICOLON** (`;`) → Create instruction, continue in current scope
- **LEFT_BRACE** (`{`) → Create scope header instruction, enter new child scope  
- **RIGHT_BRACE** (`}`) → Exit current scope, return to parent

## Data Structure Transformation

### **RawToken → Token Conversion**
```cpp
// Layer 1 RawToken
struct RawToken {
    ERawToken _raw_token;
    EToken _token;
    uint32_t _line, _column, _position;
    std::variant<...> _literal_value;
};

// Layer 2 Token Reference
struct Token {
    uint32_t _stringstreamId;     // Which input stream
    uint32_t _tokenIndex;         // Index into raw token stream
    EToken _token;                // Duplicated for quick access
};
```

### **Token Reference Benefits**
- **Memory efficiency**: 12 bytes vs full RawToken structure
- **Source correlation**: Maintains link to original token data
- **Fast access**: Cached EToken for immediate classification
- **Reconstruction**: Can retrieve full RawToken details when needed

## Instruction Building Process

### **Token Sequence Grouping**
```cpp
struct Instruction {
    std::vector<Token> _tokens;                          // Token sequence
    std::vector<ContextualToken> _contextualTokens;      // Semantic abstractions (Layer 3+)
    std::vector<std::shared_ptr<Context>> _contexts;     // Cache data
};
```

### **Instruction Types Created**
- **Statements**: `var x = 5;` → Single instruction ending with semicolon
- **Declarations**: `class MyClass` → Header instruction before brace
- **Expressions**: Complex expressions split at semicolon boundaries
- **Control structures**: `if (condition)` → Header before opening brace

## Scope Hierarchy Construction

### **Flat Vector Architecture**
```cpp
struct Scope {
    Instruction _header;                                 // Scope signature
    uint32_t _parentScopeIndex;                         // Parent reference
    std::variant<Instruction, uint32_t> _instructions;   // Content or nested scope index
    std::vector<std::shared_ptr<Context>> _contexts;     // Cache data
};
```

### **Hierarchical Organization**
- **Index-based references**: Parent-child relationships via array indices
- **GPU-friendly**: Flat vector enables parallel processing
- **Memory efficient**: No pointer chasing, cache-friendly access patterns

### **Scope Type Detection**
Based on cached signature patterns before `{`:
- **`class ClassName`** → Class scope
- **`function funcName(...)`** → Function scope  
- **`if (...)`** → Conditional scope
- **`while (...)`** → Loop scope
- **`{` alone** → Naked scope block

## ERawToken Processing Optimization

Layer 2 leverages ERawToken classification for efficient processing:

### **Structural Tokens** (Primary Focus)
- **LEFT_BRACE**: Enter new scope, clear cache for header instruction
- **RIGHT_BRACE**: Exit scope, process any remaining cached tokens
- **SEMICOLON**: Create instruction from cache, continue in current scope

### **Data-Carrying Tokens** (Process Normally)
- **IDENTIFIER**: Add to cache, may need string table lookup for scope naming
- **LITERAL**: Add to cache with type information

### **Non-Structural Tokens** (Fast Path)
- **KEYWORD/OPERATOR/PUNCTUATION**: Add to cache without special processing
- **WHITESPACE/COMMENT**: Add to cache for formatting preservation
- **NEWLINE**: Add to cache, may influence instruction boundaries

## Context System Integration

### **Structural Context Data**
Layer 2 creates structural analysis contexts:

```cpp
class StructuralContext : public Context {
public:
    ScopeType detected_scope_type;
    std::vector<uint32_t> signature_token_indices;
    uint32_t nesting_depth;
    // Other structural metadata
};
```

### **Cache Attachment Strategy**
- **Instruction contexts**: Token pattern analysis, expression complexity
- **Scope contexts**: Nesting information, scope type confidence, signature analysis

## Error Detection and Reporting

### **Structural Validation**
- **Brace matching**: Track scope entry/exit balance
- **Unexpected tokens**: Detect malformed constructs
- **Scope depth**: Monitor nesting limits and patterns

### **Error Correlation**
- **Source positions**: Use Token references to correlate back to RawToken positions
- **Context preservation**: Maintain error context through Token→RawToken mapping
- **Graceful degradation**: Continue processing with best-effort scope reconstruction

## Performance Characteristics

### **Processing Efficiency**
- **Single-pass processing**: Stream through tokens once per input stream
- **Cache-friendly access**: Sequential token processing with minimal random access
- **Parallel opportunity**: Independent scope processing after initial construction

### **Memory Optimization**
- **Reference-based**: Tokens are lightweight references, not copies
- **Flat hierarchy**: Vector-based scope storage for cache efficiency
- **Context sharing**: shared_ptr allows context data sharing between structures

## Layer 3 Interface Preparation

Layer 2 provides organized structures for Layer 3 semantic analysis:

### **Structured Token Sequences**
- **Grouped instructions**: Related tokens organized into processing units
- **Hierarchical scopes**: Clear boundary identification for semantic analysis
- **Context attachment points**: Extensible cache system for semantic data

### **Semantic Analysis Ready**
- **Token accessibility**: Easy access to both grouped and individual tokens
- **Original preservation**: All RawToken data accessible through references
- **Scope relationships**: Clear parent-child structure for context-sensitive analysis

## Key Design Principles

### **Pure Structural Focus**
- **No semantic interpretation**: Defer keyword meaning to Layer 3
- **Boundary-driven organization**: Structure based on syntactic boundaries only
- **Pattern-agnostic grouping**: Group tokens without understanding their meaning

### **Lossless Organization**
- **Reference preservation**: All original token data accessible
- **Reconstruction capability**: Can rebuild original structure from organized data  
- **Formatting maintenance**: Whitespace and comments preserved in token sequences

### **Extensible Architecture**
- **Context system**: Layers can attach cache data without modifying core structures
- **Flexible instruction content**: Instructions can hold either tokens or scope references
- **Scalable hierarchy**: Flat vector supports arbitrary nesting depths efficiently