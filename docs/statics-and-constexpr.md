# CPrime Statics, Constexpr, and Pluggable Interop Design

## Overview

CPrime's static variables and constexpr system is built around explicit cost signaling: **`runtime` indicates explicit cost**, while **`comptime` indicates zero cost**. This fundamental principle extends throughout the language, ensuring developers always know when they're paying performance costs.

The language provides only two types of static variables in its core, with additional interop capabilities available through pluggable modules.

## I. Static Variable Design

### Core Principle

- **No implicit initialization costs** - all costs must be explicit
- **Must explicitly specify when initialization occurs** 
- **Only two types in core language** (`extern` is in interop module)

### Comptime Static Variables

Zero-cost static variables evaluated at compile-time:

```cpp
comptime static TABLE: [i32; 256] = generate_table();
comptime static PI: f64 = 3.141592653589793;
comptime static MAX_SIZE: usize = 1024;
```

#### Characteristics

- **Evaluated at compile-time** - zero runtime initialization cost
- **Zero runtime cost** - becomes literal data in binary
- **Must have compile-time computable initializer**
- **Can be mutable or immutable**

#### Rules

- **Initializer must be constexpr or comptime block**
- **Cannot be declared without initialization**
- **Cannot have separate assignment**
- **Compiler error if initializer not compile-time evaluable**

```cpp
comptime static LOOKUP: [i32; 256] = {
    let mut array = [0i32; 256];
    for i in 0..256 {
        array[i] = i * i;
    }
    array
};

// Error cases
comptime static INVALID: Database;                    // ✗ No initializer
comptime static RUNTIME_FUNC: i32 = get_input();     // ✗ Runtime function
```

#### Use Cases

- Lookup tables and precomputed data
- Mathematical constants
- Configuration values known at compile-time
- Precomputed data structures

### Runtime Static Variables

Explicit startup-cost static variables evaluated at runtime:

```cpp
runtime static mut CONNECTION: Database = Database::connect();
runtime static LOGGER: Logger = Logger::init();
runtime static CONFIG: Config = Config::load_from_file();
```

#### Characteristics

- **Evaluated at runtime before main()** - explicit startup cost
- **Explicit startup cost** signaled by `runtime` keyword
- **Can use any initializer** - no restrictions
- **Can be mutable or immutable**

#### Rules

- **Can be declared without initialization** (separate assignment later)
- **Can have separate assignment after declaration**
- **Initialization order follows declaration order within module**
- **Cross-module order undefined** (must not depend on it)

```cpp
// Valid patterns
runtime static mut DATABASE: Database;           // Declaration only
DATABASE = Database::connect();                  // Later assignment

runtime static CACHE: LruCache = LruCache::new(1000);  // Direct initialization

// Cross-module dependency - undefined behavior
runtime static A: Service = Service::new();     // Module 1
runtime static B: Client = Client::connect(&A); // Module 2 - ✗ Undefined order
```

#### Use Cases

- Resource handles (files, sockets, databases)
- Runtime configuration loading
- System state initialization
- Logging and monitoring infrastructure

### Declaration vs Definition

```cpp
// Comptime - must initialize at declaration
comptime static X: i32 = 42;              // ✓ Valid
comptime static Y: i32;                   // ✗ Error: no initializer

// Runtime - can separate
runtime static mut Z: Database;           // ✓ Declaration
Z = Database::connect();                  // ✓ Later assignment
```

### Thread-Local Storage

Both static types support thread-local storage with the same rules:

```cpp
// Zero-cost thread-local
comptime thread_local THREAD_ID: i32 = 0;

// Runtime-cost thread-local  
runtime thread_local mut CONTEXT: Context = Context::new();
```

### Class Static Members

Static members follow the same cost-signaling rules:

```cpp
class Cache {
    comptime static DEFAULT_SIZE: usize = 1024;    // Zero cost
    runtime static mut INSTANCE: Cache = Cache::new();  // Startup cost
    
    // Cost is explicit in all usage
    fn get_default_size() -> usize {
        Self::DEFAULT_SIZE  // Zero cost access
    }
    
    fn get_instance() -> &Cache {
        &Self::INSTANCE     // Runtime-initialized access
    }
}
```

