# CPrime Memory Management

## Overview

CPrime adopts C++-style memory management with manual control, RAII patterns, and the possibility of dangling references. This provides maximum performance and predictability while maintaining familiar semantics for C++ programmers.

## Core Memory Model

### C++ Semantics (Not Rust)

CPrime explicitly chooses C++ memory semantics over Rust's borrow checker approach:

```cpp
// References can dangle - programmer responsibility
let x = Point::new(1, 2);
let r = &x;
drop(x);              // Explicit destruction
// r is now dangling - undefined behavior if used

// Manual memory management
let ptr = malloc(1024);
defer free(ptr);      // Ensures cleanup at scope exit

// RAII patterns work as expected
class File {
    handle: FileHandle,
    
    fn drop(&mut self) {
        os::close(self.handle);  // Automatic cleanup
    }
}
```

### Key Principles

1. **Manual memory management**: Programmer controls allocation and deallocation
2. **No borrow checker**: References don't prevent modification or destruction
3. **RAII encouraged**: Resource Acquisition Is Initialization for automatic cleanup
4. **Predictable performance**: No hidden allocations or garbage collection
5. **Explicit lifetimes**: Object lifetimes are clear from code structure

## Memory Operations

### Basic Memory Operations

Every data class supports four fundamental memory operations:

```cpp
class Example {
    data: Vec<u8>,
    id: u32,
    
    // 1. Move - always public (bit relocation)
    fn move(self) -> Self;
    
    // 2. Copy - private by default
    default fn copy(&self) -> Self;
    
    // 3. Assign - private by default  
    default fn assign(&mut self, other: &Self);
    
    // 4. Drop - automatic cleanup
    fn drop(&mut self);
}
```

#### Move Semantics

Move is always public because it's a pure bit operation with no semantic meaning:

```cpp
let x = Example::new();
let y = move x;        // x is no longer valid
// Equivalent to memcpy + invalidate source
```

#### Copy Semantics

Copy is private by default but accessible through stack containment:

```cpp
class Container {
    // Stack containment grants copy access
    data: Example,
    
    fn duplicate(&self) -> Self {
        Container {
            data: self.data.copy(),  // ✓ OK - stack contained
        }
    }
}

// Cannot copy through pointer
let ptr: Box<Example> = Box::new(Example::new());
// let copy = ptr.copy();  // ❌ ERROR - no stack containment
```

#### Custom Memory Operations

Data classes can override default memory operations:

```cpp
class DeepBuffer {
    ptr: *mut u8,
    len: usize,
    capacity: usize,
    
    // Custom copy - deep copy the buffer
    private fn copy(&self) -> Self {
        let new_ptr = malloc(self.capacity);
        memcpy(new_ptr, self.ptr, self.len);
        DeepBuffer {
            ptr: new_ptr,
            len: self.len,
            capacity: self.capacity,
        }
    }
    
    // Custom drop - free the buffer
    fn drop(&mut self) {
        if !self.ptr.is_null() {
            free(self.ptr);
        }
    }
}
```

## Stack Containment Rule

### The Core Principle

Stack containment = ownership = memory management rights

```cpp
class Container {
    // Stack instance: gets copy/assign access
    data: SecureData,
    
    // Heap pointer: NO special access
    ptr: Box<SecureData>,
    
    fn grow(&mut self) {
        // Can call private copy on stack-contained data
        let new_data = self.data.copy();  // ✓ OK
        
        // Cannot call private copy through pointer
        // let copy = self.ptr.copy();     // ❌ ERROR
        
        // But can copy the container of the pointer
        let boxed_copy = Box::new(self.ptr.as_ref().copy()); // ✓ OK if accessible
    }
}
```

### Why Stack Containment Matters

1. **Clear ownership**: Stack containment indicates responsibility for cleanup
2. **Performance optimization**: Stack-contained objects can be optimized aggressively
3. **Memory safety**: Prevents accidental sharing of private memory operations
4. **Intuitive model**: Matches programmer expectations about object ownership

### Examples of Stack Containment

```cpp
// Stack containment - full memory access
class DirectContainer {
    value: SecureData,           // Stack contained
    array: [SecureData; 10],     // All elements stack contained
    tuple: (SecureData, i32),    // First element stack contained
}

// No stack containment - limited access
class IndirectContainer {
    ptr: *mut SecureData,        // Pointer - no containment
    boxed: Box<SecureData>,      // Heap allocated - no containment
    reference: &SecureData,      // Reference - no containment
}
```

## Semantic Preservation with `semconst`

### Memory Location Independence

CPrime introduces `semconst` (semantic const) - a revolutionary field modifier that separates semantic immutability from memory immutability:

```cpp
class DataHolder {
    semconst config: Configuration,  // Semantic value preserved
    mutable cache: HashMap,          // Can modify freely
    
    // semconst allows memory reorganization while preserving value
    fn update(&mut self, new_config: Configuration) {
        // 1:1 move pattern enforced by compiler
        let old = move(self.config);     // Move out old value
        self.config = move(new_config);  // Move in new value
        
        // Compiler can optimize memory layout:
        // - Relocate config for cache alignment
        // - Move to different NUMA node
        // - Compact during garbage collection
        // All while preserving the semantic value
    }
}
```

### How `semconst` Enables Compiler Optimizations

#### 1. Cache Line Optimization

```cpp
class MatrixData {
    semconst matrix_a: [[f64; 100]; 100],
    semconst matrix_b: [[f64; 100]; 100],
    mutable result: [[f64; 100]; 100],
}

// Compiler can:
// - Align matrices to cache line boundaries
// - Transpose matrix_b for better locality
// - Split across multiple cache levels
// Without changing semantic values
```

#### 2. NUMA-Aware Memory Placement

```cpp
fn process_parallel(semconst data: LargeDataSet) {
    // Compiler decides optimal memory placement:
    // - Duplicate read-only portions across NUMA nodes
    // - Place near processing cores
    // - Move between nodes as needed
    // All transparent to the programmer
}
```

#### 3. Zero-Copy Optimizations

```cpp
class SharedBuffer {
    semconst data: Vec<u8>,
    
    fn share_with_thread(&self) -> SharedView {
        // Compiler can share memory directly
        // No copying needed - semconst guarantees no mutation
        SharedView { data: &self.data }
    }
}
```

### Relationship with Memory Operations

`semconst` fields interact with standard memory operations in specific ways:

```cpp
class Example {
    semconst immutable_data: Data,
    mutable working_data: Data,
    
    // Move - always allowed (bit relocation)
    fn relocate(self) -> Self {
        move(self)  // ✓ Works for all fields
    }
    
    // Copy - follows standard rules
    private fn copy(&self) -> Self {
        Example {
            immutable_data: self.immutable_data.copy(),  // ✓ Copy semconst
            working_data: self.working_data.copy(),      // ✓ Copy mutable
        }
    }
    
    // Modification - restricted for semconst
    fn modify(&mut self) {
        // self.immutable_data.mutate();  // ❌ Cannot mutate semconst
        self.working_data.mutate();       // ✓ Can mutate mutable
        
        // Must use 1:1 move for semconst
        let old = move(self.immutable_data);
        self.immutable_data = move(process(old));  // ✓ Atomic replacement
    }
}
```

### Memory Safety with `semconst`

The `semconst` modifier provides additional memory safety guarantees:

1. **No Partial States**: Atomic replacement prevents inconsistent states
2. **Thread Safety**: Other threads see either old or new value, never partial
3. **Coroutine Safety**: Suspension points don't break atomicity

```cpp
suspend fn async_update(holder: &mut DataHolder) {
    // Build new state (can suspend)
    let new_data = co_await fetch_data();
    let processed = co_await process_data(new_data);
    
    // Atomic replacement (cannot suspend)
    let old = move(holder.data);
    holder.data = move(processed);
    // Other coroutines see complete transition
}
```

### Performance Characteristics

| Aspect | Traditional Const | `semconst` | Mutable |
|--------|------------------|-----------|---------|
| **Memory Layout** | Fixed | Flexible | Flexible |
| **Compiler Optimization** | Limited | Maximum | Moderate |
| **Modification** | None | Atomic only | Any |
| **Thread Safety** | Implicit | Atomic transitions | Requires sync |
| **Cache Optimization** | Manual | Automatic | Manual |

For comprehensive documentation on `semconst`, see [semconst.md](semconst.md).

## RAII and Resource Management

### Automatic Resource Management

RAII (Resource Acquisition Is Initialization) is the primary resource management pattern:

```cpp
class ManagedFile {
    handle: FileHandle,
    temp_buffer: *mut u8,
    
    fn construct(path: &str) -> Result<Self> {
        let handle = os::open(path)?;
        let buffer = malloc(4096);
        Ok(ManagedFile { handle, temp_buffer: buffer })
    }
    
    // Automatic cleanup
    fn drop(&mut self) {
        os::close(self.handle);
        free(self.temp_buffer);
    }
}

// Usage - automatic cleanup when scope ends
{
    let file = ManagedFile::construct("data.txt")?;
    // ... use file
} // file.drop() called automatically here
```

### Defer Statement

For resources that don't fit the RAII pattern:

```cpp
fn process_file(path: &str) -> Result<Data> {
    let file = File::open(path)?;
    defer file;                    // File closed at function exit
    
    let buffer = allocate_buffer();
    defer free_buffer(buffer);     // Buffer freed at function exit
    
    // Multiple exit points - all execute defers
    if some_condition {
        return Err("condition failed");  // Defers execute
    }
    
    let data = file.read_async().await;
    process(data)                        // Defers execute
}
```

#### Defer Execution Order

Defers execute in reverse order (LIFO - Last In, First Out):

