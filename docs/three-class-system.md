# CPrime Three-Class System

## Overview

The three-class system is CPrime's fundamental architectural innovation that replaces traditional inheritance with a clear separation of concerns:

- **Data Classes**: RAII state holders
- **Functional Classes**: RAII state modifiers  
- **Danger Classes**: Full C++ semantics for interop

Each class type serves a specific purpose with specific constraints, enforcing good design patterns at the language level while maintaining the RAII principle throughout.

## The Three Class Types

### 1. Data Classes - RAII State Holders

Data classes are RAII state holders that contain only state (fields) and basic memory management operations. They cannot have semantic methods or business logic, focusing purely on holding state safely through RAII principles.

#### Syntax and Structure

```cpp
class StreamData {
    // Only state fields allowed
    handle: FileHandle,
    buffer: [u8; 4096],
    position: u64,
    
    // Declares who can construct this type
    constructed_by: FileStream,
    
    // Memory operations only (no semantic operations)
    fn move(self) -> Self;              // Always public
    default fn copy(&self) -> Self;     // Bit-copy by default
    default fn assign(&mut self, &Self); // Bit-assign by default
    
    // Custom memory management allowed
    private fn copy(&self) -> Self {
        // Must be memory-only (e.g., deep copy buffer)
        // No business logic allowed
        StreamData {
            handle: self.handle,
            buffer: self.buffer.clone(),  // Deep copy
            position: self.position,
        }
    }
}
```

#### Key Rules for Data Classes

1. **Only state fields allowed**: No methods except memory management
2. **Memory operations only**: `move`, `copy`, `assign`, `drop` - no semantic behavior
3. **Constructor access control**: `constructed_by` declares which functional classes can create instances
4. **Move is always public**: Pure bit relocation has no semantic meaning
5. **Default memory operations**: Bit-copy and bit-assign unless overridden
6. **Stack containment privilege**: Types that contain this data class on the stack get access to private memory operations
7. **Unions are data classes**: Unions follow all data class rules with special variant handling

#### Field Modifiers in Data Classes

Data classes support a three-tier field modifier system that provides fine-grained control over mutability and access:

##### 1. `semconst` Fields - Semantic Preservation

`semconst` (semantic const) fields preserve semantic value while allowing memory flexibility:

```cpp
class Configuration {
    semconst database_url: String,  // Atomic replacement only
    semconst port: u16,            // No partial modification
    mutable cache: HashMap,         // Can modify incrementally
    
    // semconst fields enforce 1:1 move pattern
    fn update_config(&mut self, new_url: String, new_port: u16) {
        // Must move out old values
        let old_url = move(self.database_url);
        let old_port = move(self.port);
        
        // Move in new values
        self.database_url = move(new_url);
        self.port = move(new_port);
        
        // Cache can be modified normally
        self.cache.clear();
    }
}
```

**Key characteristics of `semconst`:**
- **Atomic replacement**: Can only be replaced entirely, never partially modified
- **1:1 move pattern**: Must move old value out before moving new value in
- **Memory flexibility**: Compiler can relocate/reorganize for optimization
- **No method calls**: Cannot call mutating methods on `semconst` fields
- **Swap recognition**: Compiler recognizes swaps as permutations, not mutations

For comprehensive documentation on `semconst`, see [semconst.md](semconst.md).

##### 2. `mutable` Fields - Full Mutability

`mutable` fields can be modified freely by the owning class:

```cpp
class StreamBuffer {
    mutable buffer: Vec<u8>,        // Can modify incrementally
    mutable position: usize,        // Can update directly
    semconst capacity: usize,       // Can only replace atomically
    
    fn write(&mut self, data: &[u8]) {
        self.buffer.extend_from_slice(data);  // ✓ Modify mutable field
        self.position += data.len();          // ✓ Update mutable field
        // self.capacity += 100;               // ❌ Cannot modify semconst
    }
}
```

##### 3. Default (Unspecified) Fields

Fields without explicit modifiers default to private mutability with controlled exposure:

```cpp
class UserData {
    id: u64,                    // Default: private mutable
    name: String,               // Default: private mutable
    semconst created_at: u64,  // Explicitly semconst
    mutable cache: Cache,       // Explicitly mutable
    
    // Default fields can be modified internally
    fn rename(&mut self, new_name: String) {
        self.name = new_name;  // ✓ Can modify
    }
}
```

##### Field Modifiers and Access Rights

Field modifiers interact with access rights to provide two-dimensional access control:

```cpp
class Database {
    semconst connection_string: String,  // Class can only replace atomically
    mutable connection_pool: Pool,       // Class can modify freely
    cache: HashMap<String, Result>,      // Default: private mutable
    
    // Expose fields with different permissions
    exposes AdminOps {
        connection_string as mutable,  // Admin can modify semconst field!
        connection_pool as mutable,    // Admin can modify pool
        cache as const,               // Admin can only read cache
    }
    
    exposes UserOps {
        connection_string as const,   // User can only read
        // connection_pool not exposed // User cannot access pool
        cache as const,               // User can only read cache
    }
}

// Two-dimensional control in action:
functional class AdminOps {
    fn update_connection(db: &mut Database<AdminOps>, new_conn: String) {
        // Admin can modify even semconst fields through access rights
        db.connection_string = new_conn;  // ✓ Allowed via access right
    }
}

functional class UserOps {
    fn get_connection(db: &Database<UserOps>) -> &String {
        &db.connection_string  // ✓ Read-only access
    }
    
    // fn modify_connection(db: &mut Database<UserOps>, new: String) {
    //     db.connection_string = new;  // ❌ COMPILE ERROR - no write access
    // }
}
```

This three-tier field system complements CPrime's class architecture by:
1. **Enforcing discipline**: `semconst` prevents accidental partial mutations
2. **Enabling optimization**: Compiler has freedom with `semconst` fields
3. **Providing flexibility**: Different access levels through access rights
4. **Maintaining safety**: Clear semantics for each field modifier

#### Memory Operation Semantics

- **Move**: Always public, performs bit-wise relocation
- **Copy**: Private by default, accessible to stack containers
- **Assign**: Private by default, accessible to stack containers  
- **Drop**: Called automatically at end of scope (RAII)

#### Union Constructs (Memory Space Management)

While not classes themselves, unions are **memory space constructs** that work with the three-class system. They reserve memory space for different object types but never exist as objects themselves:

```cpp
union ConnectionSpace {
    UserConn(Connection<UserOps>),      // 40 bytes
    AdminConn(Connection<AdminOps>),    // 48 bytes
    PowerConn(Connection<UserOps, AdminOps>), // 56 bytes
    
    // Memory contract: Reserve 56 bytes (largest variant)
    constructed_by: ConnectionManager,
}

// You work with objects IN the union space, not with "the union"
functional class ConnectionManager {
    fn construct_user_in_space(space: &mut ConnectionSpace, creds: &UserCreds) -> &Connection<UserOps> {
        // Construct Connection<UserOps> object in the reserved space
        let user_conn = Connection::new_user(creds);
        unsafe { ptr::write(space as *mut Connection<UserOps>, user_conn) };
        unsafe { &*(space as *const Connection<UserOps>) }
    }
    
    fn process_connection(space: &ConnectionSpace) {
        // Access actual objects living in union space
        if let Some(user) = space.try_as::<Connection<UserOps>>() {
            UserOps::query(&user, "SELECT ...");  // Normal object method
        } else if let Some(admin) = space.try_as::<Connection<AdminOps>>() {
            AdminOps::delete_table(&admin, "temp");  // Normal object method
        }
    }
}
```

