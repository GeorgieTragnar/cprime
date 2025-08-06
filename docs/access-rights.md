# CPrime Access Rights System

## Overview

CPrime's access rights system provides **RAII state modifiers** that enable one-level mixin polymorphism through vtable extensions. These functional classes serve as the exclusive means to modify data class state while maintaining RAII safety.

Unlike traditional hierarchical inheritance, access rights use **single-level capability composition** where multiple RAII state modifiers can be mixed together without creating inheritance hierarchies. Each access right adds its own vtable pointer and memory, similar to C++ multiple inheritance but without the complexity of deep inheritance chains.

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

### Access Rights Are RAII State Modifier Extensions

Access rights in CPrime work as RAII state modifiers, adding vtable pointers and memory to the base data class to enable safe state modification:

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

## Channel Subscription through Access Rights

### Access Rights as Automatic Subscription

In CPrime's channel system, **access rights ARE the subscription mechanism**. There's no need for explicit subscribe/unsubscribe - creating a coroutine with channel access rights automatically subscribes it:

```cpp
// Message bus with internal channel
class MessageBus {
    internal_channel: Channel<Message>,
    
    // Access rights define channel capabilities
    exposes SendOps { internal_channel }    // Send capability
    exposes RecvOps { internal_channel }    // Receive capability
    
    constructed_by: MessageBusManager,
}

functional class MessageBusManager {
    fn construct(capacity: usize) -> MessageBus {
        MessageBus {
            internal_channel: channel<Message>(capacity),
        }
    }
    
    // Create producer with send-only access
    fn create_producer(bus: &MessageBus) -> &MessageBus with SendOps {
        bus with SendOps  // Producer gets send capability only
    }
    
    // Create consumer with receive-only access
    fn create_consumer(bus: &MessageBus) -> &MessageBus with RecvOps {
        bus with RecvOps  // Consumer gets receive capability only
    }
}

// Subscription happens through coroutine spawning
fn setup_message_system() {
    let bus = MessageBusManager::construct(1000);
    
    // Spawn producers - automatically subscribed through SendOps access
    spawn producer_task(MessageBusManager::create_producer(&bus));
    spawn producer_task(MessageBusManager::create_producer(&bus));
    
    // Spawn consumers - automatically subscribed through RecvOps access
    spawn consumer_task(MessageBusManager::create_consumer(&bus));
    spawn consumer_task(MessageBusManager::create_consumer(&bus));
    
    // No explicit subscribe/unsubscribe needed!
    // When coroutine terminates, access right is dropped = automatic unsubscribe
}

async fn producer_task(bus: &MessageBus with SendOps) {
    loop {
        let msg = generate_message();
        // Can only send - receive would be compile error
        co_await SendOps::send(bus, msg);
    }
    // When this coroutine ends, SendOps access dropped = unsubscribed
}

async fn consumer_task(bus: &MessageBus with RecvOps) {
    loop {
        // Can only receive - send would be compile error
        match co_await RecvOps::recv(bus) {
            Some(msg) => process_message(msg),
            None => break,  // Channel closed
        }
    }
    // When this coroutine ends, RecvOps access dropped = unsubscribed
}
```

### Access Rights Need Data Class Instances

A key insight from the channel design: **access rights require actual data class instances**. You can't have access rights floating independently:

```cpp
// CORRECT: Access rights with data class instance
class DatabaseConnection {
    handle: DbHandle,
    cache: QueryCache,
    
    // Access rights operate on the data class fields
    exposes ReadOps { handle, cache }    // Can access handle and cache
    exposes WriteOps { handle }          // Can only access handle
    
    constructed_by: ConnectionManager,
}

// INCORRECT: Access rights without data
// functional class FloatingOps {  // This doesn't make sense!
//     // What data would these operations work on?
// }

// Channel operations need the channel data
functional class ReadOps {
    fn execute_query(conn: &DatabaseConnection, sql: &str) -> QueryResult {
        // Operations work on the actual connection data
        let result = query_database(conn.handle, sql);
        conn.cache.store(sql, result.clone());  // Cache the result
        result
    }
}
```

### Channel Access Rights Pattern

The standard pattern for channel access rights:

