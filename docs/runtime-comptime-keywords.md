# CPrime Runtime and Comptime Keywords

## Overview

`runtime` and `comptime` are fundamental keywords in CPrime that signal **performance characteristics and cost models** throughout the language. They provide explicit, greppable indicators of when code has runtime overhead versus zero-cost abstractions.

**Core Principle**: 
- **`runtime`** = explicit performance cost or overhead
- **`comptime`** = zero runtime cost, compile-time evaluation

These keywords disambiguate between language constructs (compile-time declarations) and execution environments (runtime systems), ensuring developers always understand performance implications.

## I. Keyword Semantics

### Runtime Keyword

**`runtime`** signals that a language construct will have **explicit runtime overhead or cost**:

- **Not referring to "the runtime system"** (execution environment)
- **Compile-time declaration** that configures runtime behavior
- **Always indicates measurable performance cost**
- **Enables informed performance trade-offs**

### Comptime Keyword  

**`comptime`** signals **zero runtime cost** through compile-time evaluation:

- **No runtime overhead** - cost paid at compile-time only
- **Compile-time computation and code generation**
- **Results embedded directly in binary**
- **Maximum performance abstractions**

## II. Complete Usage Reference

### Static Variables

#### Comptime Static
```cpp
comptime static LOOKUP_TABLE: [i32; 256] = generate_table();
comptime static PI: f64 = 3.141592653589793;
comptime static CONFIG: Config = Config::compile_time_default();
```

**Semantics**: Zero runtime initialization cost, values embedded in binary

#### Runtime Static  
```cpp
runtime static mut DATABASE: Database = Database::connect();
runtime static LOGGER: Logger = Logger::initialize();
runtime static mut CACHE: LruCache = LruCache::new(1000);
```

**Semantics**: Explicit startup cost, initialized before main()

### Interface Declarations

#### Comptime Interface (Default)
```cpp
interface Serializable {
    memory_contract {
        id: u64,
        size: u32,
    }
    fn serialize(&self) -> Vec<u8>;
}
```

**Semantics**: Zero-cost memory contracts, direct memory access, compile-time verified

#### Runtime Interface
```cpp
runtime interface FlexibleSerializable {
    data_contract {
        id: u64,
        size: u32,
    }
    fn serialize(&self) -> Vec<u8>;
}
```

**Semantics**: Accessor method overhead, flexible memory layouts, runtime dispatch cost

### Union Declarations

#### Comptime Union (Default)
```cpp
union Message {
    Text { content: String },
    Data { payload: Vec<u8> },
    Signal { code: i32 },
}
```

**Semantics**: Fixed memory size, compile-time layout, zero polymorphic overhead

#### Runtime Union
```cpp
union runtime FlexibleMessage {
    Text { content: String },
    Data { payload: Vec<u8> },
    Binary { data: Vec<u8>, metadata: HashMap<String, String> },
}
```

**Semantics**: Growable memory, polymorphic tagging overhead, runtime type identification

### Functional Class Declarations

#### Comptime Functional Class (Default)
```cpp
functional class MathOps<T: Numeric> {
    fn add(a: T, b: T) -> T { a + b }
    fn multiply(a: T, b: T) -> T { a * b }
}
```

**Semantics**: Static dispatch, compile-time specialization, zero vtable cost

#### Runtime Functional Class
```cpp
runtime functional class DynamicMath {
    fn compute(&self, operation: Operation, a: f64, b: f64) -> f64;
    fn get_name(&self) -> &str;
}
```

**Semantics**: Dynamic dispatch, vtable lookup cost, runtime polymorphism

### Comptime Blocks

```cpp
comptime {
    const size = calculate_optimal_size();
    static_assert(size <= MAX_SIZE);
    
    if size > THRESHOLD {
        generate_large_variant();
    } else {
        generate_small_variant();
    }
}
```

**Semantics**: Compile-time execution, zero runtime cost, metaprogramming

### Thread-Local Storage

#### Comptime Thread-Local
```cpp
comptime thread_local THREAD_ID: u32 = 0;
comptime thread_local CONSTANTS: [f64; 16] = PRECOMPUTED_VALUES;
```

**Semantics**: Zero initialization cost per thread, compile-time values

#### Runtime Thread-Local  
```cpp
runtime thread_local mut CONTEXT: ThreadContext = ThreadContext::new();
runtime thread_local ALLOCATOR: ThreadAllocator = ThreadAllocator::init();
```

**Semantics**: Per-thread initialization cost, runtime setup overhead

## III. Optimization Control Keywords

### Inline Keyword

**`inline`** is a compiler directive for optimization, directly equivalent to C++ `inline`:

- **Forces inlining** when possible for performance
- **Direct C++ counterpart** - same semantics and behavior
- **Optimization hint** - compiler will attempt to inline the code
- **Can be combined** with `comptime` and `runtime` constructs

#### Inline Functions
```cpp
inline constexpr fn fast_multiply(a: i32, b: i32) -> i32 {
    return a * b;  // Always inlined, compile-time when possible
}

inline fn hot_path_function() -> i32 {
    return expensive_calculation();  // Inlined for performance
}

functional class MathOps {
    inline fn add(a: i32, b: i32) -> i32 { a + b }  // Always inlined
    inline fn multiply(a: i32, b: i32) -> i32 { a * b }  // Always inlined
}
```

#### Inline Static Variables
```cpp
inline comptime static FAST_LOOKUP: [i32; 256] = generate_table();
inline runtime static CONNECTION_POOL: Pool = Pool::new();
```

**Semantics**: Inline static variables are inlined at their usage sites when possible

### Volatile Keyword

**`volatile`** prevents compiler optimization, directly equivalent to C++ `volatile`:

- **Prevents optimization** - compiler cannot optimize access
- **Direct C++ counterpart** - same semantics for hardware interaction
- **Memory-mapped I/O** - essential for hardware registers
- **Signal handlers** - prevents optimization of shared variables

#### Volatile Variables
```cpp
volatile runtime static mut HARDWARE_REG: *mut u32 = 0x1000_0000;
volatile static mut SIGNAL_FLAG: bool = false;

fn hardware_interaction() {
    volatile let mut control_reg: *mut u32 = 0x2000_0000;
    unsafe {
        *control_reg = 0xFF;  // Never optimized away
    }
}
```

#### Volatile Function Parameters
```cpp
fn read_hardware_register(volatile reg: *const u32) -> u32 {
    unsafe { *reg }  // Compiler cannot cache or optimize this read
}

volatile fn signal_handler() {
    // This function cannot be optimized - critical for signal safety
    SIGNAL_FLAG = true;
}
```

#### Volatile with Runtime/Comptime
```cpp
// Compile-time volatile - contradiction caught at compile-time
comptime volatile static DATA: i32 = 42;  // ✗ Error: comptime values are const

// Runtime volatile - prevents runtime optimization
volatile runtime static mut SHARED_COUNTER: i32 = 0;  // ✓ Valid
```

### Combined Usage Patterns

#### Performance-Critical Code with Explicit Costs
```cpp
// Fast inlined zero-cost operations
inline comptime static CONSTANTS: [f64; 16] = precompute_trig();

// Fast inlined runtime operations  
inline runtime static mut CACHE: LruCache = LruCache::new();

// Hardware interface - never optimized
volatile runtime static mut DEVICE_STATUS: *mut u32 = 0x3000_0000;

// Hot path function
inline fn calculate_physics(dt: f32) -> Vector3 {
    return apply_forces(dt) + apply_constraints(dt);
}
```

#### Interface Methods with Optimization Control
```cpp
interface FastMath {
    inline fn multiply(&self, a: f64, b: f64) -> f64;      // Always inlined
    volatile fn read_sensor(&self) -> f64;                 // Never optimized
}

runtime interface FlexibleMath {
    inline fn common_operation(&self) -> f64;              // Inline + accessor overhead
    volatile fn hardware_operation(&self) -> f64;          // No optimization + accessor overhead
}
```

### C++ Compatibility

Both keywords maintain exact C++ semantics:

```cpp
// C++ code
inline int fast_add(int a, int b) { return a + b; }
volatile int* hardware_reg = (int*)0x1000;

// CPrime equivalent - identical semantics
inline fn fast_add(a: i32, b: i32) -> i32 { return a + b; }
volatile let hardware_reg: *mut i32 = 0x1000;
```

This ensures seamless interop with C++ codebases and familiar behavior for systems programmers.

## IV. Context Disambiguation

### Language Keywords vs Execution Environment

**Important Distinction**: The `runtime` keyword is NOT the same as "runtime system" or "execution runtime":

| Term | Meaning | Example |
|------|---------|---------|
| **`runtime` keyword** | Language construct with explicit cost | `runtime interface`, `runtime static` |
| **Runtime system** | Execution environment managing program | Coroutine scheduler, signal handler |
| **Execution runtime** | Program's running environment | OS thread pool, memory manager |

```cpp
// KEYWORD: Compile-time declaration with runtime overhead
runtime interface DatabaseOps {
    fn connect(&self) -> Result<Connection>;
}

// NOUN: Execution environment (not the keyword)
fn main() = CoroutineSchedulerRuntime {
    // This execution runtime manages coroutines
    // but uses both comptime AND runtime language constructs
}
```