**Union Construct Characteristics:**
- **Memory space constructs**: Reserve space for objects, with polymorphic tagging for identification
- **Not objects**: You never work with "the union", only objects in union-managed space
- **Two variants**: Compile-time (fixed size) or runtime (growable with resize overhead)
- **Polymorphic tagging**: Runtime unions use tagging (vtables/type info) for type identification
- **Integration**: Work seamlessly with the three-class system

**Union Constructs vs Functional Classes (Access Rights):**
- **Union Constructs**: Memory space management for different objects with polymorphic tagging
- **Functional Classes (Access Rights)**: Same object with different vtable views, casting required

For comprehensive union documentation, see [unions.md](unions.md).

### 2. Functional Classes - RAII State Modifiers

Functional classes are RAII state modifiers that contain only stateless operations. They cannot have member variables and serve as the exclusive means to modify data class state while maintaining RAII safety. They act as namespaces for related state modification functionality.

#### Syntax and Structure

```cpp
functional class FileStream {
    // ❌ COMPILE ERROR: No state allowed
    // cache: HashMap<String, Data>  
    
    // Constructor creates paired data class
    fn construct(path: &str) -> Result<StreamData> {
        let handle = os::open(path)?;
        StreamData { 
            handle, 
            buffer: [0; 4096], 
            position: 0 
        }
    }
    
    // Destructor cleans up paired data class
    fn destruct(data: &mut StreamData) {
        os::close(data.handle);
    }
    
    // Semantic operations on data
    fn read(data: &mut StreamData, buf: &mut [u8]) -> Result<usize> {
        // Implementation can modify data.position, data.buffer
        let bytes_read = os::read(data.handle, buf)?;
        data.position += bytes_read as u64;
        Ok(bytes_read)
    }
    
    fn write(data: &mut StreamData, buf: &[u8]) -> Result<usize> {
        let bytes_written = os::write(data.handle, buf)?;
        data.position += bytes_written as u64;
        Ok(bytes_written)
    }
}

// Functional classes can also operate on unions
functional class ConnectionManager {
    fn construct() -> ConnectionState {
        ConnectionState::Disconnected
    }
    
    fn connect(state: &mut ConnectionState, addr: SocketAddr) -> Result<()> {
        match state {
            ConnectionState::Disconnected => {
                *state = ConnectionState::Connecting { attempt: 1 };
                Ok(())
            },
            ConnectionState::Error { .. } => {
                *state = ConnectionState::Connecting { attempt: 1 };
                Ok(())
            },
            _ => Err("Invalid state for connection"),
        }
    }
    
    fn handle_connection_result(
        state: &mut ConnectionState, 
        result: Result<TcpStream>
    ) {
        match (state, result) {
            (ConnectionState::Connecting { attempt }, Ok(socket)) => {
                let peer = socket.peer_addr()?;
                *state = ConnectionState::Connected { socket, peer };
            },
            (ConnectionState::Connecting { attempt }, Err(e)) => {
                if *attempt < MAX_RETRIES {
                    *attempt += 1;
                } else {
                    *state = ConnectionState::Error {
                        code: ErrorCode::ConnectionFailed,
                        message: e.to_string(),
                    };
                }
            },
            _ => {},
        }
    }
}

// Without constructor = implicitly a module
functional class StringOps {
    fn split(s: &str, delimiter: char) -> Vec<&str> {
        // Implementation
    }
    
    fn join(parts: &[&str], separator: &str) -> String {
        // Implementation  
    }
}
```

#### Key Rules for Functional Classes

1. **No member variables**: Enforced at compile time
2. **Optional constructor/destructor**: For managing paired data classes or objects in union constructs
3. **Without constructor = module**: Pure utility functions
4. **All operations are static**: No `self` parameter except in constructor/destructor
5. **Stateless by design**: Cannot maintain state between calls
6. **Can operate on union constructs**: Pattern matching on objects in union space is allowed

#### Constructor/Destructor Semantics

- **Constructor**: Returns instance of paired data class or object in union construct
- **Destructor**: Cleans up data class/object instance (called automatically) 
- **Pairing**: One functional class can manage one data class or objects in union construct
- **No constructor**: Functional class becomes a pure module

### 3. Danger Classes - Full C++ Semantics

Danger classes allow traditional OOP patterns with both state and methods. They must be explicitly marked as dangerous and are used for FFI wrappers, C++ compatibility, and gradual migration.

#### Syntax and Structure

```cpp
#[danger(stateful_class)]
class LegacyDatabase {
    // Can have state AND methods
    connection: *mut c_void,
    cache: HashMap<String, QueryResult>,
    is_connected: bool,
    
    // Constructor with state initialization
    fn new(connection_string: &str) -> Result<Self> {
        LegacyDatabase {
            connection: null_mut(),
            cache: HashMap::new(),
            is_connected: false,
        }
    }
    
    // Methods can access and modify state
    fn connect(&mut self) -> Result<()> {
        if self.is_connected {
            return Ok(());
        }
        
        self.connection = unsafe { 
            ffi::db_connect(connection_string.as_ptr()) 
        };
        
        if self.connection.is_null() {
            Err("Connection failed")
        } else {
            self.is_connected = true;
            Ok(())
        }
    }
    
    fn query(&mut self, sql: &str) -> Result<QueryResult> {
        // Check cache first
        if let Some(cached) = self.cache.get(sql) {
            return Ok(cached.clone());
        }
        
        // Execute query and cache result
        let result = unsafe {
            ffi::db_query(self.connection, sql.as_ptr())
        };
        
        self.cache.insert(sql.to_string(), result.clone());
        Ok(result)
    }
    
    // Destructor
    fn drop(&mut self) {
        if self.is_connected {
            unsafe { ffi::db_close(self.connection) };
        }
    }
}
```

#### Key Rules for Danger Classes

1. **Must be explicitly marked**: `#[danger(stateful_class)]` attribute required
2. **Full C++ semantics**: Can have both state and methods
3. **Used for interop**: FFI wrappers, C++ compatibility, legacy code
4. **Can be wrapped**: Safe functional classes can wrap danger classes
5. **Explicit opt-in**: Makes dangerous code clearly visible

#### Common Danger Attributes

- `#[danger(stateful_class)]`: Traditional OOP class
- `#[danger(ffi)]`: Foreign function interface
- `#[danger(raw_pointers)]`: Raw pointer manipulation
- `#[danger(thread_bound)]`: Pinned to specific OS thread
- `#[danger(unchecked_access)]`: Bypass access control

## Class Interactions

### Data + Functional Pairing

The most common pattern is pairing a data class with a functional class:

```cpp
// Data class holds state
class FileData {
    handle: FileHandle,
    path: String,
    mode: OpenMode,
    constructed_by: FileOps,
}

// Functional class provides operations
functional class FileOps {
    fn construct(path: &str, mode: OpenMode) -> Result<FileData> {
        let handle = os::open(path, mode)?;
        Ok(FileData { handle, path: path.to_string(), mode })
    }
    
    fn destruct(data: &mut FileData) {
        os::close(data.handle);
    }
    
    fn read(data: &mut FileData, buffer: &mut [u8]) -> Result<usize> {
        os::read(data.handle, buffer)
    }
}

// Usage
let file = FileOps::construct("data.txt", OpenMode::Read)?;
defer FileOps::destruct(&mut file);

let mut buffer = [0u8; 1024];
let bytes_read = FileOps::read(&mut file, &mut buffer)?;
```

