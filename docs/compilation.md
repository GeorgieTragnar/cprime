# CPrime Compilation Model

## Overview

CPrime uses a three-phase compilation pipeline designed for maximum optimization while maintaining fast incremental builds. The initial target is C++ code generation for immediate compatibility and toolchain reuse.

## Three-Phase Compilation Pipeline

### Phase 1: Frontend (Parse + Type Check)

The frontend performs parsing, semantic analysis, and type checking:

```bash
# Frontend compilation
cprimec frontend src/**/*.cp --emit-ir --output build/ir/

# What this phase does:
# 1. Lexical analysis and parsing
# 2. AST construction and validation
# 3. Type checking and inference
# 4. Access rights verification
# 5. Three-class system enforcement
# 6. IR generation
```

#### Frontend Outputs

- **Abstract Syntax Tree (AST)**: Structured representation of source code
- **Type Information**: Complete type annotations and constraints
- **Access Rights Table**: Module friendship and capability mappings
- **Intermediate Representation (IR)**: High-level, platform-independent code

#### Type System Integration

```cpp
// Source code
class Point {
    x: f64,
    y: f64,
    constructed_by: PointOps,
}

functional class PointOps {
    fn construct(x: f64, y: f64) -> Point {
        Point { x, y }
    }
    
    fn distance(p1: &Point, p2: &Point) -> f64 {
        let dx = p1.x - p2.x;
        let dy = p1.y - p2.y;
        (dx*dx + dy*dy).sqrt()
    }
}

// Generated IR (conceptual)
DataClass Point {
    fields: [
        Field { name: "x", type: f64, access: private },
        Field { name: "y", type: f64, access: private }
    ],
    constructor_access: [PointOps],
    memory_ops: default
}

FunctionalClass PointOps {
    methods: [
        Method {
            name: "construct",
            signature: (f64, f64) -> Point,
            access: public,
            body: IR_construct_body
        },
        Method {
            name: "distance", 
            signature: (&Point, &Point) -> f64,
            access: public,
            body: IR_distance_body
        }
    ]
}
```

### Phase 2: Optimization (Optional)

The optimizer performs high-level optimizations on the IR:

```bash
# Optimization phase
cprimec optimize build/ir/**/*.ir --level=3 --output build/optimized/

# Optimization levels:
# --level=0    No optimization (debug builds)
# --level=1    Basic optimizations (function inlining, dead code elimination)
# --level=2    Advanced optimizations (loop unrolling, vectorization)
# --level=3    Aggressive optimization (cross-module optimization)
```

#### Optimization Passes

1. **Function Inlining**: Inline functional class methods
2. **Dead Code Elimination**: Remove unused code and data
3. **Access Rights Optimization**: Compile-time friend resolution
4. **Memory Layout Optimization**: Struct packing and alignment
5. **Coroutine Transformation**: Convert to state machines
6. **Channel Optimization**: Replace channels with direct calls when possible

#### Three-Class System Optimizations

```cpp
// Original code
let data = DataOps::construct(42);
let result = DataOps::process(&data);

// After optimization - direct calls
let data = Data { value: 42 };  // Constructor inlined
let result = data.value * 2;    // Method inlined
```

### Phase 3: Code Generation

The backend generates target-specific code:

```bash
# Code generation
cprimec codegen build/optimized/**/*.ir --target=x86_64-linux --output build/cpp/

# Target specifications:
# --target=x86_64-linux     Linux x86-64
# --target=x86_64-windows   Windows x86-64  
# --target=aarch64-linux    Linux ARM64
# --target=wasm32          WebAssembly
# --target=cpp-generic     Generic C++17
```

## Initial Target: C++ Code Generation

### Data Class Translation

```cpp
// CPrime source
class StreamData {
    handle: FileHandle,
    buffer: [u8; 4096],
    position: u64,
    constructed_by: StreamOps,
}

// Generated C++
struct StreamData {
    FileHandle handle;
    std::array<uint8_t, 4096> buffer;
    uint64_t position;
    
    // Move constructor (always public)
    StreamData(StreamData&& other) noexcept = default;
    
private:
    // Copy constructor (private by default)
    StreamData(const StreamData& other) = default;
    StreamData& operator=(const StreamData& other) = default;
    
    // Friends for stack containment access
    template<typename Container>
    friend class StackContainmentAccess;
    
    // Constructor access control
    friend class StreamOps;
};
```