```cpp
fn complex_cleanup() {
    defer println("First");
    defer println("Second"); 
    defer println("Third");
    
    // Output:
    // Third
    // Second  
    // First
}
```

## Smart Pointers

### Compiler-Provided Smart Pointers

CPrime provides smart pointers with special compiler support:

```cpp
// Unique ownership
let unique: Box<T> = Box::new(value);
let moved = unique;           // Move semantics
// unique is no longer valid

// Shared ownership with reference counting
let shared: Rc<T> = Rc::new(value);
let copy = shared.clone();    // Reference count increment
// Both shared and copy are valid

// Thread-safe shared ownership
let atomic: Arc<T> = Arc::new(value);
spawn {
    let local = atomic.clone();
    // Use local in another thread
}
```

### Custom Smart Pointers

You can implement custom smart pointers as danger classes:

```cpp
#[danger(stateful_class, raw_pointers)]
class CustomPtr<T> {
    ptr: *mut T,
    ref_count: *mut usize,
    
    fn new(value: T) -> Self {
        let ptr = malloc(size_of::<T>());
        let ref_count = malloc(size_of::<usize>());
        
        unsafe {
            ptr::write(ptr, value);
            ptr::write(ref_count, 1);
        }
        
        CustomPtr { ptr, ref_count }
    }
    
    fn clone(&self) -> Self {
        unsafe {
            *self.ref_count += 1;
        }
        CustomPtr { ptr: self.ptr, ref_count: self.ref_count }
    }
    
    fn drop(&mut self) {
        unsafe {
            *self.ref_count -= 1;
            if *self.ref_count == 0 {
                ptr::drop_in_place(self.ptr);
                free(self.ptr as *mut u8);
                free(self.ref_count as *mut u8);
            }
        }
    }
}
```

## Memory Allocation Strategies

### Stack Allocation (Default)

Most objects are allocated on the stack for maximum performance:

```cpp
let value = MyStruct::new();     // Stack allocated
let array = [0u8; 1024];         // Stack allocated array
```

### Heap Allocation (Explicit)

Heap allocation is explicit and obvious:

```cpp
let boxed = Box::new(MyStruct::new());    // Heap allocated
let vec = Vec::with_capacity(1000);       // Heap allocated storage
```

### Custom Allocators

For specialized allocation patterns:

```cpp
// Arena allocator for temporary objects
let arena = Arena::new(4096);
defer arena.reset();

let temp1 = arena.alloc(MyStruct::new());
let temp2 = arena.alloc(AnotherStruct::new());
// All arena objects freed when arena is reset

// Pool allocator for fixed-size objects
static POOL: ObjectPool<Connection> = ObjectPool::new();

let conn = POOL.acquire();       // Reuse existing or allocate new
defer POOL.release(conn);        // Return to pool
```

## Memory Safety Guidelines

### Safe Patterns

1. **Prefer RAII**: Use constructors/destructors for automatic cleanup
2. **Use defer for cleanup**: Ensures resources are released
3. **Stack containment**: Keep objects on stack when possible
4. **Smart pointers**: Use Box, Rc, Arc for heap-allocated objects

### Dangerous Patterns

1. **Raw pointers**: Only in danger classes with explicit marking
2. **Manual malloc/free**: Wrap in RAII classes when possible
3. **Dangling references**: Check lifetime manually
4. **Double free**: Use smart pointers or clear ownership

### Migration from C++

CPrime makes C++ migration straightforward:

```cpp
// C++ code
class LegacyClass {
    int* data;
public:
    LegacyClass() : data(new int[100]) {}
    ~LegacyClass() { delete[] data; }
    void process() { /* ... */ }
};

// Direct CPrime translation (danger class)
#[danger(stateful_class)]
class LegacyClass {
    data: *mut i32,
    
    fn new() -> Self {
        LegacyClass { data: malloc(100 * sizeof(i32)) }
    }
    
    fn drop(&mut self) {
        free(self.data);
    }
    
    fn process(&mut self) { /* ... */ }
}

// Better CPrime design (separate data/operations)
class ProcessorData {
    data: Box<[i32; 100]>,
    constructed_by: Processor,
}

functional class Processor {
    fn construct() -> ProcessorData {
        ProcessorData { data: Box::new([0; 100]) }
    }
    
    fn process(data: &mut ProcessorData) { /* ... */ }
}
```

## Performance Characteristics

### Zero-Cost Abstractions

- **RAII**: No runtime overhead - destructors inlined
- **Smart pointers**: Optimized to single indirection when possible
- **Stack containment**: Compile-time access control
- **Move semantics**: Pure bit copy, no hidden allocations

### Predictable Performance

- **No garbage collection**: Deterministic destruction timing
- **No hidden allocations**: All heap usage is explicit
- **Cache-friendly**: Data layout optimized for access patterns
- **Inline-friendly**: Functional classes encourage inlining

The memory management model provides C++ performance with additional safety guardrails through the three-class system and access rights.