### Wrapping Danger Classes

Safe classes can wrap danger classes to provide safer interfaces:

```cpp
// Danger class for low-level operations
#[danger(ffi, raw_pointers)]
class UnsafeSocket {
    fd: c_int,
    // ... unsafe implementation
}

// Safe wrapper
class SocketData {
    inner: UnsafeSocket,
    local_addr: SocketAddr,
    peer_addr: Option<SocketAddr>,
    constructed_by: SafeSocket,
}

functional class SafeSocket {
    fn construct(addr: SocketAddr) -> Result<SocketData> {
        let inner = UnsafeSocket::new(addr)?;
        Ok(SocketData {
            inner,
            local_addr: addr,
            peer_addr: None,
        })
    }
    
    fn connect(data: &mut SocketData, peer: SocketAddr) -> Result<()> {
        data.inner.connect(peer)?;
        data.peer_addr = Some(peer);
        Ok(())
    }
}
```

## Coroutine Integration with Three-Class System

CPrime coroutines are implemented using the three-class system, demonstrating the power and flexibility of the architectural pattern:

### Coroutines as Data Classes

Coroutine state follows the data class pattern - pure state with controlled construction:

```cpp
// Coroutine state as data class
class HttpHandlerCoro {
    // Stack and execution state (pure data)
    stack_memory: *mut u8,
    stack_size: usize,
    stack_top: *mut u8,
    register_context: RegisterContext,
    
    // Application-specific state
    request: HttpRequest,
    response: HttpResponse,
    processing_stage: ProcessingStage,
    
    // Pool allocation metadata
    size_tag: CoroSizeTag,
    pool_id: Option<PoolId>,
    
    // Construction control
    constructed_by: HttpCoroManager,
    
    // Memory operations follow data class rules
    fn move(self) -> Self;  // Always public
    private fn copy(&self) -> Self {
        // Custom copy for coroutine state
        HttpHandlerCoro {
            stack_memory: clone_stack_memory(self.stack_memory, self.stack_size),
            stack_size: self.stack_size,
            stack_top: self.stack_memory.add(self.stack_size),
            register_context: self.register_context.clone(),
            request: self.request.clone(),
            response: self.response.clone(),
            processing_stage: self.processing_stage,
            size_tag: self.size_tag,
            pool_id: self.pool_id,
        }
    }
}
```

### Functional Classes for Coroutine Operations

All coroutine operations are implemented as stateless functional classes:

```cpp
functional class HttpCoroManager {
    // Constructor allocates coroutine with compiler-guided size analysis
    fn construct(request: HttpRequest) -> HttpHandlerCoro {
        let size_hint = compiler_analyze_size::<HttpHandlerCoro>();
        let stack = allocate_stack_for_size_class(size_hint);
        
        HttpHandlerCoro {
            stack_memory: stack.ptr,
            stack_size: stack.size,
            stack_top: stack.ptr.add(stack.size),
            register_context: RegisterContext::new(),
            request,
            response: HttpResponse::empty(),
            processing_stage: ProcessingStage::Initial,
            size_tag: size_hint,
            pool_id: stack.pool_id,
        }
    }
    
    // Destructor handles cleanup
    fn destruct(coro: &mut HttpHandlerCoro) {
        // Free stack memory based on allocation source
        match coro.pool_id {
            Some(pool_id) => deallocate_from_pool(coro.stack_memory, pool_id),
            None => deallocate_individual(coro.stack_memory, coro.stack_size),
        }
        
        // Clean up application state
        HttpRequest::destruct(&mut coro.request);
        HttpResponse::destruct(&mut coro.response);
    }
    
    // Semantic operations
    fn resume(coro: &mut HttpHandlerCoro) -> CoroResult<HttpResponse> {
        // Context switch to coroutine stack
        let result = context_switch_to(
            coro.stack_memory,
            coro.register_context,
            &mut coro.processing_stage
        );
        
        match result {
            CoroState::Yielded => CoroResult::Suspended,
            CoroState::Completed(response) => {
                coro.response = response;
                CoroResult::Complete(coro.response.clone())
            },
            CoroState::Error(e) => CoroResult::Error(e),
        }
    }
    
    fn suspend_at(coro: &mut HttpHandlerCoro, suspension_point: SuspensionPoint) {
        // Save current register state
        save_register_context(&mut coro.register_context);
        coro.processing_stage = ProcessingStage::Suspended(suspension_point);
    }
}

// Specialized functional classes for different coroutine sizes
functional class MicroCoroManager {
    // No constructor - acts as pure module for micro coroutine operations
    fn allocate_from_pool(size_hint: CoroSizeTag) -> Option<*mut u8> {
        if size_hint == CoroSizeTag::Micro {
            MICRO_POOL_MANAGER.allocate()
        } else {
            None
        }
    }
    
    fn deallocate_to_pool(ptr: *mut u8) {
        MICRO_POOL_MANAGER.deallocate(ptr);
    }
    
    fn resume_micro(stack_ptr: *mut u8, context: &RegisterContext) -> CoroResult {
        // Optimized resume for micro coroutines
        micro_context_switch(stack_ptr, context)
    }
}
```

### Coroutine Size Classes as Union Data Classes

Coroutine storage uses CPrime's union system for efficient heterogeneous containers:

```cpp
// Union data class for different coroutine sizes
union CoroStorage {
    Micro(MicroCoro<256>),      // Stack-allocated pool storage
    Small(SmallCoro<2048>),     // Heap pool storage
    Medium(MediumCoro<16384>),  // Heap pool storage
    Large(LargeCoro),           // Individual heap allocation
    
    constructed_by: CoroStorageManager,
}

// Functional class managing union-based storage
functional class CoroStorageManager {
    fn construct_for_size(size_class: CoroSizeTag) -> CoroStorage {
        match size_class {
            CoroSizeTag::Micro => {
                let micro_coro = MicroCoroManager::allocate_from_pool()?;
                CoroStorage::Micro(micro_coro)
            },
            CoroSizeTag::Small => {
                let small_coro = SmallCoroManager::allocate_from_pool()?;
                CoroStorage::Small(small_coro)
            },
            CoroSizeTag::Medium => {
                let medium_coro = MediumCoroManager::allocate_from_pool()?;
                CoroStorage::Medium(medium_coro)
            },
            CoroSizeTag::Large => {
                let large_coro = LargeCoroManager::allocate_individual()?;
                CoroStorage::Large(large_coro)
            },
        }
    }
    
    fn resume_any(storage: &mut CoroStorage) -> CoroResult {
        match storage {
            CoroStorage::Micro(micro) => MicroCoroManager::resume_micro(micro),
            CoroStorage::Small(small) => SmallCoroManager::resume_small(small),
            CoroStorage::Medium(medium) => MediumCoroManager::resume_medium(medium),
            CoroStorage::Large(large) => LargeCoroManager::resume_large(large),
        }
    }
    
    fn migrate_to_larger_class(storage: &mut CoroStorage) -> Result<()> {
        // Pattern matching to handle migration between size classes
        let new_storage = match storage {
            CoroStorage::Micro(micro) => {
                let migrated = migrate_micro_to_small(micro)?;
                CoroStorage::Small(migrated)
            },
            CoroStorage::Small(small) => {
                let migrated = migrate_small_to_medium(small)?;
                CoroStorage::Medium(migrated)
            },
            CoroStorage::Medium(medium) => {
                let migrated = migrate_medium_to_large(medium)?;
                CoroStorage::Large(migrated)
            },
            CoroStorage::Large(_) => return Err("Already at largest size class"),
        };
        
        *storage = new_storage;
        Ok(())
    }
}
```

