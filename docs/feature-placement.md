# CPrime Feature Placement: Core Language vs Standard Library

## Overview

This document provides the definitive categorization of CPrime language concepts into three clear divisions based on necessity, implementability, and architectural boundaries. This classification guides what must be built into the compiler core versus what can be implemented as library features.

## Classification Categories

1. **100% Core Language** - Features that absolutely must be compiler-implemented due to fundamental language semantics, ABI requirements, or runtime necessity
2. **Compiler Uncertain** - Features that should probably be in the compiler but the boundary is unclear or debatable
3. **Standard Library** - Features that can and should be library-implemented through comptime execution or other mechanisms

---

## 100% Core Language

*These features cannot be implemented as library code and must be built into the compiler itself.*

### Memory & Object Model

**RAII System**
- Scope-based automatic cleanup for expected behavior
- Deterministic construction/destruction ordering
- Integration with defer statements
- *Why Core*: Stack unwinding and cleanup must be compiler-understood for optimization and correctness

**Staged Construction**
- Dead/alive object states with zombie vtables
- Atomic transition from dead to alive
- Stage mask tracking and validation
- *Why Core*: Memory layout and vtable pointer management requires deep compiler integration

**Move/Copy Semantics**
- Value vs reference semantics
- Automatic move insertion and copy elision
- Resource transfer mechanics
- *Why Core*: ABI compatibility and optimization require compiler understanding

**Stack vs Heap Allocation**
- Allocation decision making
- Lifetime analysis and escape detection
- Memory layout computation
- *Why Core*: Fundamental to code generation and memory safety

### Type System Foundation

**Primitive Types**
- int, float, bool, char and their variants
- Size and alignment specifications
- Arithmetic and logical operations
- *Why Core*: ABI compatibility requires fixed primitive definitions

**VTable Infrastructure**
- VTable memory layout and dispatch mechanism
- Universal type tags and casting infrastructure
- Primitive type vtables for unified polymorphism
- *Why Core*: Core polymorphism mechanism affects memory layout and calling conventions

**Pointer and Reference Types**
- Address-of and dereference operations
- Null pointer handling and safety
- Reference binding rules
- *Why Core*: Memory access patterns must be compiler-understood for optimization

**Union Discriminants**
- Tagged union memory layout
- Discriminant storage and access
- Pattern matching integration
- *Why Core*: Memory layout and type safety require compiler validation

### Execution Model

**Control Flow Primitives**
- if/else conditionals and branching
- while/for loops and iteration
- break/continue/return statements
- Function calls and returns
- *Why Core*: Program counter manipulation and optimizer understanding

**Coroutines**
- Function-scope suspension without triggering RAII
- Stack frame preservation and restoration
- Execution context switching
- *Why Core*: Stack manipulation cannot be library-implemented

**Signal Handling**
- catch/throw/recover syntax and semantics
- Stack unwinding for true error handling
- OS signal to CPrime signal mapping
- *Why Core*: Exception-like mechanism requires runtime integration

**defer Statements**
- Scope-based cleanup registration
- RAII integration and ordering
- Resource management guarantees
- *Why Core*: Scope analysis and cleanup insertion must be compiler-implemented

### Concurrency Primitives

**Channel Communication**
- Inter-coroutine message passing
- Blocking and non-blocking operations
- Channel lifecycle management
- *Why Core*: Requires deep runtime and scheduler integration

**Task Creation (go/spawn)**
- Coroutine spawning and lifecycle
- Parent-child task relationships
- Structured concurrency enforcement
- *Why Core*: Task scheduling and memory management require compiler support

### Policy System

**Core Compiler Policies**
- const enforcement levels (errors vs warnings)
- Optimization assumption policies
- Language behavior governance
- *Why Core*: Compiler behavior itself depends on policy settings

**Policy Enforcement**
- Policy validation and application
- Debug symbol integration
- UB prevention through policy rules
- *Why Core*: Even the compiler must follow policy rulings

### Comptime System Core

**Comptime Block Execution**
- Compile-time code execution
- Phase-specific execution hooks
- Compilation pipeline integration
- *Why Core*: Foundation that enables everything else to be library-implemented

**Flat Container Scoping Access**
- Parent/child scope indexing (not AST access)
- Scope analysis and manipulation
- Variable binding and resolution
- *Why Core*: Optimization framework requires this specific scope representation

**Pattern Matching Engine**
- Syntax pattern registration and matching
- Code transformation infrastructure
- Keyword aliasing system
- *Why Core*: Language extension mechanism must be compiler-integrated

### Runtime System

**Entry Point Assignment**
- `fn main() = runtime_expression { body }` syntax
- Runtime selection mechanism
- Platform-specific initialization
- *Why Core*: Program entry point cannot be library-controlled

**Staged Initialization**
- Explicit startup phase control
- Runtime parameter passing
- Execution environment setup
- *Why Core*: Fundamental program startup cannot be library-implemented

---

## Compiler Uncertain

*Features that should probably be in the compiler but the boundary is unclear or debatable.*

### VTable Manipulation APIs

**User-Exposed VTable Operations**
- Runtime type creation and modification
- Dynamic vtable construction
- Type system extensions
- *Uncertainty*: Could be library-exposed but needs battle-testing for safety and performance

### Advanced Memory Analysis

**Lifetime Analysis**
- Static analysis for memory safety
- Borrow checking and ownership
- Escape analysis refinement
- *Uncertainty*: Could be implemented via comptime analysis but might need compiler integration for performance

### Optimization Framework