### Functional Class Translation

```cpp
// CPrime source
functional class StreamOps {
    fn construct(path: &str) -> Result<StreamData> {
        let handle = os::open(path)?;
        Ok(StreamData { handle, buffer: [0; 4096], position: 0 })
    }
    
    fn read(data: &mut StreamData, buf: &mut [u8]) -> Result<usize> {
        os::read(data.handle, buf)
    }
}

// Generated C++
namespace StreamOps {
    inline Result<StreamData> construct(const std::string_view path) {
        auto handle = os::open(path);
        if (!handle) return handle.error();
        
        StreamData data;
        data.handle = std::move(*handle);
        data.buffer.fill(0);
        data.position = 0;
        return data;
    }
    
    inline Result<size_t> read(StreamData& data, std::span<uint8_t> buf) {
        return os::read(data.handle, buf);
    }
}
```

### Access Rights Implementation

```cpp
// CPrime source with access rights
class SecureData {
    key: [u8; 32],
    value: String,
    exposes UserOps { value }
    exposes AdminOps { key, value }
}

// Generated C++ with template friends
template<typename AccessRights>
struct SecureData {
    std::array<uint8_t, 32> key;
    std::string value;
    
    // Template-based access control
    template<typename AR = AccessRights>
    auto get_value() const -> 
        std::enable_if_t<std::is_same_v<AR, UserAccess> || 
                        std::is_same_v<AR, AdminAccess>, 
                        const std::string&> {
        return value;
    }
    
    template<typename AR = AccessRights>
    auto get_key() const -> 
        std::enable_if_t<std::is_same_v<AR, AdminAccess>, 
                        const std::array<uint8_t, 32>&> {
        return key;
    }
};

// Type aliases for different access levels
using UserSecureData = SecureData<UserAccess>;
using AdminSecureData = SecureData<AdminAccess>;
```

### Coroutine Translation

```cpp
// CPrime coroutine
fn async_process(data: &Data) -> Result<ProcessedData> {
    let intermediate = async_step_1(data).await?;
    let result = async_step_2(&intermediate).await?;
    Ok(result)
}

// Generated C++ with coroutines
cprime::Task<Result<ProcessedData>> async_process(const Data& data) {
    auto intermediate = co_await async_step_1(data);
    if (!intermediate) co_return intermediate.error();
    
    auto result = co_await async_step_2(*intermediate);
    if (!result) co_return result.error();
    
    co_return *result;
}
```

## Incremental Compilation

### Module-Based Compilation Units

```bash
# Compile only changed modules
cprimec frontend src/network/*.cp --incremental --cache=.cache/
cprimec frontend src/database/*.cp --incremental --cache=.cache/

# Dependency tracking
cprimec deps src/ --output deps.json

# Link compiled modules
cprimec link build/ir/*.ir --output build/app
```

### Caching Strategy

```
.cache/
├── frontend/          # Parsed AST and type info
│   ├── network.ast
│   ├── database.ast
│   └── utils.ast
├── ir/               # Generated IR
│   ├── network.ir
│   ├── database.ir
│   └── utils.ir
├── deps/             # Dependency information
│   └── module_deps.json
└── metadata/         # Compilation metadata
    └── timestamps.json
```

### Dependency Resolution

```cpp
// Module dependency tracking
{
    "network": {
        "depends_on": ["utils", "io"],
        "timestamp": "2024-01-15T10:30:00Z",
        "hash": "abc123def456"
    },
    "database": {
        "depends_on": ["network", "utils"],
        "timestamp": "2024-01-15T10:35:00Z", 
        "hash": "def456abc123"
    }
}
```

## Target Architectures

### Cross-Compilation Support

```bash
# Multiple target compilation
cprimec codegen build/ir/ \
    --target=x86_64-linux \
    --target=aarch64-linux \
    --target=x86_64-windows \
    --output=build/targets/

# Target-specific optimizations
cprimec codegen build/ir/ \
    --target=x86_64-linux \
    --cpu=native \
    --features=+avx2,+sse4.2 \
    --output=build/optimized/
```

### Platform-Specific Code Generation