### Three-Class Pattern in Coroutine Pools

Even the memory pool system follows the three-class pattern:

```cpp
// Pool state as data class
class MicroCoroPool {
    // Pure state - memory layout and allocation tracking
    pool_memory: &'static mut [u8; 256 * 1000],  // 256KB for 1000 micro coroutines
    allocation_bitmap: [u64; 16],                // Track 1000 slots with 16 u64s
    free_count: usize,
    total_allocations: u64,
    
    constructed_by: MicroPoolManager,
    
    // Data class memory operations
    fn move(self) -> Self;
    private fn copy(&self) -> Self {
        // Pool copying requires special handling
        MicroCoroPool {
            pool_memory: clone_pool_memory(self.pool_memory),
            allocation_bitmap: self.allocation_bitmap,
            free_count: self.free_count,
            total_allocations: self.total_allocations,
        }
    }
}

// Pool operations as functional class
functional class MicroPoolManager {
    fn construct(pool_memory: &'static mut [u8]) -> MicroCoroPool {
        MicroCoroPool {
            pool_memory,
            allocation_bitmap: [0; 16],  // All slots initially free
            free_count: 1000,
            total_allocations: 0,
        }
    }
    
    fn destruct(pool: &mut MicroCoroPool) {
        // Ensure all coroutines are deallocated
        if pool.free_count != 1000 {
            panic!("Destroying pool with active coroutines");
        }
        
        // Pool memory is static, no deallocation needed
    }
    
    fn allocate(pool: &mut MicroCoroPool) -> Option<*mut u8> {
        if pool.free_count == 0 {
            return None;
        }
        
        // Find first free slot using bit scanning
        let slot_idx = find_first_zero_bit(&pool.allocation_bitmap);
        set_bit(&mut pool.allocation_bitmap, slot_idx);
        pool.free_count -= 1;
        pool.total_allocations += 1;
        
        Some(pool.pool_memory.as_mut_ptr().add(slot_idx * 256))
    }
    
    fn deallocate(pool: &mut MicroCoroPool, ptr: *mut u8) {
        let offset = ptr.offset_from(pool.pool_memory.as_ptr()) as usize;
        let slot_idx = offset / 256;
        
        clear_bit(&mut pool.allocation_bitmap, slot_idx);
        pool.free_count += 1;
    }
    
    fn get_stats(pool: &MicroCoroPool) -> PoolStats {
        PoolStats {
            total_slots: 1000,
            free_slots: pool.free_count,
            used_slots: 1000 - pool.free_count,
            total_allocations: pool.total_allocations,
            utilization: (1000 - pool.free_count) as f64 / 1000.0,
        }
    }
}

// Usage follows three-class pattern
let mut micro_pool = MicroPoolManager::construct(&mut STATIC_POOL_MEMORY);
defer MicroPoolManager::destruct(&mut micro_pool);

let coro_ptr = MicroPoolManager::allocate(&mut micro_pool)?;
// ... use coroutine ...
MicroPoolManager::deallocate(&mut micro_pool, coro_ptr);
```

### Compiler-Library Protocol as Functional Classes

Even the compiler-library cooperation follows the three-class system:

```cpp
// Metadata as data class (generated by compiler)
class CoroMetadata {
    function_id: u64,
    estimated_stack_size: usize,
    max_stack_size: Option<usize>,
    is_bounded: bool,
    size_class: CoroSizeTag,
    migration_likelihood: f32,
    
    // No constructed_by - this is compiler-generated data
}

// Metadata registry as functional class (no constructor = module)
functional class MetadataRegistry {
    fn lookup_metadata(function_id: u64) -> Option<&'static CoroMetadata> {
        COROUTINE_METADATA_TABLE.iter()
            .find(|meta| meta.function_id == function_id)
    }
    
    fn get_size_hint<F>() -> CoroSizeTag 
    where F: CoroFunction
    {
        let function_id = F::function_id();
        Self::lookup_metadata(function_id)
            .map(|meta| meta.size_class)
            .unwrap_or(CoroSizeTag::Medium)  // Conservative default
    }
    
    fn validate_prediction(function_id: u64, actual_size: usize) -> bool {
        if let Some(metadata) = Self::lookup_metadata(function_id) {
            match metadata.max_stack_size {
                Some(max_size) => actual_size <= max_size,
                None => true,  // Unbounded - always valid
            }
        } else {
            false  // No metadata found
        }
    }
}
```

### Benefits of Three-Class Coroutine Architecture

1. **Clear Separation**: Coroutine state (data), operations (functional), and unsafe interop (danger) are clearly separated
2. **Memory Safety**: Construction control ensures proper initialization and cleanup
3. **Performance**: Functional classes enable aggressive compiler optimizations
4. **Testability**: Stateless operations are easily unit tested
5. **Composability**: Pool managers, allocators, and schedulers all follow the same pattern

This architecture enables CPrime's revolutionary coroutine performance (100x memory density, 50-100 cycle context switches) while maintaining safety and composability.

## Polymorphism: Interfaces as Polymorphic Glue

The three-class system works with CPrime's four-tier polymorphism approach, with interfaces acting as polymorphic glue between RAII state holders and RAII state modifiers:

### 1. Access Rights (Inheritance-like)
Data classes can expose access rights for inheritance-like behavior:

```cpp
class Connection {
    handle: DbHandle,
    
    exposes UserOps { handle }
    exposes AdminOps { handle }
    constructed_by: ConnectionFactory,
}

functional class UserOps {
    fn query(conn: &Connection, sql: &str) -> Result<QueryResult> {
        // Limited operations
    }
}

functional class AdminOps {
    fn query(conn: &Connection, sql: &str) -> Result<QueryResult> {
        // Full operations
    }
    
    fn delete_table(conn: &Connection, table: &str) -> Result<()> {
        // Admin-only operations
    }
}
```

### 2. Interfaces
Data classes and unions can implement interfaces:

```cpp
interface Drawable {
    fn draw(&self);
}

impl Drawable for ShapeData {
    fn draw(&self) {
        DrawOps::render(self);
    }
}

impl Drawable for ShapeUnion {  
    fn draw(&self) {
        match self {
            ShapeUnion::Circle(c) => CircleOps::draw(c),
            ShapeUnion::Rectangle(r) => RectangleOps::draw(r),
        }
    }
}
```

### 3. Interface Memory Contracts (Polymorphic Glue for N:M Composition)
Interfaces act as polymorphic glue through memory contracts, enabling N:M composition where multiple RAII state holders work with multiple RAII state modifiers through shared interface bindings, allowing abstractable work on different types using the same function calls:

