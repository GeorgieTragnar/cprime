# CPrime Access Rights System

## Overview

CPrime's access rights system provides **one-level mixin polymorphism** through vtable extensions. Unlike traditional hierarchical inheritance, access rights use **single-level capability composition** where multiple capabilities can be mixed together without creating inheritance hierarchies. Each access right adds its own vtable pointer and memory, similar to C++ multiple inheritance but without the complexity of deep inheritance chains.

This system offers both compile-time (static) and runtime (dynamic) variants, allowing developers to choose between zero-cost abstractions and dynamic flexibility.

## Core Concepts

### One-Level Mixin Model vs Hierarchical Inheritance

CPrime access rights implement a **mixin model** rather than traditional inheritance hierarchies:

```cpp
// ❌ NOT hierarchical inheritance (like C++):
// class Base { virtual void method() = 0; };
// class Derived1 : public Base { void method() override; };
// class Derived2 : public Derived1 { void method() override; };  // Deep hierarchy

// ✅ CPrime mixin model - single-level capability composition:
class NetworkConnection {
    socket: TcpSocket,
    buffer: [u8; 4096],
    
    // Each access right is a separate mixin - no inheritance hierarchy
    exposes TcpOps { socket, buffer }      // TCP capability
    exposes SslOps { socket }              // SSL capability  
    exposes CompressionOps { buffer }      // Compression capability
    exposes LoggingOps { socket, buffer }  // Logging capability
}

// VTable composition, not inheritance chain:
// - TcpOps vtable: [connect, send, receive, close]
// - SslOps vtable: [handshake, encrypt, decrypt]  
// - CompressionOps vtable: [compress, decompress]
// - LoggingOps vtable: [log_send, log_receive]

// Final object can have ALL capabilities simultaneously
let full_connection: NetworkConnection<TcpOps, SslOps, CompressionOps, LoggingOps> = 
    ConnectionFactory::create_secure_compressed_logged();

// Access specific capabilities directly - no casting chain
TcpOps::send(&full_connection, data);
SslOps::encrypt(&full_connection, &data);
CompressionOps::compress(&full_connection, &data);
LoggingOps::log_send(&full_connection, data.len());
```

**Key Differences from Hierarchical Inheritance:**

| Aspect | Hierarchical Inheritance | CPrime Mixin Model |
|--------|-------------------------|-------------------|
| **Depth** | Unlimited nesting (Base→Derived1→Derived2→...) | Single level only |
| **Composition** | Linear inheritance chain | Multiple parallel capabilities |
| **VTable Structure** | Chained vtable lookups | Direct vtable per capability |
| **Method Resolution** | Complex diamond problem | Simple direct dispatch |
| **Memory Layout** | Base + derived fields in sequence | Base + capability vtables |
| **Casting** | Upcast/downcast through hierarchy | Direct capability casting |

### Access Rights Are Inheritance Extensions

Access rights in CPrime work like C++ inheritance, adding vtable pointers and memory to the base data class:

```cpp
// Base data class
class Connection {
    handle: DbHandle,
    buffer: [u8; 4096],
    
    // Each access right adds vtable pointers and memory
    exposes UserOps { handle, buffer }      // +8 bytes for vtable ptr
    exposes AdminOps { handle, buffer }     // +8 bytes for vtable ptr
    exposes runtime QueryOps { handle }     // +8 bytes for vtable ptr + RTTI
}

// Memory layout (conceptual):
// [base fields][UserOps vtable ptr][AdminOps vtable ptr][QueryOps vtable ptr + RTTI]
// Total: base_size + (num_access_rights * ptr_size) + runtime_overhead
```

### Casting Required for Access

Like C++ inheritance, you need to cast to access derived-only operations:

```cpp
// Cannot directly call AdminOps methods on base Connection
let conn: Connection = create_connection();
// AdminOps::delete_table(&conn);  // ❌ Compile error

// Must cast to appropriate access right
if let Some(admin) = conn.cast::<AdminOps>() {
    AdminOps::delete_table(&admin);  // ✓ Now accessible
}

// Or with compile-time known type
let admin_conn: Connection<AdminOps> = create_admin_connection();
AdminOps::delete_table(&admin_conn);  // ✓ Direct access
```