```cpp
// Platform-specific implementations
#[cfg(target_arch = "x86_64")]
fn optimized_hash(data: &[u8]) -> u64 {
    // Use AVX2 instructions for hashing
}

#[cfg(target_arch = "aarch64")]
fn optimized_hash(data: &[u8]) -> u64 {
    // Use NEON instructions for hashing
}

// Generated C++ with conditional compilation
#ifdef __x86_64__
inline uint64_t optimized_hash(std::span<const uint8_t> data) {
    // AVX2 implementation
}
#elif defined(__aarch64__)
inline uint64_t optimized_hash(std::span<const uint8_t> data) {
    // NEON implementation  
}
#endif
```

## Build System Integration

### CMake Integration

```cmake
# CPrime CMake module
find_package(CPrime REQUIRED)

# Add CPrime library
cprime_add_library(mylib
    SOURCES
        src/network.cp
        src/database.cp
        src/utils.cp
    DEPENDENCIES
        system::filesystem
        external::sqlite3
    OPTIMIZATION_LEVEL 3
)

# Add CPrime executable
cprime_add_executable(myapp
    SOURCES
        src/main.cp
    DEPENDENCIES
        mylib
    TARGET x86_64-linux
)

# Generated CMake code
add_library(mylib_generated
    ${CMAKE_BINARY_DIR}/generated/network.cpp
    ${CMAKE_BINARY_DIR}/generated/database.cpp  
    ${CMAKE_BINARY_DIR}/generated/utils.cpp
)

target_link_libraries(mylib_generated 
    PRIVATE 
        cprime::runtime
        system::filesystem
        external::sqlite3
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/generated/network.cpp
    COMMAND cprimec frontend src/network.cp --emit-ir | 
            cprimec optimize --level=3 |
            cprimec codegen --target=x86_64-linux
    DEPENDS src/network.cp
)
```

### Package Manager Integration

```toml
# cprime.toml - Package configuration
[package]
name = "my-application"
version = "1.0.0"
edition = "2024"

[dependencies]
network = "2.1.0"
database = { version = "1.5", features = ["sqlite", "postgres"] }
utils = { path = "../shared-utils" }

[build]
optimization = 3
target = "x86_64-linux"
emit_debug_info = true

[features]
default = ["networking", "database"]
networking = ["network/tcp", "network/http"]
database = ["database/sqlite"]
```

## Debugging and Profiling Support

### Debug Information Generation

```bash
# Generate debug information
cprimec frontend src/ --debug-info --output build/debug/
cprimec codegen build/debug/ --debug-symbols --source-maps

# Generated debug mapping
{
    "source_line": 42,
    "source_file": "src/network.cp", 
    "generated_line": 156,
    "generated_file": "build/cpp/network.cpp",
    "function": "NetworkOps::connect"
}
```

### Profiling Integration

```cpp
// CPrime source with profiling
#[profile]
fn expensive_computation(data: &LargeData) -> Result {
    // Complex algorithm
}

// Generated C++ with profiling hooks
Result expensive_computation(const LargeData& data) {
    CPRIME_PROFILE_FUNCTION("expensive_computation");
    // Complex algorithm with profiling markers
}
```

## Error Handling and Diagnostics

### Comprehensive Error Messages

```
error[E0308]: mismatched access rights
  --> src/main.cp:23:5
   |
23 |     AdminOps::delete_user(&user_conn);
   |     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ expected `AdminConnection`, found `UserConnection`
   |
   = note: `AdminOps::delete_user` requires admin-level access rights
   = help: consider using `request_admin_access()` to escalate privileges
   = help: or use `UserOps::deactivate_user()` for user-level operations

error[E0405]: cannot find functional class `MissingOps` in this scope
  --> src/data.cp:15:20
   |
15 |     constructed_by: MissingOps,
   |                     ^^^^^^^^^^ not found in this scope
   |
   = help: you might have meant `DataOps`
   = note: consider importing with `use other_module::MissingOps`
```

### Compilation Performance

```bash
# Compilation timing
cprimec frontend src/ --timing
# Frontend: 1.2s (parsing: 0.3s, type-check: 0.9s)
# Optimization: 0.8s
# Code generation: 0.4s  
# Total: 2.4s

# Parallel compilation
cprimec frontend src/ --jobs=8 --timing
# Using 8 parallel jobs
# Total: 0.6s (4x speedup)
```

The compilation model provides fast incremental builds during development while enabling aggressive optimization for release builds, all while maintaining C++ compatibility for immediate adoption.