```cpp
// Interface defining memory access pattern
interface Cacheable {
    memory_contract {
        id: u64,           // 8 bytes at offset 0
        timestamp: u64,    // 8 bytes at offset 8
        void[32],          // Skip region at offset 16-47
        hash: u32,         // 4 bytes at offset 48
    }
    
    fn cache_key(&self) -> CacheKey;
}

// Multiple data classes implement same interface contract
class UserData {
    user_id: u64,        // Maps to id
    created: u64,        // Maps to timestamp
    name: String,        // Fits in void[32] region
    status_hash: u32,    // Maps to hash
    permissions: Vec<Permission>,  // After interface contract
    
    implements Cacheable {
        id <- user_id,
        timestamp <- created,
        hash <- status_hash,
        void[32] <- memory_region(16, 32),
    }
}

class ProductData {
    product_id: u64,     // Maps to id
    modified: u64,       // Maps to timestamp
    description: String, // Fits in void[32] region
    content_hash: u32,   // Maps to hash
    metadata: ProductMeta, // After interface contract
    
    implements Cacheable {
        id <- product_id,
        timestamp <- modified,
        hash <- content_hash,
        void[32] <- memory_region(16, 32),
    }
}

// Single templated functional class works with all Cacheable implementations
functional class CacheOps<T: Cacheable> {
    fn store_in_cache(data: &T) -> CacheResult {
        // Direct memory access through interface contract (zero-cost)
        let key = data.id ^ data.timestamp;  // Same for all T
        let entry = CacheEntry {
            key,
            hash: data.hash,
            timestamp: data.timestamp,
        };
        
        cache_system::insert(entry)
    }
    
    fn validate_cache_entry(data: &T) -> bool {
        // Generic validation logic works for UserData, ProductData, etc.
        data.timestamp > 0 && data.hash != 0
    }
}

// Usage: Single implementation works with multiple data types
let user = UserData { ... };
let product = ProductData { ... };

CacheOps::store_in_cache(&user)?;     // Same code path
CacheOps::store_in_cache(&product)?;  // Same code path
```

This enables powerful N:M composition where:
- **Multiple data classes** (UserData, ProductData, etc.) can implement the same interface
- **Multiple functional classes** can work generically across interface implementations
- **Zero-cost abstractions** through compile-time interfaces with direct memory access
- **Flexible abstractions** through runtime interfaces with accessor methods

### 4. Unions (Memory Contracts)
Unions are data classes that provide memory reservation for different object types:

```cpp
union runtime MessageSpace {
    Text(runtime TextMessage),
    Binary(runtime BinaryMessage),
    Control(runtime ControlMessage),
}

functional class MessageProcessor {
    fn handle(space: &MessageSpace) {
        // Work with actual objects in union space, not "the union"
        if let Some(text) = space.try_as::<TextMessage>() {
            text.process();  // Direct object method call via vtable
        } else if let Some(binary) = space.try_as::<BinaryMessage>() {
            binary.decode();  // Direct object method call via vtable
        } else if let Some(control) = space.try_as::<ControlMessage>() {
            control.execute();  // Direct object method call via vtable
        }
    }
}
```

## Interface Memory Contracts in the Three-Class System

### Compile-Time Interface Contracts
Compile-time interfaces require exact memory layout compliance and provide zero-cost access:

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

// Data class with matching memory layout
class DocumentData {
    doc_type: u32,      // Offset 0 - maps to type_id
    format_version: u16, // Offset 4 - maps to version
    metadata: [u8; 10], // Offset 6 - fits in void[10]
    content_length: u32, // Offset 16 - maps to data_size
    content: String,    // After interface contract
    
    implements Serializable {
        type_id <- doc_type,
        version <- format_version,
        void[10] <- memory_region(&metadata, 10),
        data_size <- content_length,
    }
}

// Zero-cost functional class operations
functional class SerializationOps<T: Serializable> {
    fn get_type_info(data: &T) -> (u32, u16) {
        // Direct memory access - zero overhead
        unsafe {
            let type_id = *(data as *const _ as *const u32);
            let version = *(data as *const _ as *const u16).offset(1);
            (type_id, version)
        }
    }
}
```

### Runtime Interface Contracts
Runtime interfaces allow flexible memory layouts with accessor methods:

```cpp
runtime interface FlexibleSerializable {
    data_contract {
        type_id: u32,       // Must be accessible, any layout
        version: u16,       // Must be accessible, any layout
        data_size: u32,     // Must be accessible, any layout
    }
    
    fn serialize(&self) -> Vec<u8>;
}

// Data class with any memory layout
class FlexibleDocument {
    title: String,          // Any offset
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

// Runtime functional class operations
functional class FlexibleSerializationOps<T: FlexibleSerializable> {
    fn get_type_info(data: &T) -> (u32, u16) {
        // Accessor method calls - small performance cost
        let type_id = data._flexible_serializable_type_id();
        let version = data._flexible_serializable_version();
        (type_id, version)
    }
}
```

### Benefits of Interface Memory Contracts in Three-Class System

1. **Architectural Consistency**: Interface contracts maintain the separation between data (memory layout), operations (functional classes), and capabilities
2. **N:M Composition**: Multiple data classes can work with multiple functional classes through shared interface contracts
3. **Performance Control**: Choose between zero-cost compile-time contracts and flexible runtime contracts
4. **Type Safety**: Compiler verifies interface implementations and field mappings
5. **Evolution Path**: Interface versioning through new interface names enables gradual migration

The interface memory contract system revolutionizes CPrime's compositional capabilities while maintaining the three-class system's architectural clarity and performance characteristics.

## Benefits of the Three-Class System

### 1. Architectural Clarity
- **Clear separation of concerns**: State, operations, and dangerous code are distinct
- **Explicit dependencies**: Constructor access control makes dependencies visible
- **No hidden complexity**: Method resolution is always clear
- **Flexible polymorphism**: Choose between access rights, interfaces, and unions

### 2. Compiler Optimizations
- **Data layout optimization**: Data classes can be packed efficiently
- **Function inlining**: Functional class methods are natural inline candidates
- **Dead code elimination**: Unused functional classes can be removed entirely

### 3. Memory Safety
- **Controlled construction**: Only authorized functional classes can create data
- **Stack containment privileges**: Clear rules for memory operation access
- **RAII enforcement**: Destructors ensure proper cleanup

### 4. Gradual Migration
- **C++ compatibility**: Danger classes provide direct interop
- **Progressive safety**: Can gradually replace danger classes with safe alternatives
- **Mixed codebases**: Safe and dangerous code can coexist

### 5. Testing and Verification
- **Testable operations**: Functional classes are easy to unit test
- **Mockable interfaces**: Data classes can be easily mocked
- **Isolated danger**: Dangerous code is contained and identifiable

## Design Patterns with Three Classes

### Factory Pattern
```cpp
functional class ConnectionFactory {
    fn create_user_connection() -> UserConnectionData { ... }
    fn create_admin_connection() -> AdminConnectionData { ... }
}
```

### Builder Pattern
```cpp
functional class HttpRequestBuilder {
    fn new() -> HttpRequestData { ... }
    fn with_header(req: &mut HttpRequestData, key: &str, value: &str) { ... }
    fn with_body(req: &mut HttpRequestData, body: Vec<u8>) { ... }
    fn build(req: HttpRequestData) -> HttpRequest { ... }
}
```

### Adapter Pattern
```cpp
functional class DatabaseAdapter {
    fn from_mysql(mysql_data: &MySqlData) -> GenericDbData { ... }
    fn from_postgres(pg_data: &PostgresData) -> GenericDbData { ... }
}
```

## Channel Examples with Three-Class System

### Channel as Data Class

Channels fit perfectly into the three-class system as data classes holding communication state:

```cpp
// Channel data class holds communication state
class MessageChannel<T> {
    // Internal channel state (pure data)
    buffer: CircularBuffer<T>,
    capacity: usize,
    closed: AtomicBool,
    