## II. Constexpr Design

### Core Concept

**`constexpr` means "can be evaluated at compile-time"** (not "must be"). This matches C++ constexpr semantics and provides dual-mode execution.

### Constexpr Functions

Functions that can execute at compile-time when inputs are constant:

```cpp
constexpr fn factorial(n: i32) -> i32 {
    return (n <= 1) ? 1 : n * factorial(n - 1);
}

constexpr fn fibonacci(n: u32) -> u64 {
    if n <= 1 { return n as u64; }
    return fibonacci(n - 1) + fibonacci(n - 2);
}
```

#### Dual-Mode Execution

```cpp
// Compile-time when inputs are constant
comptime static FACT_10: i32 = factorial(10);        // Compile-time
const SIZE: usize = factorial(5);                     // Compile-time

// Runtime when inputs are runtime values
let n = get_user_input();
let result = factorial(n);                            // Runtime
```

#### Restrictions

Constexpr functions have specific limitations to ensure compile-time evaluability:

- **No I/O operations** - cannot read files, network, etc.
- **No coroutine operations** - no async/await
- **No channel/signal operations** - no concurrent operations
- **No danger zones** - no unsafe operations
- **No dynamic allocation** (or must be freed in same expression)
- **Only call other constexpr functions**

```cpp
constexpr fn valid_math(x: i32, y: i32) -> i32 {
    return x * x + y * y;  // ✓ Valid
}

constexpr fn invalid_io() -> String {
    return read_file("config.txt");  // ✗ I/O not allowed
}

constexpr fn invalid_alloc(size: usize) -> Vec<i32> {
    return Vec::with_capacity(size);  // ✗ Dynamic allocation
}
```

#### Where Allowed

```cpp
// ✓ Functional classes
functional class Math {
    constexpr fn compute(x: i32) -> i32 { 
        return x * 2 + 1; 
    }
}

// ✓ Module-level functions
constexpr fn hash_string(s: &str) -> u32 {
    let mut hash = 0u32;
    for byte in s.bytes() {
        hash = hash.wrapping_mul(31).wrapping_add(byte as u32);
    }
    hash
}

// ✗ Data class methods (they only do memory management)
// ✗ Danger zones (incompatible with compile-time guarantees)
```

### Constexpr Values

Compile-time computable constants:

```cpp
constexpr PI: f64 = 3.14159265359;
constexpr MAX_BUFFER: usize = 1024;
constexpr DEFAULT_TIMEOUT: Duration = Duration::from_secs(30);
```

### Interaction with Comptime

Constexpr enables powerful compile-time metaprogramming:

```cpp
constexpr fn is_power_of_two(n: u32) -> bool {
    return n > 0 && (n & (n - 1)) == 0;
}

comptime {
    // Forces compile-time evaluation
    const buffer_size = 1024;
    static_assert(is_power_of_two(buffer_size));
    
    // Use for conditional compilation
    if is_power_of_two(CACHE_SIZE) {
        generate_optimized_cache_variant();
    } else {
        generate_generic_cache_variant();
    }
}
```

## III. Unified Static/Constexpr Rules

### Static Initialization with Constexpr

Decision matrix for combining static types with initializers:

| Declaration      | Initializer   | When Evaluated | Valid? | Example |
|------------------|---------------|----------------|--------|---------|
| `comptime static` | literal       | Compile-time   | ✓      | `comptime static X: i32 = 42;` |
| `comptime static` | constexpr fn  | Compile-time   | ✓      | `comptime static Y: i32 = factorial(5);` |
| `comptime static` | runtime fn    | Error          | ✗      | `comptime static Z: i32 = get_input();` |
| `comptime static` | comptime block| Compile-time   | ✓      | `comptime static W: [i32; 10] = { ... };` |
| `runtime static`  | literal       | Runtime        | ✓      | `runtime static A: i32 = 42;` |
| `runtime static`  | constexpr fn  | Runtime        | ✓      | `runtime static B: i32 = factorial(5);` |
| `runtime static`  | runtime fn    | Runtime        | ✓      | `runtime static C: i32 = get_input();` |
| `static` (alone)  | any           | Error          | ✗      | **Must specify `comptime` or `runtime`** |