## Compile-Time vs Runtime Access Rights

### Compile-Time Access Rights (Default)

By default, access rights are compile-time with restricted casting:

```cpp
class SecureData {
    key: [u8; 32],
    value: String,
    
    // Compile-time access rights - no runtime overhead
    exposes UserOps { value }
    exposes AdminOps { key, value }
}

// Compile-time type includes access right
let user_data: SecureData<UserOps> = create_user_data();
let admin_data: SecureData<AdminOps> = create_admin_data();

// Upcasting allowed (to base or interface)
let base: &SecureData = &user_data;  // ✓ Upcast to base

// Downcasting forbidden at compile-time
// let admin = user_data.cast::<AdminOps>();  // Always None/false

// Dynamic cast on compile-time access always fails unless same type
let same = user_data.dynamic_cast::<UserOps>();  // ✓ Returns Some
let diff = user_data.dynamic_cast::<AdminOps>(); // ✗ Returns None
```

### Runtime Access Rights

Runtime access rights enable full polymorphism with dynamic dispatch:

```cpp
class DynamicConnection {
    handle: DbHandle,
    
    // Runtime keyword enables dynamic dispatch
    runtime exposes UserOps { handle }
    runtime exposes AdminOps { handle }
    runtime exposes QueryOps { handle }
}

// Runtime polymorphism - type erased
let conn: runtime Connection = create_dynamic_connection();

// Dynamic casting works
match conn.dynamic_cast() {
    Some(AdminOps(admin)) => {
        AdminOps::dangerous_operation(&admin);
    },
    Some(UserOps(user)) => {
        UserOps::safe_operation(&user);
    },
    None => println("Unknown access type"),
}

// RTTI enables runtime type queries
if conn.has_capability::<AdminOps>() {
    // Admin operations available
}
```

### Memory and Performance Impact

| Feature | Compile-Time | Runtime |
|---------|--------------|---------|
| Vtable per access right | Yes (static) | Yes (dynamic) |
| Memory overhead | 8 bytes per access | 8+ bytes per access + RTTI |
| Downcasting | Forbidden | Full support |
| Dynamic dispatch | No | Yes |
| Type erasure | No | Yes |
| Zero-cost | Yes (when type known) | No |

## How Access Rights Differ from Interfaces

While access rights provide inheritance-like polymorphism, interfaces provide common vtable patterns:

```cpp
// Interface defines common operations
interface Queryable {
    fn execute_query(&self, sql: &str) -> Result<QueryResult>;
}

// Access rights implement interface
impl Queryable for Connection<UserOps> {
    fn execute_query(&self, sql: &str) -> Result<QueryResult> {
        UserOps::query(self, sql)
    }
}

impl Queryable for Connection<AdminOps> {
    fn execute_query(&self, sql: &str) -> Result<QueryResult> {
        AdminOps::query(self, sql)  // May have more privileges
    }
}

// Can work with any Queryable without casting
fn run_query(conn: &impl Queryable, sql: &str) {
    conn.execute_query(sql)?;  // No casting needed
}
```

## Memory Layout Details

### Compile-Time Layout

```cpp
class FileData {
    handle: FileHandle,     // 8 bytes
    path: String,          // 24 bytes
    
    exposes ReadOps { handle, path }    // +8 bytes vtable
    exposes WriteOps { handle }         // +8 bytes vtable
}

// Memory layout for FileData<ReadOps>:
// [handle: 8][path: 24][ReadOps vtable: 8] = 40 bytes
// 
// Memory layout for FileData<WriteOps>:
// [handle: 8][path: 24][WriteOps vtable: 8] = 40 bytes
//
// Memory layout for FileData<ReadOps, WriteOps>:
// [handle: 8][path: 24][ReadOps vtable: 8][WriteOps vtable: 8] = 48 bytes
```

### Runtime Layout

```cpp
class RuntimeFileData {
    handle: FileHandle,
    
    runtime exposes ReadOps { handle }
    runtime exposes WriteOps { handle }
}

// Memory layout includes RTTI:
// [handle: 8][vtable ptr: 8][type_info: 8+] = 24+ bytes
// The vtable itself contains pointers to all exposed operations
```