### Documentation Language Conventions

To avoid confusion, use precise terminology:

```cpp
// ✓ Clear: Keyword usage
"The `runtime` keyword indicates performance overhead"
"Use `runtime interface` for flexible memory layouts"

// ✓ Clear: Execution environment  
"The execution runtime manages coroutines"
"The runtime system handles signal delivery"

// ✗ Ambiguous
"Runtime provides dynamic behavior"  // Which runtime?
"The runtime supports both modes"    // Keyword or system?
```

## IV. Performance Implications

### Cost Analysis by Keyword

#### Comptime Constructs - Zero Runtime Cost
```cpp
comptime static TABLE: [i32; 1000] = precompute();    // 0ms at startup
interface Cacheable { /* memory contract */ }         // 0 bytes overhead  
union Message { Text, Data }                          // 0 polymorphic cost
functional class Math { fn add(...) }                 // 0 dispatch cost
```

#### Runtime Constructs - Explicit Runtime Cost
```cpp
runtime static DB: Database = connect();              // ~100ms at startup
runtime interface Flexible { /* accessors */ }       // +8 bytes vtable
union runtime Dynamic { Text, Data }                  // +4 bytes type tag
runtime functional class Service { /* vtable */ }     // +8 bytes vtable
```

### Measurement and Profiling

```bash
# Find all runtime costs
cprime analyze --runtime-costs
  Runtime statics: 3 found (250ms total startup)
  Runtime interfaces: 12 implementations (96 bytes vtable overhead)
  Runtime unions: 8 types (32 bytes tagging overhead)
  Runtime functional classes: 6 classes (48 bytes vtable overhead)

# Find all comptime optimizations
cprime analyze --comptime-optimizations  
  Comptime statics: 15 (embedded in binary, 0ms startup)
  Comptime interfaces: 23 (zero-cost memory contracts)
  Comptime unions: 11 (compile-time layout, no tagging)
  Comptime functional classes: 45 (static dispatch, inlined)
```

## V. Performance Implications

### Cost Analysis by Keyword

#### Comptime Constructs - Zero Runtime Cost
```cpp
comptime static TABLE: [i32; 1000] = precompute();    // 0ms at startup
interface Cacheable { /* memory contract */ }         // 0 bytes overhead  
union Message { Text, Data }                          // 0 polymorphic cost
functional class Math { fn add(...) }                 // 0 dispatch cost
```

#### Runtime Constructs - Explicit Runtime Cost
```cpp
runtime static DB: Database = connect();              // ~100ms at startup
runtime interface Flexible { /* accessors */ }       // +8 bytes vtable
union runtime Dynamic { Text, Data }                  // +4 bytes type tag
runtime functional class Service { /* vtable */ }     // +8 bytes vtable
```

#### Optimization Control - Performance Trade-offs
```cpp
inline comptime static FAST_CONST: i32 = 42;         // 0ms + inlined access
volatile runtime static mut HARDWARE: *mut u32;      // Startup cost + no optimization
inline fn hot_path() -> i32 { fast_calc() }          // Inlined function call
volatile fn signal_handler() { /* never optimized */ } // No optimization allowed
```

### Measurement and Profiling

```bash
# Find all runtime costs
cprime analyze --runtime-costs
  Runtime statics: 3 found (250ms total startup)
  Runtime interfaces: 12 implementations (96 bytes vtable overhead)
  Runtime unions: 8 types (32 bytes tagging overhead)
  Runtime functional classes: 6 classes (48 bytes vtable overhead)

# Find all comptime optimizations
cprime analyze --comptime-optimizations  
  Comptime statics: 15 (embedded in binary, 0ms startup)
  Comptime interfaces: 23 (zero-cost memory contracts)
  Comptime unions: 11 (compile-time layout, no tagging)
  Comptime functional classes: 45 (static dispatch, inlined)

# Find optimization directives
cprime analyze --optimization-hints
  Inline functions: 23 (forced inlining for performance)
  Volatile variables: 5 (optimization prevention for correctness)
  Combined inline+comptime: 12 (maximum performance)
  Combined volatile+runtime: 8 (explicit cost + no optimization)
```

## VI. Usage Guidelines

### When to Use Runtime

Use `runtime` when you need:

**Flexibility over Performance**:
```cpp
// Dynamic interface implementations
runtime interface PluginSystem {
    fn load_plugin(&mut self, path: &str) -> Result<()>;
    fn execute(&self, command: &str) -> Result<String>;
}

// Growable union types
union runtime NetworkMessage {
    HttpRequest { headers: HashMap<String, String>, body: Vec<u8> },
    WebSocketFrame { opcode: u8, payload: Vec<u8> },
    CustomProtocol { protocol_id: u32, data: Vec<u8> },
}
```