```cpp
// Valid combinations
comptime static SIZE: usize = constexpr_compute();     // Compile-time evaluation
runtime static VALUE: i32 = constexpr_compute();       // Runtime evaluation of constexpr
runtime static DATA: Database = Database::connect();   // Runtime-only function

// Invalid - must specify cost model
static AMBIGUOUS: i32 = 42;                           // ✗ Error: specify comptime or runtime
```

## IV. Pluggable Interop Design

### Core Language Purity

CPrime's core language contains **no foreign function interface mechanisms**:

```cpp
// Core cprime has NO:
// - extern keyword
// - FFI mechanisms  
// - C/C++ ABI concerns
// - danger(ffi) zones
// - Name mangling rules
```

This keeps the core language clean and allows for multiple interop strategies.

### Interop as Optional Module

Foreign function interface capabilities are provided through pluggable modules:

```toml
# Project without interop - pure CPrime
[project]
name = "pure_cprime_app"
# No interop dependency = no FFI available

# Project with C++ interop
[project]
name = "system_app"

[dependencies]
cpp_interop = "1.0"  # Adds FFI capabilities
```

### What Interop Modules Add

#### Extern Static Support

```cpp
#[requires(cpp_interop)]
extern static errno: i32;

#[requires(cpp_interop)]
extern static "C" {
    static global_counter: i32;
}
```

#### FFI Danger Zones

```cpp
#[requires(cpp_interop)]
#[danger(ffi)]
extern "C" fn malloc(size: usize) -> *mut u8;

#[requires(cpp_interop)]
#[danger(ffi)]
extern "C" fn free(ptr: *mut u8);
```

#### ABI Specifications

```cpp
#[requires(cpp_interop)]
#[repr(C)]
struct CCompatible {
    x: i32,
    y: f64,
}

#[requires(cpp_interop)]
#[repr(C++)]
class CppClass {
    virtual_method: fn(),
}
```

#### Language Bridges

```cpp
#[requires(cpp_interop)]
cpp! {
    #include <vector>
    std::vector<int> vec = {1, 2, 3};
}

#[requires(c_interop)]
c! {
    printf("Hello from C\n");
}
```

### Multiple Interop Modules

Different interop strategies can coexist:

```toml
# Different interop strategies
c_interop = "1.0"       # Minimal C ABI only
cpp_interop = "1.0"     # Full C++ support
rust_interop = "1.0"    # Rust ABI compatibility  
wasm_interop = "1.0"    # WebAssembly bindings
jni_interop = "1.0"     # Java Native Interface
python_interop = "1.0"  # Python C API
```

### Interop Module Interface

Each interop module must implement a standard interface:

```cpp
// Each interop module must provide
trait InteropModule {
    // Type mappings
    fn map_foreign_type(type: ForeignType) -> CprimeType;
    
    // Name mangling
    fn mangle(name: &str, signature: &Signature) -> String;
    fn demangle(mangled: &str) -> (String, Signature);
    
    // ABI handling
    fn calling_convention() -> CallingConvention;
    fn struct_layout(s: &Struct) -> Layout;
    
    // Code generation
    fn generate_binding(decl: &ExternDecl) -> Code;
}
```

## V. Compilation and Linking

### Pure CPrime Compilation

```bash
# No interop - smaller, faster compilation
cprime build --no-interop

# Produces:
# - Pure cprime binary
# - No FFI symbols
# - No C++ runtime dependency
# - Smaller binary size
```

### With Interop

```bash
# With specific interop
cprime build --interop=cpp

# Produces:
# - Binary with FFI support
# - C++ runtime linked
# - Mangled symbols for C++ compatibility
# - Additional dependency overhead
```

### Conditional Compilation

