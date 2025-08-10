# Layer 1 - Raw Tokenization

## Responsibilities

- **Pure lexical analysis**: Convert source text streams into RawToken structures
- **Dual token classification**: Generate both ERawToken (structural) and EToken (detailed) classifications  
- **String table population**: Intern identifiers and string literals with controlled mutable access
- **Source position tracking**: Maintain accurate line/column/position data for error correlation
- **Context-insensitive recognition**: Basic keyword identification without semantic interpretation

## Input/Output

- **Input**: `std::map<std::string, std::stringstream>` - Named input streams from Layer 0
- **Output**: `std::map<std::string, std::vector<RawToken>>` - Raw token streams per input
- **Side Effects**: Populates StringTable with interned strings (only layer with mutable access)
- **Dependencies**: StringTable for string interning, TokenizerState for source correlation

## Core Data Structure - RawToken

```cpp
struct RawToken {
    ERawToken _raw_token = ERawToken::INVALID;     // Structural category
    EToken _token = EToken::INVALID;               // Detailed token type
    
    uint32_t _line = UINT32_MAX;                   // Source line
    uint32_t _column = UINT32_MAX;                 // Source column  
    uint32_t _position = UINT32_MAX;               // Byte position
    
    std::variant<
        std::monostate,                            // No value
        int32_t,                                   // INT_LITERAL
        uint32_t,                                  // UINT_LITERAL  
        int64_t,                                   // LONG_LITERAL
        uint64_t,                                  // ULONG_LITERAL
        float,                                     // FLOAT_LITERAL
        double,                                    // DOUBLE_LITERAL
        bool,                                      // BOOL_LITERAL
        StringIndex                                // IDENTIFIER, STRING_LITERAL, COMMENT, etc.
    > _literal_value;
};
```

## Token Classification System

### **ERawToken Categories** - Layer 2 Optimization
Structural categories that Layer 2 uses for efficient processing:

- **LEFT_BRACE** (`{`) - Opens scopes, triggers scope entry
- **RIGHT_BRACE** (`}`) - Closes scopes, triggers scope exit  
- **SEMICOLON** (`;`) - Terminates statements, triggers instruction boundary
- **IDENTIFIER** - Variable/function names requiring string table lookup
- **LITERAL** - Values requiring variant storage and type analysis
- **KEYWORD** - Language keywords, operators, punctuation (Layer 2 mostly ignores)
- **WHITESPACE/COMMENT/NEWLINE** - Non-structural tokens for formatting preservation
- **EOF_TOKEN** - Stream termination marker

### **EToken Values** - Complete Taxonomy
Detailed token classification for precise analysis:
- **Literals**: INT_LITERAL, UINT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, BOOL_LITERAL, etc.
- **Keywords**: CLASS, STRUCT, FUNCTION, RUNTIME, DEFER, AUTO, VAR, CONST, IF, WHILE, etc.
- **Operators**: PLUS, MINUS, MULTIPLY, ASSIGN, EQUALS, LESS_THAN, LOGICAL_AND, etc.
- **Punctuation**: LEFT_PAREN, RIGHT_PAREN, COMMA, DOT, COLON, ARROW, etc.
- **Special**: IDENTIFIER, WHITESPACE, COMMENT, NEWLINE, EOF_TOKEN

## String Table Integration

### **Controlled Mutable Access**
Layer 1 is the **only layer** with mutable StringTable access:

```cpp
// Layer 1 can intern strings
StringIndex index = string_table.intern("variable_name");
raw_token._literal_value = index;

// Layer 2+ can only read
const std::string& name = string_table.get_string(index);
```

### **Interning Strategy**
- **Identifiers**: All variable/function/type names
- **String literals**: Complete string content
- **Comments**: Full comment text for documentation tools
- **No substring optimization**: Simple complete-string storage for reliability

## Tokenization Process

### **Stream Processing**
1. **Initialize per stream**: Set up TokenizerState for each input stream
2. **Character-by-character analysis**: Advance through source maintaining position
3. **Token recognition**: Identify token boundaries and classify content
4. **Dual classification**: Set both ERawToken and EToken values
5. **String interning**: Populate StringTable for text-based tokens
6. **Position recording**: Store line/column/position for error correlation

### **Context-Insensitive Rules**
- **Keywords**: Based on exact string matching only
- **No semantic analysis**: `runtime` always classified as RUNTIME keyword
- **No context awareness**: Cannot distinguish `defer` statement vs `defer` modifier
- **Literal parsing**: Type-based classification (int vs float vs string)

## Error Handling Integration

### **Source Position Correlation**
Every RawToken includes precise source location enabling:
- **Error reporting**: Exact line/column identification
- **IDE integration**: Jump-to-definition and error highlighting
- **Debug information**: Source mapping for compiled code

### **TokenizerState Integration**
Layer 1 maintains TokenizerState for error correlation:
- **Position tracking**: Current line/column/byte offset per stream
- **Token mapping**: Index-based correlation between tokens and source positions
- **Context preservation**: Source content and line boundary information

## Performance Characteristics

### **Memory Efficiency**
- **String deduplication**: StringTable eliminates duplicate storage
- **Index references**: 32-bit StringIndex instead of string copies
- **Variant storage**: Type-safe literal values without memory overhead

### **Processing Speed**
- **Single-pass tokenization**: No backtracking or multi-pass analysis
- **Stream-based processing**: Handles large files without full memory loading
- **Parallel opportunity**: Multiple input streams can be tokenized concurrently

## Layer 2 Interface

Layer 1 provides optimized data for Layer 2 structural processing:

- **ERawToken classification**: Enables fast structural pattern recognition
- **Token stream organization**: Named streams map to source files/modules
- **Index-based references**: Efficient token access for grouping operations
- **Position preservation**: Source correlation maintained through all layers

## Key Design Principles

### **Lossless Tokenization**
- **Complete information preservation**: All source content tokenized
- **Reconstruction capability**: Token stream + StringTable can rebuild original source
- **Formatting preservation**: Whitespace and comments retained

### **Layer Separation**
- **No semantic interpretation**: Defers context-sensitive analysis to Layer 2+
- **Pure lexical analysis**: Focus on character-to-token conversion only
- **Clean interfaces**: Well-defined input/output contracts

### **Error Resilience**
- **Graceful degradation**: Invalid characters become UNKNOWN tokens
- **Position maintenance**: Error tokens include accurate source locations
- **Stream independence**: Errors in one stream don't affect others