```cpp
// Step 1: Data class holds the channel
class WorkBus {
    work_channel: Channel<WorkItem>,
    result_channel: Channel<Result>,
    
    // Step 2: Expose access rights to channel operations
    exposes WorkProducer { work_channel }
    exposes WorkConsumer { work_channel, result_channel }
    exposes ResultCollector { result_channel }
    
    constructed_by: WorkBusManager,
}

// Step 3: Functional classes operate on the channels
functional class WorkProducer {
    fn submit_work(bus: &WorkBus, work: WorkItem) -> Result<()> {
        // Access to work_channel through access rights
        co_await bus.work_channel.send(work)
    }
}

functional class WorkConsumer {
    fn process_work(bus: &WorkBus) -> Option<WorkItem> {
        // Access to both channels through access rights
        if let Some(work) = co_await bus.work_channel.recv() {
            let result = process_work_item(work);
            co_await bus.result_channel.send(result);
            Some(work)
        } else {
            None  // Work channel closed
        }
    }
}

functional class ResultCollector {
    fn collect_result(bus: &WorkBus) -> Option<Result> {
        // Access only to result_channel
        co_await bus.result_channel.recv()
    }
}

// Step 4: Spawn with specific access rights
fn setup_work_system() {
    let work_bus = WorkBusManager::construct(100, 50);
    
    // Producer coroutines - can only submit work
    spawn work_producer(work_bus with WorkProducer);
    spawn work_producer(work_bus with WorkProducer);
    
    // Consumer coroutines - can consume work and send results
    spawn work_consumer(work_bus with WorkConsumer);
    spawn work_consumer(work_bus with WorkConsumer);
    spawn work_consumer(work_bus with WorkConsumer);
    
    // Result collector - can only collect results
    spawn result_collector(work_bus with ResultCollector);
}

async fn work_producer(bus: &WorkBus with WorkProducer) {
    loop {
        let work_item = generate_work();
        WorkProducer::submit_work(bus, work_item)?;
    }
    // Access automatically dropped when coroutine ends
}

async fn work_consumer(bus: &WorkBus with WorkConsumer) {
    loop {
        match WorkConsumer::process_work(bus) {
            Some(_) => continue,  // Processed work item
            None => break,        // Work channel closed
        }
    }
    // Access automatically dropped when coroutine ends
}
```

### Capability-based Channel Security

Access rights provide unforgeable channel capabilities:

```cpp
// Secure channel system with different privilege levels
class SecureMessageBus {
    user_channel: Channel<UserMessage>,
    admin_channel: Channel<AdminMessage>,
    audit_channel: Channel<AuditEvent>,
    
    // Different privilege levels
    exposes UserAccess { user_channel }                    // Users can only use user channel
    exposes AdminAccess { user_channel, admin_channel }    // Admins can use both user and admin
    exposes AuditorAccess { audit_channel }               // Auditors only see audit events
    exposes SystemAccess { user_channel, admin_channel, audit_channel }  // Full access
    
    constructed_by: SecureMessageManager,
}

functional class SecureMessageManager {
    fn create_user_session(bus: &SecureMessageBus, user: &User) -> Result<&SecureMessageBus with UserAccess> {
        if verify_user_credentials(user) {
            Ok(bus with UserAccess)  // User gets limited access
        } else {
            Err("Authentication failed")
        }
    }
    
    fn create_admin_session(bus: &SecureMessageBus, admin: &Admin) -> Result<&SecureMessageBus with AdminAccess> {
        if verify_admin_credentials(admin) && admin.has_admin_privileges() {
            Ok(bus with AdminAccess)  // Admin gets elevated access
        } else {
            Err("Admin authentication failed")
        }
    }
}

// User coroutines are automatically limited by their access rights
async fn user_session(bus: &SecureMessageBus with UserAccess) {
    loop {
        match co_await UserAccess::recv_user_message(bus) {
            Some(msg) => {
                let response = process_user_message(msg);
                UserAccess::send_user_message(bus, response)?;
                // CANNOT access admin or audit channels - compile error
                // AdminAccess::send_admin_message(bus, admin_msg);  // ERROR!
            },
            None => break,
        }
    }
}

// Admin coroutines have broader access
async fn admin_session(bus: &SecureMessageBus with AdminAccess) {
    loop {
        select {
            user_msg = UserAccess::recv_user_message(bus) => {
                if let Some(msg) = user_msg {
                    // Admin can handle user messages
                    handle_user_message(msg);
                }
            },
            
            admin_msg = AdminAccess::recv_admin_message(bus) => {
                if let Some(msg) = admin_msg {
                    // Admin can handle admin-only messages
                    handle_admin_message(msg);
                }
            }
        }
    }
}
```