    // Synchronization state
    send_waiters: WaitQueue,
    recv_waiters: WaitQueue,
    
    // Reference counting for safety
    ref_count: AtomicUsize,
    
    // Access rights for channel capabilities
    exposes SendOps { buffer, send_waiters, closed }
    exposes RecvOps { buffer, recv_waiters, closed }
    
    // Construction control
    constructed_by: MessageChannelOps<T>,
}
```

### Channel Operations as Functional Class

All channel operations are implemented as stateless functional classes:

```cpp
// Channel operations as functional class (stateless)
functional class MessageChannelOps<T> {
    // Constructor creates channel data
    fn construct(capacity: usize) -> MessageChannel<T> {
        MessageChannel {
            buffer: CircularBuffer::with_capacity(capacity),
            capacity,
            closed: AtomicBool::new(false),
            send_waiters: WaitQueue::new(),
            recv_waiters: WaitQueue::new(),
            ref_count: AtomicUsize::new(1),
        }
    }
    
    // Destructor handles cleanup
    fn destruct(channel: &mut MessageChannel<T>) {
        // Mark as closed and wake all waiters
        channel.closed.store(true, Ordering::Release);
        Self::wake_all_waiters(channel);
    }
    
    // Utility operations
    fn wake_all_waiters(channel: &MessageChannel<T>) {
        // Wake all suspended coroutines
        while let Some(waiter) = channel.send_waiters.pop() {
            SchedulerOps::wake_coroutine(waiter);
        }
        while let Some(waiter) = channel.recv_waiters.pop() {
            SchedulerOps::wake_coroutine(waiter);
        }
    }
    
    fn is_closed(channel: &MessageChannel<T>) -> bool {
        channel.closed.load(Ordering::Acquire)
    }
}

// Send operations as functional class
functional class SendOps<T> {
    suspend fn send(channel: &MessageChannel<T>, value: T) -> SendResult {
        if MessageChannelOps::is_closed(channel) {
            return SendResult::Closed;
        }
        
        // Try immediate send
        if let Some(receiver) = channel.recv_waiters.pop() {
            // Direct handoff to waiting receiver
            SchedulerOps::wake_with_value(receiver, value);
            return SendResult::Ok;
        }
        
        // Try buffer
        if channel.buffer.try_push(value) {
            return SendResult::Ok;
        }
        
        // Must suspend - add to wait queue
        let current_coro = SchedulerOps::current_coroutine_id();
        channel.send_waiters.push(current_coro);
        
        // Suspend until space available or channel closed
        co_await SchedulerOps::suspend();
        
        // Resume here when woken
        if MessageChannelOps::is_closed(channel) {
            SendResult::Closed
        } else {
            // Space should be available now
            channel.buffer.push(value);
            SendResult::Ok
        }
    }
}

// Receive operations as functional class
functional class RecvOps<T> {
    suspend fn recv(channel: &MessageChannel<T>) -> Option<T> {
        // Try immediate receive from buffer
        if let Some(value) = channel.buffer.try_pop() {
            // Wake a waiting sender if any
            if let Some(sender) = channel.send_waiters.pop() {
                SchedulerOps::wake_coroutine(sender);
            }
            return Some(value);
        }
        
        // Check if closed and empty
        if MessageChannelOps::is_closed(channel) {
            return None;
        }
        
        // Must suspend - add to wait queue
        let current_coro = SchedulerOps::current_coroutine_id();
        channel.recv_waiters.push(current_coro);
        
        // Suspend until item available or channel closed
        co_await SchedulerOps::suspend();
        
        // Resume here when woken
        if let Some(value) = channel.buffer.try_pop() {
            // Wake a sender if buffer has space now
            if let Some(sender) = channel.send_waiters.pop() {
                SchedulerOps::wake_coroutine(sender);
            }
            Some(value)
        } else {
            // Channel must be closed
            None
        }
    }
}

// Usage follows three-class pattern
let mut msg_channel = MessageChannelOps::construct(100);
defer MessageChannelOps::destruct(&mut msg_channel);

// Send using functional class method
co_await SendOps::send(&msg_channel, Message::new("hello"));

// Receive using functional class method
let received = co_await RecvOps::recv(&msg_channel);
```

### Message Bus Following Three-Class Pattern

A more complex example with message routing:

```cpp
// Message bus data class
class MessageBus {
    // Multiple channels for different message types
    user_channel: MessageChannel<UserMessage>,
    system_channel: MessageChannel<SystemMessage>,
    broadcast_channel: MessageChannel<BroadcastMessage>,
    
    // Bus metadata
    subscriber_count: AtomicUsize,
    message_count: AtomicU64,
    
    // Access rights for different capabilities
    exposes UserPublisher { user_channel }
    exposes UserSubscriber { user_channel }
    exposes SystemPublisher { system_channel }
    exposes SystemSubscriber { system_channel }
    exposes BroadcastPublisher { broadcast_channel }
    exposes BroadcastSubscriber { broadcast_channel }
    
    // Construction control
    constructed_by: MessageBusManager,
}

// Bus management as functional class
functional class MessageBusManager {
    fn construct(
        user_capacity: usize,
        system_capacity: usize,
        broadcast_capacity: usize
    ) -> MessageBus {
        MessageBus {
            user_channel: MessageChannelOps::construct(user_capacity),
            system_channel: MessageChannelOps::construct(system_capacity),
            broadcast_channel: MessageChannelOps::construct(broadcast_capacity),
            subscriber_count: AtomicUsize::new(0),
            message_count: AtomicU64::new(0),
        }
    }
    
    fn destruct(bus: &mut MessageBus) {
        // Clean up all channels
        MessageChannelOps::destruct(&mut bus.user_channel);
        MessageChannelOps::destruct(&mut bus.system_channel);
        MessageChannelOps::destruct(&mut bus.broadcast_channel);
    }
    
    // Factory methods for different access levels
    fn create_user_session(bus: &MessageBus) -> UserSession {
        UserSession {
            bus_ref: bus,  // Reference to data class
        }
    }
    
    fn create_system_service(bus: &MessageBus) -> SystemService {
        SystemService {
            bus_ref: bus,  // Reference to data class
        }
    }
}

// User session wraps bus with specific access rights
class UserSession {
    bus_ref: &MessageBus,
    
    exposes UserOps { bus_ref }
    constructed_by: UserSessionOps,
}

functional class UserOps {
    fn publish_message(session: &UserSession, msg: UserMessage) -> Result<()> {
        // Can only access user channel through access rights
        co_await SendOps::send(&session.bus_ref.user_channel, msg)
    }
    
    fn subscribe_to_messages(session: &UserSession) -> Option<UserMessage> {
        // Can only receive from user channel
        co_await RecvOps::recv(&session.bus_ref.user_channel)
    }
    
    // Cannot access system or broadcast channels - compile error
    // fn publish_system_message(...) // Would not compile
}

// System service has broader access
class SystemService {
    bus_ref: &MessageBus,
    
