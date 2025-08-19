# CPrime Compiler Implementation Overview

This document provides a comprehensive review of the CPrime compiler architecture, detailing the conceptual steps from source code loading through each implemented layer and sublayer, including their specific concerns and responsibilities.

## Architectural Foundation

### **N-Layer Architecture Design**
- **Expandable**: Designed for future layers (currently implementing Layers 0-2)
- **Validation Pipeline**: Each layer has a validation counterpart for comprehensive testing
- **Component-based**: Shared commons, logging with spdlog, comprehensive error handling
- **Stateless Processing**: Each layer transforms input to output without maintaining state

### **Core Design Principles**
1. **Sequential Processing**: Each layer completely transforms its input before passing to next layer
2. **Stateless Design**: Layers don't maintain internal state between compilations
3. **Error Resilience**: Comprehensive error handling at each layer with graceful degradation  
4. **Memory Efficiency**: String interning, lightweight token references, component buffering
5. **Debug Transparency**: Every layer provides detailed logging and analysis tools
6. **Validation-First**: Each layer designed with comprehensive testing from the start

---

## **LAYER 0: INPUT PROCESSING**
**Core Responsibility**: File System Interface → String Streams

### **Primary Concerns:**
- **File System Access**: Read `.cp` and `.cprime` files from disk
- **Stream Generation**: Convert file contents to named stringstreams  
- **Error Resilience**: Handle file read errors, missing files, permissions
- **Stream ID Generation**: Create unique identifiers based on file paths

### **Key Operations:**
1. **File Validation**: Check existence, readability, supported extensions (`.cp`, `.cprime`)
2. **Content Reading**: Read entire file into memory as stringstream
3. **Stream Mapping**: Create `map<string, stringstream>` with unique IDs
4. **Error Reporting**: Basic file-level error handling

### **Implementation Details:**
- **Static Methods**: Stateless processing through `InputProcessor` class
- **Extension Support**: `.cp` and `.cprime` file extensions
- **Stream ID Generation**: Based on filename without directory path
- **Memory Management**: Direct file-to-stringstream conversion

### **Outputs**: `map<string, stringstream>` → Layer 1

---

## **LAYER 1: RAW TOKENIZATION** 
**Core Responsibility**: String Streams → Raw Token Vectors

### **Architecture**: 5-sublayer pipeline with sequential token refinement

### **Sublayer 1A: Character-Level Token Extraction + Exec Alias Detection**
**Responsibility**: Initial character processing and exec alias identification

**Key Features:**
- **Single-character tokens**: `{`, `}`, `;`, `(`, `)`, `[`, `]`, etc.
- **State machine processing**: Handle whitespace, comments, basic structural tokens
- **Exec alias detection**: Early identification of potential exec alias patterns
- **ProcessingChunk creation**: Intermediate representation for further refinement

**Implementation:**
- Character-by-character stream processing
- Exec alias registry integration for early detection
- Comment handling (line and block comments)
- Whitespace token creation for formatting preservation

### **Sublayer 1B: String and Character Literals (Prefix-Aware)**  
**Responsibility**: Extract and process quoted string and character literals

**Key Features:**
- **String literal extraction**: Handle quoted strings with escape sequences
- **Character literal processing**: Single-character constants with escaping
- **Prefix awareness**: Support for string prefixes (raw strings, unicode, etc.)
- **String table integration**: Intern strings for memory efficiency and deduplication

**Implementation:**
- Escape sequence processing (`\n`, `\"`, `\\`, etc.)
- Multi-character escape sequences (unicode, octal, hex)
- String table population for efficient storage
- Proper quote matching and error handling

### **Sublayer 1C: Operator Extraction**
**Responsibility**: Identify and extract multi-character operators

**Key Features:**
- **Multi-character operators**: `->`, `::`, `+=`, `==`, `!=`, `<=`, `>=`, etc.  
- **Never-identifier operators**: Tokens that can never be part of identifiers
- **Precedence preparation**: Organize operators for later precedence analysis
- **Compound assignment**: `+=`, `-=`, `*=`, `/=`, `%=`, etc.

**Implementation:**
- Longest-match operator recognition
- Operator precedence classification preparation  
- Context-free operator extraction
- Operator token type assignment

### **Sublayer 1D: Number Literals (Suffix-Aware)**
**Responsibility**: Parse and validate numeric literals

**Key Features:**
- **Integer literals**: Binary (`0b`), octal (`0o`), hexadecimal (`0x`), decimal
- **Floating-point literals**: Scientific notation, decimal points, exponential form
- **Suffix processing**: Type suffixes (`u`, `l`, `f`, `ul`, `ll`, etc.)
- **Range validation**: Check for overflow/underflow conditions