### Dynamic Channel Access Rights

Using runtime access rights for dynamic capability management:

```cpp
// Dynamic privilege escalation/de-escalation
class DynamicChannelBus {
    message_channel: Channel<Message>,
    
    runtime exposes UserOps { message_channel }
    runtime exposes AdminOps { message_channel }
    runtime exposes SuperuserOps { message_channel }
}

async fn dynamic_session(bus: &runtime DynamicChannelBus) {
    loop {
        // Runtime capability checking
        if let Some(user_ops) = bus.cast::<UserOps>() {
            // User-level operations
            match co_await UserOps::recv_message(&user_ops) {
                Some(msg) => process_user_message(msg),
                None => break,
            }
        } else if let Some(admin_ops) = bus.cast::<AdminOps>() {
            // Admin-level operations
            match co_await AdminOps::recv_privileged_message(&admin_ops) {
                Some(msg) => process_admin_message(msg),
                None => break,
            }
        } else {
            // No valid access rights
            log::error!("Session has no valid channel access");
            break;
        }
    }
}

// Privilege escalation through re-authentication
fn escalate_privileges(
    current_session: &runtime DynamicChannelBus,
    credentials: &AdminCredentials
) -> Result<runtime DynamicChannelBus> {
    if verify_admin_credentials(credentials) {
        // Create new session with elevated privileges
        let elevated_bus: runtime DynamicChannelBus = upgrade_to_admin_access(current_session)?;
        Ok(elevated_bus)
    } else {
        Err("Privilege escalation denied")
    }
}
```

### Subscription Lifecycle Management

Access rights automatically manage subscription lifecycle:

```cpp
// Subscription lifecycle tied to access right lifetime
fn subscription_lifecycle_demo() {
    let event_bus = EventBusManager::construct();
    
    {
        // Create subscriber with access rights
        let subscriber = EventBusManager::create_subscriber(&event_bus);
        
        // Spawn coroutine with subscription access
        let handle = spawn event_processor(subscriber);
        
        // Subscription is active while access rights exist
        co_await handle;  // Wait for coroutine completion
        
    }  // subscriber dropped here - automatic unsubscription!
    
    // event_bus continues to exist, but subscriber is unsubscribed
}

async fn event_processor(bus: &EventBus with RecvOps) {
    // Automatic subscription through access rights
    loop {
        match co_await RecvOps::recv(bus) {
            Some(event) => {
                process_event(event);
                // Subscription remains active
            },
            None => {
                println("Event channel closed");
                break;  // Clean termination
            }
        }
    }
    // When coroutine ends, access right is dropped
    // Scheduler automatically removes from channel's wait queue
}

// Explicit subscription management when needed
functional class SubscriptionManager {
    fn create_managed_subscription(
        bus: &EventBus,
        subscriber_id: SubscriberId
    ) -> ManagedSubscription {
        ManagedSubscription {
            bus_access: bus with RecvOps,
            subscriber_id,
            is_active: true,
        }
    }
    
    fn terminate_subscription(subscription: &mut ManagedSubscription) {
        subscription.is_active = false;
        // Access rights still exist but marked inactive
        // Subscriber can choose when to drop access
    }
}
```

## Templated Access Rights through Interface Memory Contracts

### N:M Composition with Interface Contracts

Interface memory contracts revolutionize access rights by enabling **templated access rights** that work generically across multiple data classes. This enables true N:M composition where multiple data classes can work with multiple access rights through shared interface bindings.

### Generic Functional Classes

Instead of creating separate functional classes for each data type, interfaces enable generic operations:

```cpp
// Traditional approach: Separate functional classes for each type
functional class UserCacheOps {
    fn store(user: &UserData) -> Result<()> { ... }
    fn retrieve(user_id: u64) -> Option<UserData> { ... }
}

functional class ProductCacheOps {
    fn store(product: &ProductData) -> Result<()> { ... }
    fn retrieve(product_id: u64) -> Option<ProductData> { ... }
}

// Interface-enabled approach: Single generic functional class
interface Cacheable {
    memory_contract {
        id: u64,
        timestamp: u64,
        void[32],
        hash: u32,
    }
    fn cache_key(&self) -> CacheKey;
}

// Generic functional class works with ANY Cacheable implementation
functional class CacheOps<T: Cacheable> {
    fn store(data: &T) -> Result<()> {
        let key = data.id;  // Direct access through interface contract
        let entry = CacheEntry {
            key,
            timestamp: data.timestamp,
            hash: data.hash,
            data: serialize_cacheable(data),
        };
        CACHE_STORE.insert(entry)
    }
    
    fn retrieve_by_key(key: u64) -> Option<T> {
        CACHE_STORE.get(&key)
            .and_then(|entry| deserialize_cacheable::<T>(&entry.data))
    }
    
    fn invalidate_expired(max_age: Duration) -> usize {
        let cutoff = SystemTime::now() - max_age;
        let cutoff_timestamp = cutoff.duration_since(UNIX_EPOCH).unwrap().as_secs();
        
        CACHE_STORE.retain(|_, entry| entry.timestamp >= cutoff_timestamp);
        // Returns count of invalidated entries
        CACHE_STORE.len()
    }
}
```

### Multiple Data Classes Implementing Same Interface

Multiple data classes can implement the same interface contract, enabling uniform access:

```cpp
// User data implementing Cacheable
class UserData {
    user_id: u64,        // Maps to id
    last_login: u64,     // Maps to timestamp
    profile: UserProfile, // Fits in void[32]
    profile_hash: u32,   // Maps to hash
    permissions: Vec<Permission>,
    
    implements Cacheable {
        id <- user_id,
        timestamp <- last_login,
        hash <- profile_hash,
        void[32] <- memory_region(&profile, sizeof(UserProfile)),
    }
    
    exposes UserOps { user_id, profile, permissions }
    constructed_by: UserManager,
}

// Product data implementing Cacheable  
class ProductData {
    product_id: u64,     // Maps to id
    modified_at: u64,    // Maps to timestamp
    metadata: ProductMeta, // Fits in void[32]
    content_hash: u32,   // Maps to hash
    inventory: ProductInventory,
    
    implements Cacheable {
        id <- product_id,
        timestamp <- modified_at,
        hash <- content_hash,
        void[32] <- memory_region(&metadata, sizeof(ProductMeta)),
    }
    
    exposes ProductOps { product_id, metadata, inventory }
    constructed_by: ProductManager,
}

// Order data implementing Cacheable
class OrderData {
    order_id: u64,       // Maps to id
    created_at: u64,     // Maps to timestamp
    order_summary: OrderSummary, // Fits in void[32]
    summary_hash: u32,   // Maps to hash
    items: Vec<OrderItem>,
    
    implements Cacheable {
        id <- order_id,
        timestamp <- created_at,
        hash <- summary_hash,
        void[32] <- memory_region(&order_summary, sizeof(OrderSummary)),
    }
    
    exposes OrderOps { order_id, order_summary, items }
    constructed_by: OrderManager,
}
```

### Cross-Type Operations with Template Constraints

Generic functional classes can operate across all types implementing the interface:

```cpp
// Audit operations work with any data type implementing Auditable
interface Auditable {
    memory_contract {
        entity_id: u64,
        user_id: u64,
        event_timestamp: u64,
        audit_hash: u32,
    }
    fn audit_signature(&self) -> AuditSignature;
}

// Generic audit operations
functional class AuditOps<T: Auditable> {
    fn log_access(data: &T, accessing_user: UserId) -> Result<()> {
        let audit_entry = AuditEntry {
            entity_id: data.entity_id,      // Direct interface access
            accessed_by: accessing_user,
            accessed_at: SystemTime::now(),
            original_user: data.user_id,    // Direct interface access
            event_time: data.event_timestamp, // Direct interface access
            integrity_hash: data.audit_hash, // Direct interface access
        };
        
        AUDIT_LOG.append(audit_entry)
    }
    
    fn verify_integrity(data: &T) -> IntegrityResult {
        let computed_hash = calculate_audit_hash(
            data.entity_id,
            data.user_id,
            data.event_timestamp
        );
        
        if computed_hash == data.audit_hash {
            IntegrityResult::Valid
        } else {
            IntegrityResult::Corrupted {
                expected: data.audit_hash,
                computed: computed_hash,
            }
        }
    }
    
    fn find_related_events(data: &T, time_window: Duration) -> Vec<AuditEntry> {
        let start_time = data.event_timestamp.saturating_sub(time_window.as_secs());
        let end_time = data.event_timestamp + time_window.as_secs();
        
        AUDIT_LOG.entries()
            .filter(|entry| {
                entry.entity_id == data.entity_id &&
                entry.event_time >= start_time &&
                entry.event_time <= end_time
            })
            .collect()
    }
}

// Multiple data types can implement Auditable
impl Auditable for UserData {
    // Maps UserData fields to Auditable contract
    entity_id <- user_id,
    user_id <- user_id,
    event_timestamp <- last_login,
    audit_hash <- profile_hash,
}

impl Auditable for OrderData {
    // Maps OrderData fields to Auditable contract  
    entity_id <- order_id,
    user_id <- customer_id,
    event_timestamp <- created_at,
    audit_hash <- summary_hash,
}

// Single implementation works across types
let user = UserData { ... };
let order = OrderData { ... };

AuditOps::log_access(&user, current_user_id)?;   // Same code path
AuditOps::log_access(&order, current_user_id)?;  // Same code path
```