**Comptime-Based Optimizations**
- Dead code elimination via comptime
- Constant folding and propagation
- Loop optimizations through scope analysis
- *Uncertainty*: Revolutionary approach - optimizations as library features that migrate to compiler

### Performance Cost Analysis

**Runtime/Comptime Cost Tracking**
- Performance impact warnings
- Zero-cost abstraction verification
- Memory usage profiling
- *Uncertainty*: Could be library analysis but might need compiler integration for accuracy

### Three-Class System Enforcement

**Data/Functional/Danger Class Validation**
- Architectural constraint enforcement
- Cross-class interaction rules
- Capability composition validation
- *Uncertainty*: Could be comptime-based validation but might need compiler support for enforcement

### Polymorphism Implementation Details

**Dispatch Optimization Strategies**
- Thunk vs pre-computed dispatch choice
- Selective dual this pointer management
- Performance-optimized casting
- *Uncertainty*: Could be library-configurable but compiler might need awareness for optimization

---

## Standard Library

*Features that can and should be implemented through comptime execution or other library mechanisms.*

### Template and Generic System

**Template Definitions**
- Generic type and function templates
- Template specialization and constraints
- Concept definitions and validation
- *Why Library*: Comptime pattern matching can transform generic syntax to concrete implementations

**Interface/Trait System**
- Interface contracts and implementations
- Trait bounds and associated types
- Dynamic dispatch through interfaces
- *Why Library*: Can be implemented via comptime code generation and vtable manipulation

### Language Syntax Extensions

**Iteration Constructs**
- foreach loops and iterators
- Range-based iteration syntax
- Iterator trait definitions
- *Why Library*: Syntax sugar transformable via comptime pattern matching

**Property System**
- Automatic getter/setter generation
- Property access syntax
- Validation and transformation hooks
- *Why Library*: Comptime code generation can create accessor methods

**Pattern Matching**
- match expressions and destructuring
- Exhaustiveness checking
- Guard conditions and filters
- *Why Library*: Transformable to switch statements and conditional logic via comptime

**Async/Await Syntax**
- Asynchronous function syntax sugar
- await expression transformation
- Coroutine syntax simplification
- *Why Library*: Syntax transformation to underlying coroutine primitives

### Metaprogramming Framework

**Reflection System**
- Runtime type information
- Field and method introspection
- Dynamic invocation capabilities
- *Why Library*: Comptime code generation can create reflection data structures

**Serialization Framework**
- Automatic serialization/deserialization
- Format-specific implementations
- Schema validation and migration
- *Why Library*: Code generation via comptime analysis of type structure

**Test Framework**
- Test definition and organization
- Assertion macros and utilities
- Test discovery and execution
- *Why Library*: Comptime pattern matching can transform test syntax to runtime calls

### Advanced Language Features

**Domain-Specific Languages**
- SQL embedding and query generation
- Regular expression integration
- State machine definitions
- *Why Library*: Comptime pattern matching enables arbitrary syntax extensions

**Code Generation Utilities**
- Builder patterns and fluent APIs
- Macro-like transformations
- Boilerplate elimination
- *Why Library*: Comptime quote/unquote system enables structured code generation

### Error Handling Extensions

**Result and Optional Types**
- Monadic error handling patterns
- Chaining and transformation operators
- Integration with signal handling
- *Why Library*: Can be implemented as regular types with comptime-generated convenience methods

**Error Context and Propagation**
- Error annotation and context
- Automatic error propagation syntax
- Error handling policy enforcement
- *Why Library*: Syntax transformations can wrap operations with error handling

### Performance and Optimization

**Optimization Passes**
- Advanced optimization techniques
- Domain-specific optimizations
- Performance profiling integration
- *Why Library*: Comptime access to flat scoping enables optimization implementation as library code

**Memory Pool Management**
- Custom allocators and pools
- Memory usage optimization
- RAII-integrated resource management
- *Why Library*: Can be implemented using core allocation primitives

### Interoperability

**C++ Integration**
- Header file processing
- ABI compatibility layers
- Template instantiation mapping
- *Why Library*: Can use comptime to generate appropriate interface code

**FFI and External Libraries**
- Foreign function interfaces
- Library binding generation
- ABI translation layers
- *Why Library*: Comptime code generation can create binding code

---

## Key Boundaries and Principles

### The Comptime Boundary
- **Core provides**: Comptime execution infrastructure with flat scoping access
- **Library implements**: All syntax transformations, code generation, and language extensions
- **Migration path**: Battle-tested library optimizations can move to compiler for performance

### The VTable Boundary  
- **Core provides**: VTable infrastructure, memory layout, dispatch mechanism
- **Library exposes**: High-level APIs for vtable manipulation (after battle-testing)
- **Safety principle**: User-exposed vtable operations must prove safety before compiler integration

### The Policy Boundary
- **Core enforces**: Fundamental policies that affect compiler behavior itself
- **Library defines**: Domain-specific policies and validation rules
- **Consistency principle**: Policy system is foundational - even compiler follows policy rulings

### The Performance Boundary
- **Core guarantees**: Zero-cost abstractions for fundamental language features
- **Library optimizes**: Advanced optimizations through comptime with migration path to compiler
- **Evolution principle**: Successful library optimizations become compiler features for performance

### The RAII Boundary
- **Core handles**: Expected behavior and resource cleanup
- **Signal system handles**: True error handling and unexpected situations
- **Separation principle**: RAII is for expected flow, signals for exceptional flow

This categorization ensures CPrime maintains a minimal, focused compiler core while enabling unlimited extensibility through its revolutionary comptime system and clear migration paths for proven library features.