## Advanced Patterns

### Multiple Access Rights

A single object can expose multiple access rights:

```cpp
class DatabaseConnection {
    handle: DbHandle,
    cache: QueryCache,
    
    exposes QueryOps { handle, cache }
    exposes AdminOps { handle }
    exposes CacheOps { cache }
}

// Can be accessed through any exposed interface
let conn = create_connection();

// Must cast to specific access right
if let Some(query) = conn.cast::<QueryOps>() {
    QueryOps::select(&query, "SELECT ...");
}

if let Some(admin) = conn.cast::<AdminOps>() {
    AdminOps::create_table(&admin, schema);
}
```

### Access Right Hierarchies

Access rights can form hierarchies similar to inheritance:

```cpp
// Base access
class BasicAccess {
    exposes ReadOps { ... }
}

// Extended access (conceptual - syntax TBD)
class AdminAccess : BasicAccess {
    exposes WriteOps { ... }
    exposes DeleteOps { ... }
}

// Admin has all Basic operations plus more
let admin: AdminAccess = create_admin();
ReadOps::read(&admin);    // ✓ Inherited
WriteOps::write(&admin);  // ✓ Admin-only
```

### Combining with Interfaces

Access rights and interfaces work together:

```cpp
interface DataSource {
    fn fetch_data(&self) -> Vec<u8>;
}

// Different access rights provide different implementations
impl DataSource for FileData<ReadOps> {
    fn fetch_data(&self) -> Vec<u8> {
        ReadOps::read_all(self)
    }
}

impl DataSource for NetworkData<SecureOps> {
    fn fetch_data(&self) -> Vec<u8> {
        SecureOps::encrypted_read(self)
    }
}

// Generic function works with any DataSource
fn process_data(source: &impl DataSource) {
    let data = source.fetch_data();
    // Process data...
}
```

## Security Model

### Capability-Based Security

Access rights provide unforgeable capabilities:

```cpp
mod secure {
    class ProtectedData {
        secret: Secret,
        
        // Only SecureOps can access secret
        exposes SecureOps { secret }
        constructed_by: SecureModule,
    }
    
    functional class SecureModule {
        fn construct(secret: Secret) -> ProtectedData {
            ProtectedData { secret }
        }
    }
    
    functional class SecureOps {
        fn process_secret(data: &ProtectedData) -> Result<()> {
            // Can access data.secret
            validate_secret(&data.secret)
        }
    }
}

// External code cannot forge access
// let fake = ProtectedData { secret: stolen };  // ❌ Cannot construct
// SecureOps::process_secret(&fake);             // ❌ Wrong type
```

### Runtime Security Checks

Runtime access rights enable dynamic security policies:

```cpp
class DynamicResource {
    data: SensitiveData,
    
    runtime exposes UserOps { data }
    runtime exposes AdminOps { data }
    runtime exposes AuditorOps { data }
}

fn enforce_security_policy(
    resource: &runtime Resource,
    user: &User,
    operation: Operation
) -> Result<()> {
    // Check user permissions
    match (user.role, operation) {
        (Role::Admin, _) => {
            if let Some(admin) = resource.cast::<AdminOps>() {
                AdminOps::perform(&admin, operation)
            } else {
                Err("Resource doesn't support admin operations")
            }
        },
        (Role::User, Operation::Read) => {
            if let Some(user_ops) = resource.cast::<UserOps>() {
                UserOps::read(&user_ops)
            } else {
                Err("Resource doesn't support user operations")
            }
        },
        _ => Err("Permission denied"),
    }
}
```

## Comparison with C++ Inheritance

```cpp
// C++ approach
class Connection {
    DbHandle handle;
};

class UserConnection : public Connection {
    // Adds vtable, can downcast
};

class AdminConnection : public UserConnection {
    // More inheritance, deeper hierarchy
};

// CPrime approach
class Connection {
    handle: DbHandle,
    
    exposes UserOps { handle }
    exposes AdminOps { handle }
}

// Advantages:
// 1. No deep hierarchies
// 2. Explicit memory cost
// 3. Multiple independent access rights
// 4. Compile-time vs runtime choice
// 5. No virtual destructor issues
```

