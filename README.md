# CPrime Programming Language

CPrime is a systems programming language designed to achieve everything that C++ tries to achieve, making close or identical decisions where C++ excels, while completely replacing C++-style inheritance with a differently structured polymorphism system based on access rights and capability security.

## Key Features

- **Three-Class System**: Separation of state (Data), operations (Functional), and unsafe code (Danger)
- **Access Rights**: Compile-time capability security instead of inheritance-based polymorphism
- **Go-Style Concurrency**: Coroutines, channels, and structured concurrency
- **C++ Memory Model**: Manual memory management with RAII, references can dangle
- **Zero-Cost Abstractions**: Pay only for what you use, explicit performance costs
- **C++ Interop**: Direct compatibility and gradual migration path

## Quick Example

```cpp
// Data class - pure state
class FileData {
    handle: FileHandle,
    path: String,
    constructed_by: FileOps,
}

// Functional class - pure operations
functional class FileOps {
    fn construct(path: &str) -> Result<FileData> {
        let handle = os::open(path)?;
        Ok(FileData { handle, path: path.to_string() })
    }
    
    fn read_line(file: &mut FileData) -> Result<String> {
        os::read_line(file.handle)
    }
    
    fn destruct(file: &mut FileData) {
        os::close(file.handle);
    }
}

// Usage with RAII
let file = FileOps::construct("data.txt")?;
defer FileOps::destruct(&mut file);

let content = FileOps::read_line(&mut file)?;
println("Read: {}", content);
```

## Core Concepts

### Three-Class System

CPrime enforces architectural patterns through three distinct class types:

1. **Data Classes**: Contain only state and memory operations
2. **Functional Classes**: Contain only stateless operations  
3. **Danger Classes**: Allow traditional OOP for C++ interop and unsafe operations

### Access Rights Instead of Inheritance

Instead of inheritance hierarchies, CPrime uses compile-time access rights for polymorphism:

```cpp
class SecureData {
    key: [u8; 32],
    value: String,
    
    // Different functional classes see different fields
    exposes UserOps { value }
    exposes AdminOps { key, value }
}

functional class UserOps {
    fn read_value(data: &SecureData) -> &str {
        &data.value  // Can access value
        // data.key  // ❌ Cannot access key
    }
}

functional class AdminOps {
    fn read_key(data: &SecureData) -> &[u8] {
        &data.key   // Can access both key and value
    }
}
```

### Coroutines and Channels

Go-style concurrency with coroutines and message passing:

```cpp
fn main() {
    let (tx, rx) = channel<Message>();
    
    // Spawn background tasks
    spawn producer(tx.clone());
    spawn consumer(rx);
    
    // Structured concurrency
    parallel {
        spawn task_a();
        spawn task_b();
    } // Waits for both tasks to complete
}

fn producer(ch: Sender<Message>) {
    spawn async move {
        for i in 0..100 {
            ch.send(Message::Data(i)).await;
        }
    };
}
```

## Documentation

### Core Language Design
- [Core Philosophy](docs/core-philosophy.md) - Design principles and inspirations
- [Three-Class System](docs/three-class-system.md) - Data, Functional, and Danger classes
- [Memory Management](docs/memory-management.md) - C++ semantics, RAII, and stack containment
- [Access Rights](docs/access-rights.md) - Capability-based security model
- [Type System](docs/type-system.md) - Basic types, generics, and no inheritance

### Language Features
- [Concurrency](docs/concurrency.md) - Coroutines, channels, and defer statements
- [Module System](docs/module-system.md) - Module declarations and boundaries
- [Danger Zones](docs/danger-zones.md) - FFI integration and unsafe operations
- [Compilation](docs/compilation.md) - Three-phase compilation model and C++ code generation
- [Design Patterns](docs/design-patterns.md) - Pattern adaptations and new patterns

## Project Status

CPrime is currently in the design and specification phase. We are:

1. **Creating detailed documentation** for each language component
2. **Designing interactions** between different systems
3. **Planning incremental implementation** starting with the most certain features

### Implementation Roadmap

**Phase 1: Core Foundation**
- Basic type system and three-class enforcement
- Simple functional classes and data classes
- Memory management with RAII and defer

**Phase 2: Access Rights**
- Compile-time access control
- Module boundaries and friend relationships
- Basic capability security

**Phase 3: Concurrency**
- Single-threaded coroutines
- Channel communication primitives
- Structured concurrency patterns

**Phase 4: Advanced Features**
- Danger classes and FFI integration
- Runtime access rights (optional)
- Full M:N threading model

**Phase 5: Tooling**
- Complete compiler implementation
- C++ code generation backend
- Build system integration

## Design Goals

### Safety Without Complexity
- Natural prevention of many bugs through architectural constraints
- No borrow checker needed - simple rules about stack containment
- Clear danger boundaries for unsafe operations

### Performance First
- Zero-cost abstractions by default
- Explicit opt-in for runtime features
- No hidden allocations or indirection
- Direct machine code mapping

### Familiarity and Migration
- C++ programmers feel immediately at home
- Direct interop with existing C++ codebases
- Gradual adoption possible through danger classes
- Clear upgrade path from C++

## Contributing

CPrime is in early design phase. Contributions welcome for:

- Design feedback and discussion
- Documentation improvements
- Prototype implementations
- Example code and use cases

## License

[License TBD - likely MIT or Apache 2.0]

## Comparison with Other Languages

| Feature | CPrime | C++ | Rust | Go |
|---------|--------|-----|------|-----|
| Memory Management | Manual + RAII | Manual + RAII | Borrow Checker | Garbage Collection |
| Inheritance | ❌ Access Rights | ✅ Virtual Inheritance | ❌ Traits | ❌ Interfaces |
| Concurrency | Coroutines + Channels | Threads + Primitives | async/await + Channels | Goroutines + Channels |
| Safety | Architectural | Programmer Responsibility | Compile-time Checked | Runtime Checked |
| Performance | Zero-cost Abstractions | Zero-cost Abstractions | Zero-cost Abstractions | Some Runtime Overhead |
| C++ Interop | Direct | Native | FFI Only | FFI Only |

## Contact

[Contact information TBD]

---

**"CPrime: C++ performance and familiarity, with capability-based security and Go-style concurrency"**