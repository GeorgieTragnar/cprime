# CPrime `semconst` Field Modifier

## Overview

`semconst` (semantic const) is a revolutionary field modifier in CPrime that separates **semantic immutability** from **memory immutability**. While traditional `const` prevents any modification at the memory level, `semconst` preserves the semantic value while allowing the compiler complete freedom to reorganize memory for optimization.

This fundamental innovation enables:
- **Location independence**: Values are preserved while memory can be moved, reallocated, or reorganized
- **Atomic replacement**: Fields can only be replaced in their entirety, never partially modified
- **Compiler optimizations**: Complete freedom for cache alignment, NUMA optimization, and memory compaction
- **Coroutine safety**: Natural handling of suspension points without breaking atomicity

## Core Concept: Semantic vs Memory Immutability

### Traditional Const (Memory Immutability)
```cpp
// Traditional const - memory location is fixed
const data: LargeStruct = initialize();
// Cannot modify data at all
// Cannot move data to different memory location
// Compiler cannot optimize memory layout
```

### Semantic Const (Value Preservation)
```cpp
// semconst - value preserved, memory flexible
class DataHolder {
    semconst data: LargeStruct,
    
    // Can only replace entirely via 1:1 move pattern
    fn update(&mut self, new_data: LargeStruct) {
        let temp = move(self.data);     // Must move out
        self.data = move(new_data);      // Must move in
        // Compiler verifies 1:1 pattern
    }
}

// Compiler can:
// - Relocate data for cache optimization
// - Reorganize memory layout
// - Move between NUMA nodes
// - Compact memory during GC
// All while preserving the semantic value
```

## The 1:1 Move Pattern

### Fundamental Rule

`semconst` fields enforce a strict 1:1 move pattern - you must move the old value out before moving a new value in:

```cpp
class Configuration {
    semconst settings: Settings,
    
    // ✓ CORRECT: 1:1 move pattern
    fn replace_settings(&mut self, new_settings: Settings) -> Settings {
        let old = move(self.settings);   // Move out
        self.settings = move(new_settings); // Move in
        old                               // Return old value
    }
    
    // ❌ INCORRECT: Direct mutation
    fn modify_setting(&mut self) {
        self.settings.timeout = 30;  // COMPILE ERROR: Cannot mutate semconst
    }
    
    // ❌ INCORRECT: Method calls that mutate
    fn update_setting(&mut self) {
        self.settings.update();       // COMPILE ERROR: Cannot call mutating methods
    }
}
```

### What This Prevents

1. **Partial modifications**: Cannot change individual fields or elements
2. **In-place mutations**: Cannot modify through methods or operators
3. **Direct field writes**: Cannot assign to subfields
4. **Incremental updates**: Cannot use operators like `+=` or `++`

### What This Enables

1. **Memory safety**: No partial states visible to other threads
2. **Optimization freedom**: Compiler can relocate without semantic impact
3. **Clear intent**: Explicitly shows fields that preserve semantic meaning
4. **Coroutine safety**: Atomic replacement works across suspension points

## Technical Implementation

### Swap Operations

The compiler recognizes swap patterns as permutations rather than mutations:

```cpp
class DataProcessor {
    semconst primary: Buffer,
    semconst secondary: Buffer,
    
    fn swap_buffers(&mut self) {
        // Recognized as permutation at bitwise level
        std::swap(self.primary, self.secondary);  // ✓ Allowed
        
        // Compiler sees this as:
        // 1. Atomically exchange memory contents
        // 2. No semantic values created or destroyed
        // 3. Just a reordering of existing values
    }
}
```

### Coroutine Integration

`semconst` naturally handles coroutine suspension points:

```cpp
suspend fn process_data(holder: &mut DataHolder) {
    // Build new state (can suspend during construction)
    let new_data = co_await fetch_and_process_data();
    
    // Atomic replacement (single operation)
    let old = move(holder.data);
    holder.data = move(new_data);
    
    // Other coroutines see either old or new, never partial
}
```

### Block-Level Semantics

The compiler verifies semantic preservation at block boundaries:

```cpp
fn complex_update(&mut self) {
    // Block begins - compiler notes semconst fields
    
    let temp1 = move(self.field1);
    let temp2 = move(self.field2);
    
    // Multiple operations allowed within block
    let processed1 = process(temp1);
    let processed2 = process(temp2);
    
    self.field1 = move(processed2);  // Swapped
    self.field2 = move(processed1);  // Swapped
    
    // Block ends - compiler verifies all semconst fields restored
}
```

## Two-Dimensional Access Control

### The Breakthrough

`semconst` introduces two independent permission axes:

1. **Data class perspective**: What the class can do with its own fields
2. **Interface perspective**: What external access is granted

### Permission Matrix

