# Interface Memory Contracts: Polymorphic Glue for N:M Composition

## Overview

CPrime interfaces act as **polymorphic glue** between RAII state holders (data classes) and RAII state modifiers (functional classes). They serve as constructs for exact memory contracts and function signature contracts, enabling abstractable work on different types using the same function calls.

This dual-purpose design enables powerful N:M composition where multiple data classes can work with multiple access rights through shared interface contracts, while maintaining CPrime's zero-cost abstraction principles.

Unlike traditional interfaces that only define function signatures, CPrime interfaces create a bridge mechanism through **memory access patterns** that data classes link to their internal fields, enabling generic functional classes to work across different data types with predictable performance characteristics.

## Core Concepts

### Interface as Memory Copy Bitmap

Interfaces act as polymorphic glue by defining a "bitmap" pattern for memory operations - they specify which chunks of memory should be accessible and in what pattern, enabling abstractable work on different data class types using the same function calls, while the RAII state holder (data class) controls the actual layout:

```cpp
interface Cacheable {
    memory_contract {
        id: u64,           // 8 bytes at offset 0
        timestamp: u64,    // 8 bytes at offset 8
        void[32],          // 32 bytes at offset 16-47 (skip region)
        hash: u32,         // 4 bytes at offset 48
        // Total contract size: 52 bytes
    }
    
    fn cache_key(&self) -> CacheKey;
    fn invalidate(&mut self);
}
```

The interface defines both:
1. **Memory Access Pattern**: Which fields exist and their types
2. **Functional Contract**: Which operations must be implemented

### Data Class Controlled Layout with Linking

Data classes maintain complete control over their memory layout and explicitly link their fields to interface contracts:

```cpp
class UserData {
    user_id: u64,        // offset 0
    created: u64,        // offset 8  
    name: String,        // offset 16 (fits in void[32])
    email: String,       // offset 40 (fits in void[32])
    status_hash: u32,    // offset 48
    permissions: Vec<Permission>,  // After interface contract
    
    implements Cacheable {
        id <- user_id,           // Direct memory link
        timestamp <- created,    // Direct memory link
        hash <- status_hash,     // Direct memory link
        void[32] <- memory_region(16, 32),  // Explicit skip region
    }
}
```

The `<-` operator creates explicit field linking between interface expectations and data class fields.

## Compile-Time vs Runtime Interfaces

### Single Declaration Principle

Each interface can be declared as **either** compile-time or runtime, but **never both**:

```cpp
// Option A: Compile-time interface (zero-cost, inflexible)
interface Cacheable {
    memory_contract { ... }
    fn cache_key(&self) -> CacheKey;
}

// Option B: Runtime interface (flexible, accessor cost)
runtime interface FlexibleCacheable {
    data_contract { ... }  // Same fields, different access method
    fn cache_key(&self) -> CacheKey;
}

// FORBIDDEN: Cannot have both versions of same interface name
```

This prevents function explosion and maintains uniform access patterns within each interface type.

### Compile-Time Interfaces: Zero-Cost Path

Compile-time interfaces require exact memory layout compliance and provide direct memory access:

```cpp
interface Serializable {
    memory_contract {
        type_id: u32,       // Must be at offset 0
        version: u16,       // Must be at offset 4
        void[10],          // Exactly 10 bytes at offset 6-15
        data_size: u32,     // Must be at offset 16
    }
    
    fn serialize(&self) -> Vec<u8>;
}

// MUST match exact layout
class DocumentData {
    doc_type: u32,      // Maps to type_id at offset 0
    format_version: u16, // Maps to version at offset 4
    metadata: [u8; 10], // Fits exactly in void[10]
    content_length: u32, // Maps to data_size at offset 16
    content: String,    // After interface contract
    
    implements Serializable {
        type_id <- doc_type,
        version <- format_version,
        void[10] <- memory_region(&metadata, 10),
        data_size <- content_length,
    }
}

// Compile ERROR if layout doesn't match exactly
```

### Runtime Interfaces: Flexible Access

Runtime interfaces use accessor methods and allow any memory layout:

```cpp
runtime interface FlexibleSerializable {
    data_contract {
        type_id: u32,       // Must be accessible, any layout
        version: u16,       // Must be accessible, any layout  
        data_size: u32,     // Must be accessible, any layout
    }
    
    fn serialize(&self) -> Vec<u8>;
}

class FlexibleDocument {
    title: String,          // Any layout
    doc_type: u32,         // Anywhere in memory
    content: Vec<u8>,       // Any layout
    format_version: u16,    // Anywhere in memory
    
    implements FlexibleSerializable {
        type_id <- doc_type,        // Generates accessor method
        version <- format_version,  // Generates accessor method
        data_size <- content.len() as u32,  // Computed accessor
    }
    
    // Compiler generates:
    // fn _flexible_serializable_type_id(&self) -> u32 { self.doc_type }
    // fn _flexible_serializable_version(&self) -> u16 { self.format_version }
    // fn _flexible_serializable_data_size(&self) -> u32 { self.content.len() as u32 }
}
```