**Implementation:**
- Multi-base number parsing with proper validation
- Floating-point format recognition (IEEE standards)
- Suffix parsing and type hint generation
- Error detection for malformed numbers

### **Sublayer 1E: Keywords + Identifiers + Exec Alias Recognition**
**Responsibility**: Final token classification and exec alias completion

**Key Features:**
- **Keyword detection**: Convert CHUNK tokens to specific language keywords  
- **Identifier creation**: Remaining alphanumeric sequences become identifiers
- **Exec alias finalization**: Complete exec alias token generation and registration
- **RawToken generation**: Final token format with complete position tracking

**Implementation:**
- Keyword lookup table for language-specific keywords
- Context-free identifier recognition
- Exec alias registry completion
- Position and source correlation maintenance

### **Outputs**: `map<string, vector<RawToken>>` → Layer 2

---

## **LAYER 2: STRUCTURE BUILDING & CONTEXTUALIZATION**
**Core Responsibility**: Raw Tokens → Structured Scopes with Contextual Analysis

### **Architecture**: 4-sublayer pipeline with increasing semantic awareness

### **Sublayer 2A: Pure Structural Scope Building**
**Responsibility**: Convert tokens to structured scopes with clear boundaries

**Key Features:**
- **Token lightweight conversion**: RawToken → Token references for efficiency
- **Cache-and-semicolon parsing**: Mandatory semicolons for unambiguous structure
- **Flat scope vector**: Parent/child indexing instead of nested structures for performance
- **Exec scope registration**: Register exec blocks in ExecAliasRegistry for later processing
- **Header/Footer/Body separation**: Distinguish instruction types for semantic analysis

**Key Data Structures:**
- **TokenCache**: Accumulate tokens until structural boundaries (semicolons, braces)
- **ScopeBuilder**: State machine for progressive scope construction
- **Instruction types**: Header (scope declarations), Footer (scope endings), Body (executable content), Nested (scope references)

**Implementation Details:**
- Semicolon-driven parsing for unambiguous structure detection
- Parent-child scope indexing for efficient traversal
- Exec scope identification and preliminary registration
- Instruction type classification for later processing

### **Sublayer 2B: Exec Logic Compilation and Resolution**
**Responsibility**: Compile and link exec blocks with executable logic

**Key Features:**
- **Lua script compilation**: Convert exec blocks to ExecutableLambda format
- **Exec-scope linking**: Connect scope indices to their executable logic implementations
- **ExecAliasRegistry population**: Complete exec alias definitions with compiled scripts
- **Two-pass processing**: Parent scopes first, then specialization scopes
- **Template parameter resolution**: Handle parameterized exec blocks with type substitution

**Implementation Details:**
- Lua runtime integration for exec script compilation
- Exec alias to scope index mapping
- Template parameter substitution and specialization handling
- Dependency resolution between exec blocks
- Error handling for invalid exec scripts

### **Sublayer 2C: Namespace-Aware CHUNK Token Disambiguation**  
**Responsibility**: Resolve ambiguous CHUNK tokens using namespace context

**Key Features:**
- **Hierarchical traversal**: Inherit and maintain namespace context from parent scopes
- **CHUNK → Token resolution**: Convert ambiguous CHUNK tokens to KEYWORD/IDENTIFIER/EXEC_ALIAS
- **Context-sensitive keywords**: Handle keywords like `runtime`, `defer`, `exposes` with context-dependent meanings
- **In-place replacement**: Preserve source correlation while transforming tokens
- **Namespace context tracking**: Maintain scope-aware symbol resolution

**Implementation Details:**
- Namespace context inheritance through scope hierarchy
- Symbol table lookup for context-sensitive resolution
- CHUNK token replacement with proper type classification
- Source position preservation during transformation
- Namespace-qualified identifier resolution

### **Sublayer 2D: Instruction Contextualization & Sequential Analysis**
**Responsibility**: Apply semantic analysis and pattern recognition to instructions

**Key Features:**
- **Sequential processing**: Flat iteration through all scope instructions for performance
- **N:M Contextualization System**: Advanced pattern recognition producing variable output
- **Exec execution processing**: Single-pass exec code generation and integration
- **Comprehensive error handling**: Detailed contextualization error reporting with source locations
- **Pattern-based analysis**: Sophisticated token pattern matching for semantic understanding

#### **N:M Contextualization System (Advanced Pattern Recognition)**

**Architecture**: Template-based `BaseContextualizer<PatternElementType>` infrastructure

**Three Specialized Contextualizers:**

##### **Header Contextualizer**
**Patterns**: Class declarations, function signatures, inheritance, template definitions
- **Class patterns**: `class Name { ... }`, `struct Name { ... }`, `interface Name { ... }`
- **Function signatures**: Complex parameter lists, return type arrows, template parameters
- **Inheritance patterns**: `: BaseClass`, implements lists, access modifiers
- **N:M Examples**: 41 input tokens → 15 contextual tokens (complex class headers)