## Integration with Coroutines

### Capability Preservation Across Suspensions

Access rights are preserved across coroutine suspension points, maintaining security and capabilities throughout the coroutine's lifetime:

```cpp
class DatabaseCoro {
    stack_memory: *mut u8,
    connection: DbConnection,
    query_cache: QueryCache,
    
    // Mixin-style access rights preserved across suspensions
    exposes ReadOps { connection, query_cache }
    exposes WriteOps { connection }
    exposes AdminOps { connection, query_cache }
    
    constructed_by: DbCoroManager,
}

// Coroutine maintains capabilities across all suspension points
async fn handle_admin_request(request: AdminRequest) -> AdminResponse {
    // This coroutine has AdminOps capabilities throughout execution
    let validation = co_await validate_admin_credentials(&request.auth);
    
    if validation.is_valid() {
        // Still has AdminOps after suspension
        let query_result = co_await AdminOps::execute_privileged_query(&request.sql);
        
        // Another suspension - capabilities still preserved
        co_await AdminOps::clear_audit_logs();
        
        AdminResponse::success(query_result)
    } else {
        AdminResponse::unauthorized()
    }
}

// Coroutine creation with specific access rights
let admin_coro = DbCoroManager::construct_with_admin_access(admin_connection);

// Access rights casting works on coroutines too
if let Some(admin_ops) = admin_coro.cast::<AdminOps>() {
    AdminOps::execute_management_query(&admin_ops, "OPTIMIZE TABLE users");
}
```

### Coroutine Access Right Patterns

```cpp
// Pattern 1: Security-focused coroutines
class SecureTaskCoro {
    task_data: TaskData,
    security_context: SecurityContext,
    
    // Different access levels for different operations
    exposes GuestOps { task_data }           // Read-only access
    exposes UserOps { task_data }            // Standard operations
    exposes ModeratorOps { task_data }       // Moderation capabilities
    exposes AdminOps { task_data, security_context }  // Full access
}

async fn process_user_task(task: UserTask) -> TaskResult {
    // This coroutine automatically has UserOps capabilities
    let processed = co_await UserOps::process_task(&task);
    
    if processed.needs_review() {
        // Can't escalate to ModeratorOps - would need different coroutine type
        co_await UserOps::submit_for_review(&processed);
    }
    
    TaskResult::completed(processed)
}

// Pattern 2: Capability-based workflow coroutines
async fn moderation_workflow(task: ModerationTask) -> ModerationResult {
    // This coroutine has ModeratorOps capabilities
    let review = co_await ModeratorOps::review_content(&task.content);
    
    match review.decision {
        Decision::Approve => {
            co_await ModeratorOps::approve_content(&task.content);
            ModerationResult::approved()
        },
        Decision::Reject => {
            co_await ModeratorOps::reject_content(&task.content, &review.reason);
            ModerationResult::rejected(review.reason)
        },
        Decision::Escalate => {
            // ModeratorOps can escalate but not execute admin actions
            co_await ModeratorOps::escalate_to_admin(&task);
            ModerationResult::escalated()
        },
    }
}
```

### Runtime Coroutine Polymorphism

```cpp
// Mixed coroutine types with different access rights
union runtime CoroVariant {
    GuestCoro(runtime SecureTaskCoro<GuestOps>),
    UserCoro(runtime SecureTaskCoro<UserOps>),
    ModeratorCoro(runtime SecureTaskCoro<ModeratorOps>),
    AdminCoro(runtime SecureTaskCoro<AdminOps>),
}

class TaskScheduler {
    active_coroutines: Vec<runtime CoroVariant>,
    constructed_by: SchedulerOps,
}

functional class SchedulerOps {
    fn process_all_tasks(scheduler: &mut TaskScheduler) {
        for coro_space in &mut scheduler.active_coroutines {
            // Unified API across all coroutine access levels
            if let Some(guest) = coro_space.try_as::<SecureTaskCoro<GuestOps>>() {
                GuestOps::resume_limited(guest);
            } else if let Some(user) = coro_space.try_as::<SecureTaskCoro<UserOps>>() {
                UserOps::resume_standard(user);
            } else if let Some(moderator) = coro_space.try_as::<SecureTaskCoro<ModeratorOps>>() {
                ModeratorOps::resume_moderation(moderator);
            } else if let Some(admin) = coro_space.try_as::<SecureTaskCoro<AdminOps>>() {
                AdminOps::resume_admin(admin);
            }
            
            // Or use interface if available
            if let Some(resumable) = coro_space.try_as::<dyn Resumable>() {
                resumable.resume();  // Polymorphic resume
            }
        }
    }
}
```