## Void Regions and Memory Layout

### Explicit Void Sizing

Void regions must be explicitly sized to prevent ambiguity:

```cpp
interface NetworkPacket {
    memory_contract {
        header_type: u32,
        void[16],                    // Exactly 16 bytes
        void[sizeof(IpAddress)],     // Size of specific type  
        payload_length: u32,
        void[payload_length],        // Variable size (runtime determined)
        checksum: u32,
    }
}
```

### Void Region Usage

Void regions serve multiple purposes:

1. **Skip Regions**: Skip over implementation-specific data
2. **Alignment Padding**: Ensure proper field alignment
3. **Variable Data**: Handle variable-length content
4. **Future Extension**: Reserve space for interface evolution

```cpp
class TcpPacket {
    packet_type: u32,
    src_addr: IpAddress,     // 16 bytes
    dest_addr: IpAddress,    // 16 bytes
    length: u32,
    payload: Vec<u8>,        // Variable size
    crc32: u32,
    
    implements NetworkPacket {
        header_type <- packet_type,
        void[16] <- memory_region(&src_addr, 16),
        void[sizeof(IpAddress)] <- memory_region(&dest_addr, 16),
        payload_length <- length,
        void[payload_length] <- payload.as_slice(),  // Runtime-sized region
        checksum <- crc32,
    }
}
```

## N:M Composition Through Interfaces

### Multiple Data Classes, Single Interface

Multiple data classes can implement the same interface contract:

```cpp
interface Auditable {
    memory_contract {
        entity_id: u64,
        user_id: u64,
        timestamp: u64,
        action_hash: u32,
    }
    
    fn audit_log(&self) -> AuditEntry;
}

// Different data classes implementing same interface
class UserAction {
    action_id: u64,      // Maps to entity_id
    user: u64,           // Maps to user_id
    performed_at: u64,   // Maps to timestamp  
    hash: u32,           // Maps to action_hash
    action_data: ActionPayload,
    
    implements Auditable {
        entity_id <- action_id,
        user_id <- user,
        timestamp <- performed_at,
        action_hash <- hash,
    }
}

class DocumentEdit {
    doc_id: u64,         // Maps to entity_id
    editor: u64,         // Maps to user_id
    modified: u64,       // Maps to timestamp
    content_hash: u32,   // Maps to action_hash
    changes: Vec<Edit>,
    
    implements Auditable {
        entity_id <- doc_id,
        user_id <- editor,
        timestamp <- modified,
        action_hash <- content_hash,
    }
}
```

### Templated Functional Classes

Single functional class works with multiple data types through interface contracts:

```cpp
functional class AuditOps<T: Auditable> {
    fn log_access(data: &T) -> Result<()> {
        // Same memory access pattern for ALL T implementing Auditable
        let entry = AuditEntry {
            entity: data.entity_id,    // Direct access through interface
            user: data.user_id,        // Direct access through interface  
            time: data.timestamp,      // Direct access through interface
            signature: data.action_hash, // Direct access through interface
        };
        
        audit_system::record(entry)
    }
    
    fn validate_access(data: &T, requesting_user: UserId) -> bool {
        // Generic validation logic works for UserAction, DocumentEdit, etc.
        data.user_id == requesting_user || 
        security::has_admin_access(requesting_user)
    }
}

// Single implementation works with all Auditable types
let user_action = UserAction { ... };
let doc_edit = DocumentEdit { ... };

AuditOps::log_access(&user_action)?;   // Same code path
AuditOps::log_access(&doc_edit)?;      // Same code path
```

### Single Data Class, Multiple Interfaces

A data class can implement multiple interface contracts:

```cpp
class OrderData {
    order_id: u64,
    customer_id: u64,
    created_at: u64,
    total_amount: Money,
    status_hash: u32,
    items: Vec<OrderItem>,
    
    // Implements multiple interfaces
    implements Cacheable {
        id <- order_id,
        timestamp <- created_at,
        hash <- status_hash,
        void[32] <- memory_region(&total_amount, sizeof(Money)),
    }
    
    implements Auditable {
        entity_id <- order_id,
        user_id <- customer_id,
        timestamp <- created_at,
        action_hash <- status_hash,
    }
    
    implements Billable {
        transaction_id <- order_id,
        amount <- total_amount,
        customer <- customer_id,
    }
}

// Can be used with multiple functional class types
let order = OrderData { ... };

CacheOps::store(&order)?;           // Through Cacheable interface
AuditOps::log_access(&order)?;      // Through Auditable interface
BillingOps::process_payment(&order)?; // Through Billable interface
```