    exposes SystemOps { bus_ref }
    constructed_by: SystemServiceOps,
}

functional class SystemOps {
    fn publish_system_message(service: &SystemService, msg: SystemMessage) -> Result<()> {
        co_await SendOps::send(&service.bus_ref.system_channel, msg)
    }
    
    fn publish_broadcast(service: &SystemService, msg: BroadcastMessage) -> Result<()> {
        // System can broadcast to all users
        co_await SendOps::send(&service.bus_ref.broadcast_channel, msg)
    }
    
    fn monitor_user_messages(service: &SystemService) -> Option<UserMessage> {
        // System can monitor user traffic
        co_await RecvOps::recv(&service.bus_ref.user_channel)
    }
}
```

### Channel-Based Worker Pool

A complete worker pool implementation using the three-class system:

```cpp
// Worker pool data class
class WorkerPool<T> {
    // Work distribution channel
    work_channel: MessageChannel<WorkItem<T>>,
    
    // Result collection channel
    result_channel: MessageChannel<WorkResult<T>>,
    
    // Pool metadata
    worker_count: usize,
    active_workers: AtomicUsize,
    processed_count: AtomicU64,
    
    // Access rights for different roles
    exposes WorkProducer { work_channel }
    exposes WorkConsumer { work_channel, result_channel }
    exposes ResultCollector { result_channel }
    
    constructed_by: WorkerPoolManager<T>,
}

// Pool management operations
functional class WorkerPoolManager<T> {
    fn construct(
        worker_count: usize,
        work_capacity: usize,
        result_capacity: usize
    ) -> WorkerPool<T> {
        WorkerPool {
            work_channel: MessageChannelOps::construct(work_capacity),
            result_channel: MessageChannelOps::construct(result_capacity),
            worker_count,
            active_workers: AtomicUsize::new(0),
            processed_count: AtomicU64::new(0),
        }
    }
    
    fn destruct(pool: &mut WorkerPool<T>) {
        // Ensure all workers are terminated
        Self::shutdown_all_workers(pool);
        
        // Clean up channels
        MessageChannelOps::destruct(&mut pool.work_channel);
        MessageChannelOps::destruct(&mut pool.result_channel);
    }
    
    fn start_workers(pool: &WorkerPool<T>) {
        for worker_id in 0..pool.worker_count {
            // Each worker gets WorkConsumer access rights
            let worker_access = WorkerConsumer {
                pool_ref: pool,
                worker_id,
            };
            
            spawn worker_task(worker_access);
            pool.active_workers.fetch_add(1, Ordering::Relaxed);
        }
    }
    
    fn shutdown_all_workers(pool: &WorkerPool<T>) {
        // Close work channel - all workers will terminate
        MessageChannelOps::close(&mut pool.work_channel);
        
        // Wait for workers to finish
        while pool.active_workers.load(Ordering::Acquire) > 0 {
            std::thread::sleep(Duration::from_millis(10));
        }
    }
}

// Worker with access to both work and result channels
class WorkerConsumer<T> {
    pool_ref: &WorkerPool<T>,
    worker_id: usize,
    
    exposes WorkerOps { pool_ref }
    constructed_by: WorkerOps<T>,
}

functional class WorkerOps<T> {
    fn process_work(worker: &WorkerConsumer<T>) -> WorkerResult {
        loop {
            // Receive work (competitive consumption)
            match co_await RecvOps::recv(&worker.pool_ref.work_channel) {
                Some(WorkItem::Job(work)) => {
                    // Process the work item
                    let result = process_work_item(work);
                    
                    // Send result back
                    let work_result = WorkResult {
                        worker_id: worker.worker_id,
                        result,
                        processed_at: SystemTime::now(),
                    };
                    
                    co_await SendOps::send(&worker.pool_ref.result_channel, work_result);
                    
                    // Update counters
                    worker.pool_ref.processed_count.fetch_add(1, Ordering::Relaxed);
                },
                Some(WorkItem::Terminate) => {
                    println("Worker {} received termination signal", worker.worker_id);
                    break;
                },
                None => {
                    println("Work channel closed, worker {} terminating", worker.worker_id);
                    break;
                }
            }
        }
        
        // Worker cleanup
        worker.pool_ref.active_workers.fetch_sub(1, Ordering::Relaxed);
        WorkerResult::Terminated
    }
}

// Usage of the worker pool system
fn worker_pool_example() {
    // Create pool following three-class pattern
    let mut pool = WorkerPoolManager::construct(4, 100, 50);
    defer WorkerPoolManager::destruct(&mut pool);
    
    // Start workers
    WorkerPoolManager::start_workers(&pool);
    
    // Create producer with limited access
    let producer = WorkProducer { pool_ref: &pool };
    
    // Create result collector with limited access
    let collector = ResultCollector { pool_ref: &pool };
    
    // Send work
    spawn producer_task(producer);
    spawn result_collector_task(collector);
    
    // Wait for processing to complete
    std::thread::sleep(Duration::from_secs(10));
    
    // Shutdown (automatic via defer)
}
```

### Three-Class Benefits in Channel Design

1. **Clear Separation**: Channel state (data), operations (functional), and access control are distinct
2. **Memory Safety**: Construction control ensures proper initialization of complex channel structures
3. **Performance**: Functional classes enable aggressive inlining and optimization
4. **Testability**: Stateless operations are easily unit tested
5. **Composability**: Channels, access rights, and coroutines all follow the same pattern

## Composition Control in Libraries

### Library Ownership of Combinatorial Possibilities

The three-class system creates a fundamental principle: **if you own the object definitions, you control the combinatorial possibilities**. This becomes crucial for library design where N:M composition can create exponential combinations.

#### The N:M Explosion Challenge

```cpp
// Library provides:
class DatabaseConnection {
    handle: DbHandle,
    pool_info: PoolInfo,
    constructed_by: ConnectionManager,
}

// And functional classes:
functional class ReadOps { /* ... */ }
functional class WriteOps { /* ... */ }
functional class AdminOps { /* ... */ }
functional class CacheOps { /* ... */ }
functional class LoggingOps { /* ... */ }

// Potential combinations explode exponentially:
// DatabaseConnection<ReadOps>
// DatabaseConnection<WriteOps>
// DatabaseConnection<AdminOps>
// DatabaseConnection<ReadOps, CacheOps>
// DatabaseConnection<WriteOps, LoggingOps>
// DatabaseConnection<AdminOps, CacheOps, LoggingOps>
// ... and many more
```

#### Library Control Philosophy

Libraries should explicitly control which combinations they support:

```cpp
module Database {
    // Data classes
    class Connection {
        handle: DbHandle,
        state: ConnectionState,
        constructed_by: ConnectionManager,
    }
    
    // Functional classes  
    functional class ReadOps { /* ... */ }
    functional class WriteOps { /* ... */ }
    functional class AdminOps { /* ... */ }
    
    // Explicit combination control
    comptime {
        // Safe, tested combinations
        extern template Connection<ReadOps>;
        extern template Connection<WriteOps>;
        extern template Connection<AdminOps>;
        extern template Connection<ReadOps, WriteOps>;
        
        // Conditional combinations
        #[cfg(feature = "admin")]
        extern template Connection<AdminOps, WriteOps>;
        
        // Security-validated combinations only
        for (ops_combo in generate_safe_combinations()) {
            if (validate_security_model(ops_combo)) {
                extern template Connection<{...ops_combo}>;
            }
        }
    }
    
