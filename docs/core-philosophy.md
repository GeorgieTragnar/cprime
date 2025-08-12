# CPrime Core Philosophy

## Overview

CPrime is a systems programming language designed to achieve everything that C++ tries to achieve, making close or identical decisions where C++ excels, while completely replacing C++-style inheritance with a differently structured polymorphism system.

## Primary Inspirations

### 1. C++ Memory Model
- **Manual memory management**: Programmer controls allocation and deallocation
- **References can dangle**: No borrow checker - programmer responsibility
- **RAII patterns**: Resource Acquisition Is Initialization for automatic cleanup
- **Performance-first**: Zero-cost abstractions and explicit performance costs

### 2. Go Concurrency
- **Goroutines**: Lightweight cooperative coroutines with M:N threading
- **Channels**: Primary communication mechanism between coroutines
- **Defer statement**: Ensures cleanup at scope exit
- **Simple concurrency model**: Easy to reason about concurrent programs

### 3. Capability Security
- **Access rights as unforgeable tokens**: Compile-time security boundaries
- **Module boundaries**: Fine-grained access control without runtime overhead
- **Principle of least privilege**: Grant minimum necessary permissions
- **Zero-cost security**: Security model compiled away at runtime

### 4. Zero-Cost Abstractions
- **Pay only for what you use**: No hidden allocations or indirection
- **Compile-time resolution**: Complex abstractions become simple machine code
- **Explicit performance costs**: Runtime features clearly marked as opt-in
- **Predictable performance**: No surprises in generated code

## Fundamental Design Principles

### 1. Three-Class System
**Separation of concerns at the language level:**
- **Data classes**: Pure state, no behavior
- **Functional classes**: Pure operations with memoize-only optimization storage
- **Danger classes**: Full C++ semantics for interop and legacy code

**Benefits:**
- Enforces good architectural patterns
- Makes code intentions explicit  
- Enables powerful compiler optimizations
- Provides clear boundaries for different programming paradigms
- Allows performance optimization without compromising architectural purity through memoize fields

### 2. No Inheritance
**Replace class hierarchies with composition and access rights:**
- **Composition over inheritance**: Build complex types from simpler ones
- **Access rights**: Control visibility and capabilities through compile-time grants
- **No virtual dispatch overhead**: Direct function calls by default
- **Clear dependencies**: No hidden method resolution

**Advantages:**
- Eliminates diamond problem and multiple inheritance complexity
- Makes code dependencies explicit and trackable
- Enables better compiler optimizations
- Simplifies mental model for large codebases

### 3. Module-Based Security  
**Compile-time access control with zero runtime cost:**
- **Friend modules**: Explicit grants of access to private data
- **Capability boundaries**: Modules define what operations are possible
- **Static verification**: Security properties checked at compile time
- **Zero runtime overhead**: Security model compiled away

### 4. Explicit Danger Zones
**Clear boundaries between safe and unsafe operations:**
- **Danger attributes**: Mark code that requires careful review
- **Isolated unsafety**: Contain dangerous operations to specific boundaries
- **Gradual safety**: Mix safe and dangerous code as needed
- **Migration path**: Easy transition from C++ danger classes to safe alternatives

### 5. Coroutine-First Concurrency
**M:N threading with Go-style simplicity:**
- **Cooperative scheduling**: Coroutines yield control voluntarily
- **Structured concurrency**: Clear lifetime management for concurrent tasks
- **Channel communication**: Message passing over shared memory
- **No data races**: Communication patterns prevent common concurrency bugs

## Design Philosophy in Practice

### Safety Without Complexity
- **Natural bug prevention**: Architecture prevents many common mistakes
- **No borrow checker needed**: Simple rules about stack containment and access
- **Clear mental model**: Programmers can reason about code behavior
- **Gradual learning curve**: C++ programmers feel immediately at home

### Performance First
- **Zero-cost abstractions by default**: High-level features compile to efficient code
- **Explicit runtime features**: Dynamic behavior clearly marked as opt-in
- **No hidden allocations**: Memory usage is predictable and explicit
- **Direct machine mapping**: Language constructs map clearly to hardware

### Familiarity and Migration
- **C++ mental model**: Familiar concepts like RAII, manual memory management
- **Go-style concurrency**: Proven patterns for concurrent programming  
- **Gradual adoption**: Can mix CPrime with existing C++ code
- **Clear upgrade path**: Danger classes provide compatibility bridge

### Flexibility and Expressiveness
- **Multiple paradigms**: Functional, imperative, and system programming styles
- **Runtime variants**: Opt-in dynamic features when static isn't sufficient
- **Extensible design**: Access rights system allows novel programming patterns
- **Future-proof**: Architecture supports evolution without breaking changes

## What CPrime Is NOT

### Not Rust
- **No borrow checker**: References can dangle like in C++
- **No lifetime annotations**: Programmer manages object lifetimes
- **Manual memory management**: malloc/free or RAII, not garbage collection
- **C++ compatibility**: Direct interop with existing C++ codebases

### Not Traditional OOP
- **No inheritance hierarchies**: Composition and access rights instead
- **No polymorphism by default**: Use access rights for behavior variation
- **No virtual methods**: Direct function calls unless explicitly opted into
- **No hidden complexity**: Method resolution is always explicit

### Not Pure Functional
- **Mutation allowed**: Variables can be modified in place
- **Imperative style**: Statements and side effects are natural
- **Performance-oriented**: Efficiency over mathematical purity
- **Practical pragmatism**: Best tool for systems programming, not theoretical elegance

## Core Value Proposition

CPrime provides **C++ performance and familiarity** while adding:
- **Capability-based security** through compile-time access control
- **Go-style concurrency** without manual thread management complexity
- **Clear separation** between safe and unsafe code boundaries
- **Natural prevention** of common bugs through architectural constraints

The result is a language that feels familiar to C++ programmers but guides them toward better architectural patterns while maintaining the performance characteristics essential for systems programming.