```cpp
class Database {
    semconst connections: [Connection; 10],  // Class can only move
    mutable cache: HashMap<String, Result>,   // Class can mutate freely
    
    // Expose semconst field as mutable to admin
    exposes AdminOps {
        connections as mutable,  // Admin gets full access
        cache as const,         // Admin gets read-only
    }
    
    // Expose semconst field as const to user
    exposes UserOps {
        connections as const,   // User gets read-only
        // cache not exposed    // User cannot access
    }
}

// Usage demonstrates two-dimensional control:
fn admin_task(db: &Database<AdminOps>) {
    AdminOps::modify_connection(&mut db.connections[0]);  // ✓ Can modify
    let cache_view = AdminOps::view_cache(&db.cache);     // ✓ Can read
}

fn user_task(db: &Database<UserOps>) {
    let conn = UserOps::get_connection(&db.connections[0]); // ✓ Can read
    // UserOps::modify_connection(&mut db.connections[0]);  // ❌ Cannot modify
    // let cache = UserOps::view_cache(&db.cache);          // ❌ Cannot access
}
```

### Interface Memory Contracts

`semconst` fields can participate in interface memory contracts:

```cpp
interface Identifiable {
    memory_contract {
        id: u64,            // Must be accessible
        version: semconst u32,  // Semantically preserved
    }
}

class Document {
    semconst doc_id: u64,      // Maps to id
    semconst doc_version: u32,  // Maps to version
    mutable content: String,
    
    implements Identifiable {
        id <- doc_id,
        version <- doc_version,
    }
}

// Generic operations respect semconst semantics
functional class IdentityOps<T: Identifiable> {
    fn get_identity(item: &T) -> (u64, u32) {
        // Direct memory access for compile-time interfaces
        (item.id, item.version)  // Both are semconst
    }
    
    fn update_version(item: &mut T, new_version: u32) {
        // Must use 1:1 move pattern for semconst field
        let old_version = move(item.version);
        item.version = move(new_version);
    }
}
```

## Use Cases and Patterns

### Configuration Management

```cpp
class AppConfig {
    semconst database_url: String,     // Changed atomically
    semconst port: u16,               // Changed atomically
    mutable connection_pool: Pool,     // Can be modified incrementally
    
    fn reload_config(&mut self, new_config: ConfigFile) {
        // Atomic replacement of configuration
        let old_url = move(self.database_url);
        let old_port = move(self.port);
        
        self.database_url = move(new_config.database_url);
        self.port = move(new_config.port);
        
        // Pool can be updated incrementally
        self.connection_pool.resize(new_config.pool_size);
    }
}
```

### Immutable Data Structures

```cpp
class ImmutableList<T> {
    semconst elements: Vec<T>,
    
    fn append(&self, item: T) -> ImmutableList<T> {
        let mut new_elements = self.elements.clone();
        new_elements.push(item);
        
        ImmutableList {
            elements: move(new_elements)  // New list with appended item
        }
    }
    
    fn update_at(&self, index: usize, item: T) -> ImmutableList<T> {
        let mut new_elements = self.elements.clone();
        new_elements[index] = item;
        
        ImmutableList {
            elements: move(new_elements)  // New list with updated item
        }
    }
}
```

### Cache-Friendly Data Layout

```cpp
class MatrixProcessor {
    // Compiler can reorganize for optimal cache line usage
    semconst matrix_a: [[f64; 1000]; 1000],
    semconst matrix_b: [[f64; 1000]; 1000],
    mutable result: [[f64; 1000]; 1000],
    
    fn compute_product(&mut self) {
        // Compiler may:
        // - Transpose matrix_b for better cache locality
        // - Align matrices to cache lines
        // - Split across NUMA nodes
        // All while preserving semantic values
        
        for i in 0..1000 {
            for j in 0..1000 {
                for k in 0..1000 {
                    self.result[i][j] += self.matrix_a[i][k] * self.matrix_b[k][j];
                }
            }
        }
    }
}
```

## Interaction with Other Language Features

### With Access Rights

```cpp
class SecureData {
    semconst encryption_key: [u8; 32],
    mutable data: Vec<u8>,
    
    exposes DecryptOps {
        encryption_key as const,  // Read-only access to semconst
        data as mutable,
    }
    
    exposes EncryptOps {
        encryption_key as const,  // Read-only access to semconst
        data as const,
    }
}
```

### With Unions

```cpp
union ConfigValue {
    String(semconst String),      // String variant is semconst
    Number(semconst f64),        // Number variant is semconst
    Boolean(mutable bool),       // Boolean variant is mutable
    
    constructed_by: ConfigManager,
}

functional class ConfigManager {
    fn update_string_value(value: &mut ConfigValue, new_str: String) {
        if let ConfigValue::String(ref mut s) = value {
            // Must use 1:1 move pattern
            let old = move(*s);
            *s = move(new_str);
        }
    }
}
```

