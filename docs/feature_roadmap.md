# CPrime Feature Roadmap

## Recently Completed Features ✅

### Core Language Infrastructure
- [x] Basic lexer, parser, and AST structure
- [x] LLVM IR code generation backend
- [x] Symbol table and type system
- [x] Basic control flow (if, while, for loops)
- [x] Function declarations and calls
- [x] Class definitions with fields
- [x] Print function with format string support
- [x] Field access on class instances

### Pointer and Reference System
- [x] Lexer tokens for pointer/reference syntax (`*`, `&`, `&&`)
- [x] Type system extensions (POINTER, REFERENCE, RVALUE_REFERENCE)
- [x] AST structures for pointer and reference types
- [x] Parser support for pointer/reference declarations
- [x] AST nodes for pointer operations (dereference, address-of)

### Special Member Functions (Rule of Five)
- [x] Complete special member parsing (constructors, assignment operators, destructor)
- [x] `= default` and `= delete` syntax support
- [x] Symbol table tracking of special member availability
- [x] Automatic constructor calls on object creation
- [x] Automatic destructor calls at scope exit (LIFO order)
- [x] Scope-based object lifecycle management
- [x] Warning documentation for constructor/destructor mismatches

## High Priority Features 🔥

### Defer Statement Implementation
- [ ] Add `defer` keyword token to lexer
- [ ] Create DeferStatement AST node
- [ ] Implement defer parsing in statement parser
- [ ] Add defer stack management to CodeGenerator
- [ ] Generate deferred cleanup calls in LIFO order
- [ ] Handle defer with function returns and early exits
- [ ] Create comprehensive defer tests
- [ ] Document defer usage patterns

### Aggregate Initialization
- [ ] Add support for `ClassName{}` syntax parsing
- [ ] Implement aggregate initialization in expression parser
- [ ] Generate LLVM IR for aggregate construction
- [ ] Add support for field initialization lists `ClassName{field1: value1, field2: value2}`
- [ ] Handle nested aggregate initialization
- [ ] Create tests for various aggregate patterns
- [ ] Document aggregate initialization syntax

## Medium Priority Features 📋

### Memory Management
- [ ] Implement Box<T> smart pointer type
- [ ] Add Rc<T> reference counting smart pointer
- [ ] Add Arc<T> atomic reference counting for threading
- [ ] Custom allocator support
- [ ] Arena allocator implementation
- [ ] Object pool allocator
- [ ] Stack vs heap allocation control

### Enhanced Type System
- [ ] Generic types and templates
- [ ] Type inference improvements
- [ ] Custom type conversions
- [ ] Pointer arithmetic operations
- [ ] Reference type validation
- [ ] Const correctness system

### Error Handling
- [ ] Result<T, E> type implementation
- [ ] Option<T> type implementation
- [ ] Pattern matching for Result/Option
- [ ] Try operator (?) for error propagation
- [ ] Panic and abort mechanisms
- [ ] Stack unwinding on errors

### Advanced Language Features
- [ ] Traits/interfaces system
- [ ] Method implementations on types
- [ ] Operator overloading
- [ ] Iterator protocol
- [ ] Closure support
- [ ] Lambda expressions

## Low Priority Features 📝

### Concurrency (Go-inspired)
- [ ] Coroutine (`spawn`) implementation
- [ ] Channel types for communication
- [ ] Select statement for channel operations
- [ ] M:N threading runtime
- [ ] Work stealing scheduler
- [ ] Structured concurrency (parallel blocks)
- [ ] Cancellation and timeouts

### Standard Library
- [ ] Collections (Vec, HashMap, BTreeMap)
- [ ] String manipulation utilities
- [ ] File I/O operations
- [ ] Network programming primitives
- [ ] Time and date handling
- [ ] Regular expressions
- [ ] JSON serialization/deserialization

### Developer Experience
- [ ] Enhanced error messages with source locations
- [ ] IDE integration (LSP server)
- [ ] Debugger support (DWARF generation)
- [ ] Package manager and build system
- [ ] Documentation generation
- [ ] Benchmark framework
- [ ] Profiling tools integration

### Optimization Features
- [ ] Dead code elimination
- [ ] Inlining optimizations
- [ ] Move semantics optimization
- [ ] Lifetime analysis for better codegen
- [ ] SIMD instruction generation
- [ ] Link-time optimization (LTO)

## Future Considerations 🔮

### Advanced Memory Features
- [ ] Garbage collection option (optional)
- [ ] Memory-mapped file support
- [ ] Custom memory layouts
- [ ] NUMA-aware allocation
- [ ] Memory pool management

### Platform Integration
- [ ] C/C++ FFI (Foreign Function Interface)
- [ ] System call wrappers
- [ ] Platform-specific optimizations
- [ ] WebAssembly target support
- [ ] Embedded systems support

### Language Interop
- [ ] C header parsing and binding generation
- [ ] Python bindings
- [ ] JavaScript/WebAssembly integration
- [ ] Rust interoperability

## Implementation Notes

### Next Steps (Immediate)
1. **Defer Statement**: Critical for proper resource management, aligns with Go-style patterns documented in concurrency.md
2. **Aggregate Initialization**: Needed to make constructor/destructor tests more comprehensive and practical
3. **Complete Pointer/Reference Operations**: Finish the partially implemented pointer system

### Architecture Decisions
- CPrime follows C++ memory semantics (not Rust borrow checker)
- Three-class system: data classes, functional classes, danger classes
- RAII patterns encouraged through constructor/destructor lifecycle
- Manual memory management with smart pointer abstractions
- Compositional polymorphism instead of inheritance

### Testing Strategy
- Each major feature requires comprehensive test coverage
- Integration tests for feature combinations
- Performance benchmarks for memory management features
- Compatibility tests with C++ migration patterns

## Documentation Updates Needed
- [ ] Update language reference with defer syntax
- [ ] Add aggregate initialization examples
- [ ] Create memory management best practices guide
- [ ] Document three-class system usage patterns
- [ ] Add concurrency programming guide when implemented