## Performance Implications

### Zero-Cost Compile-Time Path

When using compile-time interfaces with proper alignment:

```cpp
functional class FastCacheOps<T: Cacheable> {  // Compile-time interface
    fn get_cache_key(data: &T) -> u64 {
        // Direct memory access - zero overhead
        unsafe { 
            let id = *(data as *const _ as *const u64);
            let timestamp = *(data as *const _ as *const u64).offset(1);
            id ^ timestamp
        }
    }
}
```

### Runtime Accessor Cost

When using runtime interfaces:

```cpp
functional class FlexibleCacheOps<T: FlexibleCacheable> {  // Runtime interface
    fn get_cache_key(data: &T) -> u64 {
        // Accessor method calls - small performance cost
        let id = data._flexible_cacheable_id();
        let timestamp = data._flexible_cacheable_timestamp();
        id ^ timestamp
    }
}
```

### Performance Comparison

| Interface Type | Memory Access | Function Calls | Optimization | Use Case |
|----------------|---------------|----------------|--------------|----------|
| Compile-time | Direct memory | Zero overhead | Full inline | High-performance, fixed layout |
| Runtime | Accessors | Method call cost | Limited | Flexible layout, dynamic composition |

## Best Practices

### Choose Interface Type Based on Use Case

```cpp
// High-performance, stable layout
interface CriticalPath {
    memory_contract { ... }
    // Used in hot loops, performance critical
}

// Flexible composition, evolving requirements  
runtime interface AdaptableSystem {
    data_contract { ... }
    // Used in plugin systems, varying implementations
}
```

### Minimize Interface Contract Size

```cpp
// Good: Focused interface
interface Identifiable {
    memory_contract {
        id: u64,
        hash: u32,
    }
}

// Avoid: Overly complex interface
interface Overloaded {
    memory_contract {
        id: u64,
        timestamp: u64,
        user_id: u64,
        session_id: u64,
        metadata: [u8; 64],
        // ... many more fields
    }
}
```

### Use Void Regions Judiciously

```cpp
interface ExtensibleFormat {
    memory_contract {
        version: u32,
        core_data: u64,
        void[32],          // Room for version-specific extensions
        checksum: u32,
    }
}
```

### Design for Evolution

```cpp
// Version 1: Simple interface
interface DataProcessorV1 {
    memory_contract {
        data_id: u64,
        process_flags: u32,
    }
}

// Version 2: Extended interface (new name)
interface DataProcessorV2 {
    memory_contract {
        data_id: u64,
        process_flags: u32,
        priority: u16,
        metadata_size: u16,
        void[metadata_size],  // Variable metadata
    }
}
```

## Integration with CPrime Systems

### Channel System Integration

```cpp
runtime interface ChannelMessage {
    data_contract {
        message_id: u64,
        sender_id: u64,
        message_type: u32,
    }
}

// Multiple message types implement same interface
class UserMessage implements ChannelMessage { ... }
class SystemAlert implements ChannelMessage { ... }
class DataUpdate implements ChannelMessage { ... }

// Generic channel operations
functional class ChannelOps<T: ChannelMessage> {
    fn route_message(msg: &T, channel: &Channel<T>) -> Result<()> {
        match msg.message_type {
            USER_MSG => user_channel.send(msg),
            SYSTEM_MSG => system_channel.send(msg),
            DATA_MSG => data_channel.send(msg),
        }
    }
}
```

### Coroutine System Integration

```cpp
interface CoroutineTask {
    memory_contract {
        task_id: u64,
        priority: u32,
        state_hash: u32,
    }
}

// Different coroutine types implement same interface
class IoTask implements CoroutineTask { ... }
class ComputeTask implements CoroutineTask { ... }
class NetworkTask implements CoroutineTask { ... }

// Generic scheduler operations
functional class SchedulerOps<T: CoroutineTask> {
    fn schedule_task(task: &T, scheduler: &TaskScheduler) -> Result<()> {
        scheduler.enqueue_by_priority(task.priority, task.task_id)
    }
}
```

Interface memory contracts revolutionize CPrime's compositional capabilities, enabling true N:M composition between data classes and access rights while maintaining the language's zero-cost abstraction principles. The explicit choice between compile-time performance and runtime flexibility gives developers full control over the performance characteristics of their abstractions.