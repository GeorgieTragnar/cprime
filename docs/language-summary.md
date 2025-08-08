# CPrime Language Summary

## Core Architecture

### Three-Class System

**Data Classes**: RAII state holders
- Pure state with memory operations only
- Constructor access control via `constructed_by`
- Foundation for all other systems
- **`semconst` fields**: Enforce 1:1 move-in/move-out policy within local scope for atomic value replacement

**Functional Classes**: RAII state modifiers
- Stateless operations that modify data class state
- Gain access rights to data classes through interface contracts at instantiation
- Enable inheritance-like polymorphism with explicit memory costs
- Can be templated through interface contracts

**Danger Classes**: C++ equivalent for interop
- Full C++ semantics (stateful and functional)
- Used for C++ compatibility and legacy integration

### Supporting Constructs

**Interfaces**: Memory contracts or accessor constructs for polymorphic glue between classes
- **Compile-time interfaces**: Memory contracts with zero overhead
- **Runtime interfaces**: Accessor-based with overhead
- Single declaration principle: Either compile-time OR runtime, never both
- Enable N:M composition between data and functional classes

**Unions**: Memory space constructs with polymorphic tagging
- **Compile-time unions**: Fixed memory size, locked memory contracts
- **Runtime unions**: Growable size with resize overhead, runtime elevators for objects
- Use polymorphic tagging for type identification
- Enable heterogeneous containers and pattern matching

## Revolutionary Systems

### Coroutines
- Heap-allocated stacks that never move
- Stack-allocated micro pools with 100x memory density
- 50-100 cycle context switches
- Compiler-analyzed pool allocation

### Channels
- Three-layer architecture: Language (semantics), Library (storage), Scheduler (lifecycle)
- Competitive consumption: Each message consumed by exactly ONE receiver
- Functional classes as automatic subscription mechanism through access rights
- Memory safety through reference counting

### Interface Memory Contracts
- Enable N:M composition: Multiple data classes work with multiple functional classes
- Compile-time interfaces: Zero-cost direct memory access
- Runtime interfaces: Flexible accessor-based access
- Data class controlled layout with explicit field linking (`<-` operator)

### Signal Handling and Runtime Selection
- **Unified signal model**: OS signals and exceptions use same syntax (`catch`/`recover`)
- **Runtime-agnostic primitives**: Signal handling works across all execution models
- **Flexible runtime selection**: `fn main() = RuntimeChoice` determines execution behavior
- **Progressive safety levels**: Configurable from C-level permissiveness to Rust-level guarantees
- **Signal-to-coroutine conversion**: Scheduler runtime transforms signals into async operations

## Key Principles

### Zero-Cost Abstractions
- Compile-time interface contracts provide direct memory access
- Functional classes enable aggressive compiler optimizations
- Pay only for what you use

### Explicit Control
- Performance costs are visible (runtime interfaces, accessor methods)
- Memory layout controlled by data classes
- Developer chooses between speed and flexibility
- **Library Control**: Object owners control combinatorial possibilities through explicit extension modes

### Memory Safety
- RAII enforced at language level
- Construction control prevents invalid states
- Reference counting prevents use-after-free
- No hidden complexity or surprising behaviors

### Composability
- Interface contracts enable generic programming
- Three-class system maintains architectural clarity
- N:M composition without inheritance complexity
- Uniform patterns across all systems

## Polymorphism Tiers

1. **Functional Classes**: Inheritance-like with explicit memory costs (via access rights mechanism)
2. **Traditional Interfaces**: Shared operations across types via constructs
3. **Interface Memory Contracts**: N:M composition enablement via constructs
4. **Union Pattern Matching**: Heterogeneous collections via runtime elevators

Each tier serves specific use cases while maintaining type safety and performance characteristics. Interfaces and unions are constructs that enable polymorphism, not classes themselves.

## Design Philosophy

### Cost-Signaling Through Keywords

CPrime's fundamental principle: **performance costs must be explicit**. The language uses `runtime` and `comptime` keywords to signal performance characteristics:

- **`runtime`** = explicit performance cost or overhead
- **`comptime`** = zero runtime cost, compile-time evaluation

This cost-signaling extends throughout the language:
- `runtime static` - explicit startup cost
- `comptime static` - zero runtime cost  
- `runtime interface` - accessor method overhead
- `runtime union` - polymorphic tagging overhead

### Language Synthesis

CPrime combines:
- **C++'s control** without complexity
- **Rust's safety** without fighting the borrow checker  
- **Go's simplicity** without sacrificing performance
- **Revolutionary innovations** in coroutines and composition
- **Explicit cost model** through keyword-based signaling

The result is a systems programming language that makes the right thing easy and the wrong thing impossible, while providing unprecedented performance in coroutine-based concurrent systems with flexible signal handling that adapts to different execution models.

## Additional Documentation

### Foundational Concepts
- **[Runtime/Comptime Keywords](runtime-comptime-keywords.md)**: Complete reference for cost-signaling keywords
- **[Statics and Constexpr](statics-and-constexpr.md)**: Static variables, constexpr functions, and pluggable interop

### Core Systems
- **[Signal Handling](signal-handling.md)**: Language primitives, syntax, and safety levels
- **[Execution Runtime System](runtime-system.md)**: Execution models and runtime environment behavior
- **[Coroutines](coroutines.md)**: Revolutionary coroutine architecture with signal integration
- **[Channels](channels.md)**: Three-layer channel architecture for concurrent communication
- **[Interfaces](interfaces.md)**: Polymorphic glue and N:M composition patterns