**Runtime Configuration**:
```cpp
// Configuration loaded at startup
runtime static CONFIG: AppConfig = AppConfig::from_environment();

// Database connections with startup cost
runtime static mut DB_POOL: ConnectionPool = ConnectionPool::new();
```

### When to Use Comptime

Use `comptime` when you want:

**Maximum Performance**:
```cpp
// Zero-cost lookup tables
comptime static PERFECT_HASH: [u32; 256] = generate_perfect_hash();

// Compile-time computation
comptime static PRIMES: [u32; 1000] = sieve_of_eratosthenes();
```

**Zero-Cost Abstractions**:
```cpp
// Memory contracts without overhead
interface Cacheable {
    memory_contract {
        key: u64,
        timestamp: u64,
        size: u32,
    }
}

// Compile-time specialization
functional class VectorOps<T, const N: usize> {
    comptime fn dot_product(a: &[T; N], b: &[T; N]) -> T {
        // Unrolled at compile-time for each N
    }
}
```

### Migration Strategies

#### From Runtime to Comptime (Performance Optimization)
```cpp
// Before: Runtime overhead
runtime interface Processor {
    fn process(&self, data: &[u8]) -> Vec<u8>;
}

// After: Zero-cost alternative
interface Processor {
    memory_contract {
        algorithm_id: u8,
        buffer_size: u32,
    }
    fn process(&self, data: &[u8]) -> Vec<u8>;
}
```

#### From Comptime to Runtime (Flexibility Requirement)
```cpp  
// Before: Compile-time fixed
union Message {
    Text { content: String },
    Data { payload: Vec<u8> },
}

// After: Runtime flexibility for plugin system
union runtime ExtensibleMessage {
    Text { content: String },
    Data { payload: Vec<u8> },
    Plugin { type_id: u32, data: Vec<u8> },
}
```

### When to Use Inline

Use `inline` when you need:

**Performance-Critical Functions**:
```cpp
inline fn vector_dot_product(a: &[f32], b: &[f32]) -> f32 {
    let mut sum = 0.0;
    for i in 0..a.len() {
        sum += a[i] * b[i];
    }
    sum
}
```

**Small Utility Functions**:
```cpp
functional class StringOps {
    inline fn is_empty(s: &str) -> bool { s.len() == 0 }
    inline fn char_at(s: &str, idx: usize) -> char { s.chars().nth(idx).unwrap() }
}
```

**Header-Only Utilities**:
```cpp
inline constexpr fn max(a: i32, b: i32) -> i32 {
    return if a > b { a } else { b };
}
```

### When to Use Volatile

Use `volatile` when you need:

**Hardware Interaction**:
```cpp
volatile runtime static mut GPIO_CONTROL: *mut u32 = 0x4000_0000;
volatile runtime static mut UART_DATA: *mut u8 = 0x4000_1000;

fn read_sensor() -> u16 {
    volatile let sensor_reg: *const u16 = 0x5000_0000;
    unsafe { *sensor_reg }  // Never cached or optimized
}
```

**Signal Handler Variables**:
```cpp
volatile static mut INTERRUPT_COUNT: u32 = 0;

volatile fn interrupt_handler() {
    INTERRUPT_COUNT += 1;  // Never optimized away
}
```

**Multi-threaded Shared Data** (with proper synchronization):
```cpp
volatile runtime static mut SHUTDOWN_FLAG: bool = false;

fn worker_thread() {
    while !SHUTDOWN_FLAG {  // Always reads from memory
        do_work();
    }
}
```

## VII. Integration with Language Features

### Interaction with Other Keywords

#### With Danger Classes
```cpp
danger class UnsafeBuffer {
    // Cannot use comptime - danger zones require runtime checks
    runtime static mut GLOBAL_BUFFER: *mut u8 = std::ptr::null_mut();
}
```

#### With Semconst Fields
```cpp
class Config {
    semconst database_url: String,
    
    // Can expose via different cost models
    exposes comptime ConfigOps { database_url }    // Zero-cost access
    exposes runtime DynamicOps { database_url }    // Dynamic capability
}
```

#### With Coroutines
```cpp
// Coroutine storage with different cost models
comptime static CORO_POOL: [CoroutineSlot; 1000] = init_pool();  // Pre-allocated
runtime static mut DYNAMIC_COROS: Vec<Coroutine> = Vec::new();   // Growable
```

### Error Messages and Debugging