##### **Instruction Contextualizer** 
**Patterns**: Variable declarations, assignments, function calls, control flow
- **Declaration patterns**: `int x;`, `auto y = value;`, `const z = literal;`
- **Assignment patterns**: `=`, compound assignments (`+=`, `-=`, etc.)
- **Function call patterns**: Method calls, constructor calls, static calls
- **N:M Examples**: 9 input tokens → 4 contextual tokens (function call statements)

##### **Footer Contextualizer**
**Patterns**: Return statements, cleanup code, scope finalization
- **Return patterns**: `return;`, `return expression;`
- **Control flow**: `break;`, `continue;`, exception handling
- **Cleanup patterns**: `defer` statements, resource management
- **N:M Examples**: 5 input tokens → 5 contextual tokens (whitespace-aware footers)

**Advanced Pattern Matching Features:**
- **Enum-based Token Matching**: Uses `EToken` enums directly, no string comparisons
- **Complex Pattern Elements**: Function parameters, type bodies, template parameters, namespace paths
- **Priority-based Matching**: Higher priority patterns attempted first for disambiguation
- **Whitespace-aware Processing**: Proper handling of formatting and spacing tokens
- **N:M Token Mapping**: Variable input tokens produce variable contextual outputs based on semantic patterns

**Implementation Details:**
- Template-based pattern matching with type safety
- Pattern priority queues for efficient matching
- Context-sensitive token classification
- Whitespace token handling and preservation
- Error recovery for unrecognized patterns

### **Outputs**: `vector<Scope>` with fully contextualized tokens → Future Layers

---

## **Cross-Cutting Concerns**

### **Error Handling System**
- **ErrorHandler**: Centralized error reporting with precise source location resolution
- **ErrorReporter lambdas**: Layer-specific error reporting callbacks for context-aware errors
- **ContextualizationError types**: Specific error categories for pattern matching failures
- **Source location tracking**: Maintain file/line/column information throughout entire pipeline

### **Shared Infrastructure**
- **StringTable**: Centralized string interning for memory efficiency and fast comparisons
- **ExecAliasRegistry**: Global registry for exec block management and resolution
- **Component-based logging**: spdlog integration with selective buffering per compiler component
- **Token/RawToken system**: Lightweight token references vs full token data for memory efficiency

### **Validation Pipeline**
- **Layer-specific validation**: Each layer has dedicated testing counterpart (layer0validation, layer1validation, etc.)
- **Google Test integration**: Comprehensive test framework with automated test discovery
- **Debug tooling**: CLI tools for layer-by-layer analysis and debugging (`cprime_cli`)
- **Pattern testing**: Automated validation of N:M contextualization with test case generation

### **Development Tools**
- **Build Scripts**: Standardized build, test, analyze, and compile workflows
- **Analysis Scripts**: Layer-by-layer debugging and inspection tools
- **Example Testing**: Comprehensive example files for testing all language features
- **Logging Infrastructure**: Component-specific, level-controlled logging throughout pipeline

---

## **Current Implementation Status**

### **Fully Implemented**
- ✅ **Layer 0**: Complete input processing with file validation and stream generation
- ✅ **Layer 1**: Full 5-sublayer tokenization pipeline with exec alias support  
- ✅ **Layer 2**: Complete 4-sublayer structure building with advanced N:M contextualization
- ✅ **N:M Pattern Recognition**: Advanced contextualization system with three specialized contextualizers
- ✅ **Error Handling**: Comprehensive error reporting with source location tracking
- ✅ **Testing Infrastructure**: Google Test integration with layer-specific validation
- ✅ **Development Tools**: Build scripts, analysis tools, debugging infrastructure

### **Key Technical Achievements**
- **Performance**: Flat scope structure with efficient indexing
- **Memory Efficiency**: String interning, lightweight token references
- **Pattern Recognition**: Sophisticated N:M contextualization with enum-based matching
- **Error Resilience**: Graceful degradation with detailed error reporting
- **Maintainability**: Template-based architecture with consistent interfaces
- **Extensibility**: N-layer design ready for future expansion

### **Architecture Benefits**
1. **Scalable**: Easy addition of new layers and pattern types
2. **Testable**: Comprehensive validation pipeline at every layer
3. **Debuggable**: Detailed logging and analysis tools at every step
4. **Efficient**: Optimized data structures and processing algorithms  
5. **Maintainable**: Clean separation of concerns and consistent interfaces
6. **Extensible**: Template-based design allows easy pattern extension

This implementation successfully transforms CPrime source code from raw text files into fully structured, contextualized representations with sophisticated semantic analysis, setting the foundation for future AST construction and code generation phases.