### Interface-Based Access Rights Composition

Data classes can implement multiple interfaces, enabling rich compositional patterns:

```cpp
// Data class implementing multiple interface contracts
class CustomerTransaction {
    transaction_id: u64,
    customer_id: u64,
    amount: Money,
    processed_at: u64,
    transaction_hash: u32,
    details: TransactionDetails,
    
    // Implements multiple interfaces
    implements Cacheable {
        id <- transaction_id,
        timestamp <- processed_at,
        hash <- transaction_hash,
        void[32] <- memory_region(&amount, sizeof(Money)),
    }
    
    implements Auditable {
        entity_id <- transaction_id,
        user_id <- customer_id,
        event_timestamp <- processed_at,
        audit_hash <- transaction_hash,
    }
    
    implements Billable {
        bill_id <- transaction_id,
        amount <- amount,
        customer <- customer_id,
        processed <- processed_at,
    }
    
    // Traditional access rights for specific operations
    exposes TransactionOps { transaction_id, amount, details }
    exposes AccountingOps { transaction_id, amount, processed_at }
    
    constructed_by: TransactionManager,
}

// Can be used with multiple generic functional classes
let transaction = CustomerTransaction { ... };

// Interface-based generic operations
CacheOps::store(&transaction)?;           // Through Cacheable
AuditOps::log_access(&transaction, user_id)?; // Through Auditable  
BillingOps::process_payment(&transaction)?;   // Through Billable

// Traditional access rights operations
TransactionOps::validate(&transaction)?;      // Specific operations
AccountingOps::reconcile(&transaction)?;      // Specific operations
```

### Performance Characteristics of Templated Access Rights

#### Compile-Time Interface Access (Zero-Cost)
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

#### Runtime Interface Access (Flexible)
```cpp
functional class FlexibleCacheOps<T: FlexibleCacheable> {  // Runtime interface
    fn get_cache_key(data: &T) -> u64 {
        // Accessor method calls - small cost for flexibility
        let id = data._flexible_cacheable_id();
        let timestamp = data._flexible_cacheable_timestamp();
        id ^ timestamp
    }
}
```

### Benefits of Templated Access Rights

1. **Code Reuse**: Single implementation works across multiple data types
2. **Type Safety**: Compiler verifies interface contracts at compile time
3. **Performance Options**: Choose between zero-cost and flexible access
4. **Compositional Power**: Data classes can implement multiple interfaces
5. **Evolution**: Add new interfaces without changing existing code

### Migration from Traditional to Templated Access Rights

```cpp
// Before: Multiple separate functional classes
functional class UserCacheOps { ... }
functional class ProductCacheOps { ... }
functional class OrderCacheOps { ... }

// After: Single templated functional class
functional class CacheOps<T: Cacheable> { ... }

// Data classes implement interface
class UserData implements Cacheable { ... }
class ProductData implements Cacheable { ... }  
class OrderData implements Cacheable { ... }

// Usage becomes uniform
CacheOps::store(&user_data)?;
CacheOps::store(&product_data)?;
CacheOps::store(&order_data)?;
```

This represents a fundamental evolution in CPrime's access rights system, enabling powerful generic programming while maintaining type safety and performance characteristics.

Access rights provide powerful inheritance-like polymorphism with explicit memory costs and the choice between compile-time performance and runtime flexibility. With interface memory contracts, they now also enable N:M composition through templated functional classes that work generically across multiple data types. In the channel system, they serve as the automatic subscription mechanism, eliminating the need for explicit subscribe/unsubscribe operations while providing strong capability-based security.