The keywords provide clear error contexts:

```cpp
comptime static INVALID: Database = Database::connect();
// Error: Cannot use runtime function 'Database::connect' in comptime static initializer
//        Suggestion: Use 'runtime static' or provide compile-time alternative

runtime interface TooFast {
    memory_contract { /* ... */ }
}
// Error: 'memory_contract' requires comptime interface for zero-cost access
//        Suggestion: Remove 'runtime' or use 'data_contract'
```

## VIII. Advanced Patterns

### Conditional Compilation with Cost Models

```cpp
#[cfg(optimize_for = "speed")]
comptime static ALGORITHM: AlgorithmImpl = FastAlgorithm::new();

#[cfg(optimize_for = "flexibility")]
runtime static mut ALGORITHM: Box<dyn Algorithm> = Box::new(PluggableAlgorithm::new());

// Usage remains the same
let result = ALGORITHM.process(data);
```

### Hybrid Cost Models

```cpp
// Static data with runtime configuration
comptime static BASE_CONFIG: BaseConfig = load_base_config();
runtime static mut OVERRIDES: ConfigOverrides = ConfigOverrides::from_args();

functional class ConfigOps {
    fn get_value(key: &str) -> Option<Value> {
        // Check runtime overrides first (cost: hash lookup)
        if let Some(value) = OVERRIDES.get(key) {
            return Some(value);
        }
        // Fall back to compile-time config (cost: zero)
        BASE_CONFIG.get(key)
    }
}
```

### Cost-Aware Generic Programming

```cpp
trait Storage<T> {
    type Cost: CostModel;
    fn store(&mut self, item: T) -> StoreResult<Self::Cost>;
}

// Zero-cost implementation
impl<T, const N: usize> Storage<T> for ArrayStorage<T, N> {
    type Cost = CompileTime;  // Signals zero runtime cost
    
    comptime fn store(&mut self, item: T) -> StoreResult<CompileTime> {
        // Compile-time bounds checking, zero runtime cost
    }
}

// Runtime-cost implementation  
impl<T> Storage<T> for VecStorage<T> {
    type Cost = Runtime;  // Signals runtime overhead
    
    fn store(&mut self, item: T) -> StoreResult<Runtime> {
        // Runtime bounds checking, allocation costs
    }
}
```

## IX. Cross-Reference Guide

### Where Each Keyword Appears

| Language Feature | Comptime Variant | Runtime Variant | Optimization Control | Default |
|------------------|------------------|-----------------|----------------------|---------|
| Static Variables | `comptime static` | `runtime static` | `inline`, `volatile` | **Must specify** |
| Interfaces | `interface` (implicit) | `runtime interface` | `inline` methods | **Comptime** |
| Unions | `union` (implicit) | `union runtime` | N/A | **Comptime** |
| Functional Classes | `functional class` | `runtime functional class` | `inline` methods | **Comptime** |
| Thread-Local | `comptime thread_local` | `runtime thread_local` | `inline`, `volatile` | **Must specify** |
| Blocks | `comptime { }` | N/A | N/A | **Comptime only** |
| Functions | `constexpr fn` | `fn` | `inline fn`, `volatile fn` | **Runtime** |
| Variables | N/A | Local variables | `volatile let` | **Runtime** |

### Related Documentation

- **[Statics and Constexpr](statics-and-constexpr.md)**: Complete static variable design
- **[Interfaces](interfaces.md)**: Interface constructs and memory contracts  
- **[Unions](unions.md)**: Union constructs and polymorphic tagging
- **[Access Rights](access-rights.md)**: Functional classes and access mechanisms
- **[Runtime System](runtime-system.md)**: Execution runtime environments

## Summary

CPrime provides a comprehensive set of performance control keywords:

### Cost-Signaling Keywords
- **`runtime`**: Explicit performance cost, measurable overhead, runtime flexibility
- **`comptime`**: Zero runtime cost, compile-time evaluation, maximum performance

### Optimization Control Keywords  
- **`inline`**: Forces inlining when possible, direct C++ counterpart for performance
- **`volatile`**: Prevents compiler optimization, direct C++ counterpart for correctness

### Complete Performance Vocabulary
By combining cost-signaling with optimization control, CPrime provides explicit performance semantics:
- `inline comptime static` - Maximum performance (zero cost + inlined)
- `volatile runtime static mut` - Explicit cost + no optimization  
- `inline constexpr fn` - Fast compile-time/runtime dual-mode functions
- `volatile fn` - Functions that must never be optimized

This enables developers to make informed decisions about performance trade-offs while maintaining clear separation between language constructs and execution environments, with familiar C++ optimization semantics.