```cpp
#[cfg(has_interop)]
module ffi_bindings {
    extern static c_global: i32;
}

#[cfg(not(has_interop))]  
module ffi_bindings {
    // Pure cprime fallback
    runtime static c_global: i32 = 0;
}
```

## VI. Migration and Evolution

### Version Evolution Strategy

```cpp
// cprime 1.0 - Core language
comptime static X: i32 = 42;
runtime static Y: Logger = Logger::new();

// cprime 2.0 - New static type (hypothetical)
lazy static Z: Database = Database::connect();  // Added without breaking core

// cpp_interop 1.0
extern static errno: i32;

// cpp_interop 2.0 - Enhanced design
extern static errno: i32 with(thread_safe);  // Enhanced without breaking core
```

### Deprecation Path

```cpp
// Old interop module
#[deprecated("Use cpp_interop_v2")]
module cpp_interop_v1 { ... }

// New interop module  
module cpp_interop_v2 { ... }

// User code can migrate gradually
#[allow(deprecated)]
use cpp_interop_v1::legacy_function;
```

## VII. Tooling Integration

### Build System

```toml
[build]
default_interop = "none"  # Pure by default

[build.profiles.embedded]
interop = "none"          # No FFI for embedded

[build.profiles.desktop]
interop = "cpp"           # C++ for desktop apps

[build.profiles.web]
interop = "wasm"          # WASM for web
```

### Analysis Tools

```bash
# Find all runtime costs
cprime analyze --startup-costs
  3 runtime statics found:
    main.cp:10 - runtime static LOGGER (50ms)
    db.cp:5 - runtime static CONNECTION (200ms)
  Total: 250ms

# Find all FFI boundaries
cprime analyze --ffi-usage
  FFI used in modules: [network, filesystem]
  Consider pure cprime alternatives

# Security audit
cprime audit --no-ffi
  ✓ No FFI usage found
  ✓ Binary has no foreign dependencies
```

### Performance Profiling

```bash
# Profile static initialization
cprime profile --static-init
  Startup profile:
  - comptime statics: 0ms (embedded in binary)
  - runtime statics: 250ms total
    - DATABASE (150ms)
    - LOGGER (50ms)
    - CONFIG (50ms)
```

## VIII. Documentation Strategy

### Core Language Docs

- **No mention of FFI/extern** in core documentation
- **Focus on pure cprime concepts** only
- **Examples use only comptime/runtime static**
- **Clear cost signaling** throughout

### Interop Module Docs

- **Separate documentation** for each interop module
- **"Advanced: Language Interoperability"** section
- **Clear warnings** about safety and performance implications
- **Migration guides** from pure cprime

### Teaching Progression

1. **Beginner**: `comptime`/`runtime` static only, cost awareness
2. **Intermediate**: constexpr and comptime blocks
3. **Advanced**: Interop modules when needed
4. **Expert**: Writing custom interop modules

## IX. Benefits Summary

### Language Benefits

- **Pure core language** without legacy baggage
- **Explicit costs everywhere** - no hidden performance implications
- **Modular complexity** - pay only for what you use
- **Evolution freedom** - can change interop without breaking core

### Developer Benefits

- **Clear mental model** - two static types with explicit costs
- **No hidden initialization costs** - `runtime` keyword signals cost
- **Greppable runtime costs** - search for `runtime static`
- **Optional FFI** - can avoid entirely for pure applications

### Ecosystem Benefits

- **Multiple interop strategies** possible
- **Gradual migration** from C/C++ 
- **Security by default** - no FFI unless explicitly enabled
- **Embeddable** - can strip all interop for minimal binaries

## Cost Signaling Philosophy

This design establishes the fundamental CPrime principle: **`runtime` indicates explicit cost, `comptime` indicates zero cost**. This pattern extends throughout the entire language:

- `runtime static` - explicit startup cost
- `comptime static` - zero runtime cost
- `runtime interface` - accessor method overhead
- `comptime` blocks - zero runtime cost evaluation
- `runtime unions` - polymorphic tagging overhead

By making costs explicit through keywords, developers always understand the performance implications of their choices, enabling informed decisions about trade-offs between convenience and performance.