### With Templates

```cpp
class Container<T> {
    semconst items: Vec<T>,
    
    // Generic operations must respect semconst
    fn replace_all(&mut self, new_items: Vec<T>) -> Vec<T> {
        let old = move(self.items);
        self.items = move(new_items);
        old
    }
}

// Specialization for specific types
impl Container<String> {
    fn replace_if_empty(&mut self, new_items: Vec<String>) {
        if self.items.is_empty() {
            let _ = move(self.items);
            self.items = move(new_items);
        }
    }
}
```

## Performance Benefits

### Compiler Optimizations

1. **Memory Layout Freedom**
   - Reorder fields for cache line alignment
   - Pack fields to minimize padding
   - Split hot/cold data automatically

2. **NUMA Optimization**
   - Move data closer to processing cores
   - Duplicate read-only semconst data across nodes
   - Minimize cross-node memory access

3. **Allocation Strategies**
   - Use arena allocators for semconst fields
   - Compact memory during idle periods
   - Coalesce small allocations

### Zero-Cost Abstraction

```cpp
// Function accepting semconst parameter
fn process(semconst data: LargeStruct) {
    // Compiler chooses optimal passing strategy:
    // - Pass by value for small structs
    // - Pass by reference for large structs
    // - Pass by register for primitive types
    // All without changing semantics
}

// Equivalent performance to manual optimization
fn process_manual_by_ref(data: &LargeStruct) { ... }
fn process_manual_by_val(data: LargeStruct) { ... }
```

## Best Practices

### When to Use `semconst`

1. **Configuration data**: Values that change atomically
2. **Immutable collections**: Data structures with value semantics
3. **Cache keys**: Data that identifies cached values
4. **Protocol buffers**: Structured data that's replaced wholesale
5. **State snapshots**: Complete state replacements in state machines

### When NOT to Use `semconst`

1. **Incrementally updated data**: Use `mutable` for gradual changes
2. **Performance counters**: Use `mutable` for frequent updates
3. **Stream buffers**: Use `mutable` for continuous data flow
4. **Temporary scratch space**: Use `mutable` for working memory

### Design Guidelines

```cpp
class WellDesigned {
    // Configuration - changes atomically
    semconst config: Config,
    
    // Identity - semantically fixed
    semconst id: Uuid,
    
    // Working state - changes incrementally
    mutable buffer: Vec<u8>,
    mutable stats: Statistics,
    
    // Good: Atomic config reload
    fn reload(&mut self, new_config: Config) {
        let old = move(self.config);
        self.config = move(new_config);
        self.buffer.clear();  // Mutable can be modified
    }
    
    // Good: Clear separation of concerns
    fn process(&mut self, data: &[u8]) {
        self.buffer.extend_from_slice(data);  // Modify mutable
        self.stats.bytes_processed += data.len();  // Modify mutable
        // self.config unchanged (semconst)
        // self.id unchanged (semconst)
    }
}
```

## Migration Guide

### From Traditional Const

```cpp
// Before: Traditional const
class OldStyle {
    const settings: Settings,
    
    // Cannot change settings at all
    fn new(settings: Settings) -> Self {
        OldStyle { settings }
    }
}

// After: Using semconst
class NewStyle {
    semconst settings: Settings,
    
    fn new(settings: Settings) -> Self {
        NewStyle { settings }
    }
    
    // Can now atomically replace settings
    fn update_settings(&mut self, new_settings: Settings) -> Settings {
        let old = move(self.settings);
        self.settings = move(new_settings);
        old
    }
}
```

### From Mutable

```cpp
// Before: Fully mutable
class OldStyle {
    mutable config: Config,
    
    // Could accidentally partially modify
    fn unsafe_update(&mut self) {
        self.config.field1 = value1;  // Partial update
        // Might crash here - inconsistent state!
        self.config.field2 = value2;  // Rest of update
    }
}

// After: Using semconst for atomicity
class NewStyle {
    semconst config: Config,
    
    // Forced atomic replacement
    fn safe_update(&mut self, new_config: Config) {
        let old = move(self.config);
        self.config = move(new_config);  // All-or-nothing
    }
}
```

## Conclusion

`semconst` represents a fundamental innovation in programming language design, being the first language feature to explicitly separate semantic immutability from memory immutability. This separation enables:

1. **Safety**: Atomic value replacement prevents partial states
2. **Performance**: Complete compiler freedom for optimization
3. **Clarity**: Explicit declaration of semantic intent
4. **Composability**: Works naturally with CPrime's other features

By providing fine-grained control over field mutability semantics while maintaining memory flexibility, `semconst` enables patterns that are impossible or expensive in other languages, making it a cornerstone feature of CPrime's design philosophy.