# CPrime Language Summary

## Core Architecture

### Four-Class System

**Data Classes**: RAII state holders
- Pure state with memory operations only
- Constructor access control via `constructed_by`
- Foundation for all other systems
- **`semconst` fields**: Enforce 1:1 move-in/move-out policy within local scope for atomic value replacement

**Access Rights (Functional Classes)**: RAII state modifiers
- Stateless operations that modify data class state
- Enable inheritance-like polymorphism with explicit memory costs
- Can be templated through interface contracts

**Interfaces**: Constructs for exact memory contracts and function signature contracts acting as polymorphic glue between state holders and state modifiers, allowing abstractable work on different types using the same function calls
- Traditional interfaces: Common vtable contracts
- Memory contracts: Enable N:M composition
- Single declaration principle: Either compile-time OR runtime, never both

**Unions**: Allow templates to work with classes even though their memory layouts as a whole don't match, and furthermore allow through polymorphic tags or compile-time resolution to work with an object through switch case statements
- Memory contracts reserving space for largest variant
- Objects live inside union space, not "the union" itself
- Enable heterogeneous containers and generic programming

## Revolutionary Systems

### Coroutines
- Heap-allocated stacks that never move
- Stack-allocated micro pools with 100x memory density
- 50-100 cycle context switches
- Compiler-analyzed pool allocation

### Channels
- Three-layer architecture: Language (semantics), Library (storage), Scheduler (lifecycle)
- Competitive consumption: Each message consumed by exactly ONE receiver
- Access rights as automatic subscription mechanism
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

1. **Access Rights**: Inheritance-like with explicit memory costs
2. **Traditional Interfaces**: Shared operations across types
3. **Interface Memory Contracts**: N:M composition enablement
4. **Unions**: Heterogeneous collections and pattern matching

Each tier serves specific use cases while maintaining type safety and performance characteristics.

## Design Philosophy

CPrime combines:
- **C++'s control** without complexity
- **Rust's safety** without fighting the borrow checker  
- **Go's simplicity** without sacrificing performance
- **Revolutionary innovations** in coroutines and composition

The result is a systems programming language that makes the right thing easy and the wrong thing impossible, while providing unprecedented performance in coroutine-based concurrent systems with flexible signal handling that adapts to different execution models.

## Additional Documentation

- **[Signal Handling](signal-handling.md)**: Language primitives, syntax, and safety levels
- **[Runtime System](runtime-system.md)**: Execution models and runtime-specific behavior
- **[Coroutines](coroutines.md)**: Revolutionary coroutine architecture with signal integration
- **[Channels](channels.md)**: Three-layer channel architecture for concurrent communication
- **[Interfaces](interfaces.md)**: Polymorphic glue and N:M composition patterns