### Mixin Composition in Coroutines

```cpp
// Complex capability mixing in coroutines
class WebServerCoro {
    request: HttpRequest,
    response: HttpResponse,
    connection: NetworkConnection,
    
    // Multiple independent capabilities can be mixed
    exposes HttpOps { request, response }          // HTTP protocol handling
    exposes TlsOps { connection }                  // TLS encryption
    exposes CompressionOps { response }            // Response compression
    exposes CacheOps { request, response }         // Caching logic
    exposes LoggingOps { request, response }       // Request logging
    exposes MetricsOps { request, response }       // Performance metrics
}

async fn handle_web_request(request: HttpRequest) -> HttpResponse {
    // This coroutine has ALL the mixed capabilities available
    
    // HTTP processing
    let parsed = co_await HttpOps::parse_request(&request);
    
    // Check cache first
    if let Some(cached) = co_await CacheOps::lookup(&parsed) {
        // Log cache hit
        co_await LoggingOps::log_cache_hit(&request);
        co_await MetricsOps::record_cache_hit();
        return cached;
    }
    
    // Process request
    let mut response = co_await HttpOps::process_request(&parsed);
    
    // Apply compression if supported
    if CompressionOps::client_supports_compression(&request) {
        response = co_await CompressionOps::compress_response(&response);
    }
    
    // Cache the response
    co_await CacheOps::store(&request, &response);
    
    // Log and record metrics
    co_await LoggingOps::log_response(&request, &response);
    co_await MetricsOps::record_response_time(&request);
    
    response
}

// Creating coroutine with specific capability mix
let web_coro = WebServerManager::construct_full_featured(request);
// This coroutine has access to all 6 capability mixins

// Can cast to specific capabilities as needed
if let Some(metrics) = web_coro.cast::<MetricsOps>() {
    MetricsOps::generate_report(&metrics);
}
```

## Best Practices

### Choose Compile-Time When Possible

```cpp
// Good: Compile-time when access is known
fn process_user_data(conn: &Connection<UserOps>) {
    UserOps::read(conn);  // Zero overhead
}

// Use runtime only when needed
fn process_plugin_data(conn: &runtime Connection) {
    // Plugin might have any access rights
    match conn.cast() {
        Some(UserOps(u)) => UserOps::read(&u),
        Some(AdminOps(a)) => AdminOps::read(&a),
        _ => panic!("Unknown access type"),
    }
}
```

### Minimize Access Rights

```cpp
// Bad: Too many access rights
class OverloadedData {
    data: Vec<u8>,
    
    exposes ReadOps { data }
    exposes WriteOps { data }
    exposes AppendOps { data }
    exposes TruncateOps { data }
    exposes CompressOps { data }
    // ... 10 more
}

// Good: Grouped logically
class StreamData {
    data: Vec<u8>,
    
    exposes IOOps { data }        // Read, Write, Seek
    exposes AdminOps { data }     // Truncate, Resize
}
```

### Use Interfaces for Common Operations

```cpp
// Instead of casting everywhere
fn process_connections(conns: &[Connection]) {
    for conn in conns {
        if let Some(query) = conn.cast::<QueryOps>() {
            QueryOps::execute(&query, "SELECT ...");
        }
    }
}

// Use interface
fn process_queryables(conns: &[impl Queryable]) {
    for conn in conns {
        conn.execute_query("SELECT ...");  // No casting
    }
}
```

Access rights provide powerful inheritance-like polymorphism with explicit memory costs and the choice between compile-time performance and runtime flexibility.