    // Some combinations deliberately omitted:
    // extern template Connection<AdminOps, ReadOps>;  // Unnecessary
    // extern template Connection<DebugOps>;           // Internal only
    // extern template Connection<UnsafeTestOps>;      // Testing only
}
```

### Security Boundaries Through Composition Control

```cpp
module PaymentProcessor {
    class PaymentData {
        card_number: EncryptedString,
        amount: Currency,
        merchant_id: MerchantId,
        constructed_by: PaymentManager,
    }
    
    functional class UserPaymentOps { /* safe operations */ }
    functional class MerchantPaymentOps { /* merchant operations */ }
    functional class AdminPaymentOps { /* admin operations */ }
    functional class AuditOps { /* logging and compliance */ }
    
    // Library enforces security through combination control
    comptime {
        // Safe user combinations
        extern template PaymentData<UserPaymentOps>;
        extern template PaymentData<UserPaymentOps, AuditOps>;
        
        // Safe merchant combinations  
        extern template PaymentData<MerchantPaymentOps>;
        extern template PaymentData<MerchantPaymentOps, AuditOps>;
        
        // Admin combinations
        extern template PaymentData<AdminPaymentOps, AuditOps>;  // Audit required
        
        // Forbidden combinations (security violations):
        // extern template PaymentData<UserPaymentOps, AdminPaymentOps>;
        // extern template PaymentData<MerchantPaymentOps, AdminPaymentOps>;
        // extern template PaymentData<AdminPaymentOps>;  // No audit
        
        // Compile-time security validation
        static_assert(!allows_escalation<UserPaymentOps, AdminPaymentOps>(),
                     "Users cannot escalate to admin privileges");
        static_assert(requires_audit<AdminPaymentOps>(),
                     "Admin operations must include audit trail");
    }
}
```

### Testing Scope Control

Libraries only test what they explicitly provide:

```cpp
module NetworkStack {
    class TcpConnection {
        socket: Socket,
        buffer: Buffer,
        constructed_by: TcpManager,
    }
    
    // Testing matrix matches provided combinations
    comptime {
        const connection_types = [BasicTcp, SecureTcp, ReliableTcp];
        const buffer_types = [SimpleBuffer, RingBuffer];
        
        // Generate test-validated combinations
        for (conn_type in connection_types) {
            extern template TcpConnection<{conn_type}>;
            
            for (buf_type in buffer_types) {
                if (is_compatible(conn_type, buf_type)) {
                    extern template TcpConnection<{conn_type}, {buf_type}>;
                    
                    // Each combination gets explicit test coverage
                    #[test]
                    fn test_{conn_type}_{buf_type}_combination() {
                        // Comprehensive testing for this specific combination
                    }
                }
            }
        }
    }
    
    // Users can only rely on tested combinations
    // Custom combinations require user testing
}
```

### Extension Control Matrix

Libraries can use the extension control matrix to manage composition boundaries:

```cpp
// Fully controlled - library manages all combinations
#[extension(data_closed, access_closed)]
module CryptoLibrary {
    class CryptoContext {
        algorithm: CryptoAlgorithm,
        key_material: SecureKey,
        constructed_by: CryptoManager,
    }
    
    // Only these security-validated combinations exist
    extern template CryptoContext<AES256>;
    extern template CryptoContext<ChaCha20>;
    extern template CryptoContext<AES256, HMACAuth>;
}

// Operations extensible - users can add new operations to library data
#[extension(data_closed, access_open)]
module MonitoringLibrary {
    class MetricsCollector {
        counters: AtomicCounters,
        timers: TimerArray,
        constructed_by: MetricsManager,
    }
    
    // Library provides core operations
    extern template MetricsCollector<CoreMetrics>;
    extern template MetricsCollector<PerformanceMetrics>;
    
    // Users can add: MetricsCollector<CustomAnalytics>
    // But cannot modify MetricsCollector structure
}

// Data extensible - users implement interfaces on their types
#[extension(data_open, access_closed)]
module SerializationLibrary {
    interface Serializable {
        fn serialize(&self) -> Vec<u8>;
        fn deserialize(data: &[u8]) -> Result<Self>;
    }
    
    // Users can implement Serializable on any data class
    // But cannot add methods to Serializable interface
}

// Fully extensible - complete protocol
#[extension(data_open, access_open)]
module PluginSystem {
    interface PluginProtocol {
        type Config;
        type State;
        
        fn initialize(&mut self, config: Self::Config) -> Result<()>;
        fn process(&mut self, input: &[u8]) -> Result<Vec<u8>>;
    }
    
    // Users can implement complete custom plugins
    // And extend the protocol with new methods
}
```

### Distribution Strategy Implications

Extension modes determine what libraries ship:

```cpp
libdatabase/
  bin/
    database.so              // Pre-compiled combinations
  include/
    public_api.h             // Always included - public API
    connection_data.h        // Only if data_open
    operation_interfaces.h   // Only if access_open
    // private_internals.h   // Never shipped for closed components
```

| Extension Mode | Ships Binary | Ships Headers | User Capability |
|----------------|--------------|---------------|-----------------|
| `[closed, closed]` | ✓ | Public API only | Use pre-compiled combinations |
| `[data_open, closed]` | ✓ | API + data class headers | Implement interfaces |
| `[closed, access_open]` | ✓ | API + operation headers | Add new operations |
| `[open, open]` | ✓ | Full source headers | Complete extensibility |

### Benefits of Library Composition Control

1. **Security Boundaries**: Prevent dangerous capability combinations
2. **Testing Scope**: Only test what you provide, clear user responsibility
3. **API Clarity**: Explicit about what combinations are supported
4. **Performance**: Pre-compiled common cases, header compilation for extensions
5. **Evolution**: Can add combinations without breaking existing code

### Best Practices for Library Authors

```cpp
// 1. Start closed, open gradually
#[extension(data_closed, access_closed)]  // v1.0 - conservative
// #[extension(data_closed, access_open)]  // v2.0 - based on user needs
// #[extension(data_open, access_open)]    // v3.0 - if needed

// 2. Use comptime to avoid manual enumeration
comptime {
    for (combination in generate_valid_combinations()) {
        if (passes_security_audit(combination)) {
            extern template DataClass<{...combination}>;
        }
    }
}

// 3. Document extension points clearly
/// Extension Policy:
/// - [data_closed]: Library controls all data structures  
/// - [access_open]: Users can add new operations via headers
/// - Security: All admin operations require audit trail
/// - Testing: Library tests provided combinations, users test custom ones

// 4. Provide migration paths (can only become MORE open)
// v1: [closed, closed] -> v2: [closed, open] ✓ Safe
// v2: [closed, open] -> v1: [closed, closed] ❌ Breaking change
```

This composition control system ensures that the three-class system scales to library boundaries while maintaining security, testability, and clear architectural boundaries.

The three-class system enforces good architectural patterns while maintaining the performance characteristics essential for systems programming. Channels exemplify this approach by cleanly separating data (communication state), operations (send/receive logic), and capabilities (access rights), resulting in a powerful yet safe concurrency primitive.