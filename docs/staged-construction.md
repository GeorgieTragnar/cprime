# CPrime Staged Construction

## Overview

Staged construction is a revolutionary object lifecycle management system that allows objects to exist in two states: **dead** (unusable) or **alive** (fully functional). Objects transition from dead to alive through explicit staging operations that must complete for all members before the object awakens.

This design provides memory safety guarantees, prevents partial initialization bugs, and enables powerful compiler optimizations while maintaining CPrime's core principles of performance and C++ compatibility.

## Core Concept

### Object States

Every staged object exists in one of two states:

1. **Dead State**: Object memory is allocated but contents are uninitialized or partially initialized. All method calls result in immediate termination.
2. **Alive State**: Object is fully constructed and operational. All methods work normally.

The transition from dead to alive is atomic and controlled through the staging system.

### Memory Layout

```cpp
struct StagedServer {
    void* vptr;           // Points to dead_vtable or alive_vtable
    char network_storage[sizeof(Network)];  // Uninitialized initially
    char db_storage[sizeof(Database)];      // Uninitialized initially  
    char cache_storage[sizeof(Cache)];      // Uninitialized initially
    uint8_t stage_mask;   // Bitfield tracking what's constructed
};
```

### VTable Management

Staged construction uses a dual-vtable system:

```cpp
// Shared by ALL staged types when dead - prevents accidental usage
struct DeadVTable {
    void (*any_call)() = []{ abort("Dead object accessed"); };
    // All virtual calls point to the same abort function
};

// Per-type alive vtable with normal functionality
struct ServerVTable {
    void (*destructor)(Server*);
    void (*query)(Server*, ...);
    void (*connect)(Server*, ...);
    // Normal member functions
};
```

## Language Syntax

### Basic Staging Declaration

```cpp
class Server {
    staged Network network;           // Declared but not constructed
    staged Database db;               // No memory allocated yet
    staged optional Cache cache;      // Optional staged member
    
    // Staging function - compiler-enforced signal handling
    [[stage_function]] void awaken() {
        stage(network, "config.json");   // May block
        stage(db, network.connection()); // May throw stage_failure
        
        if (memory_available())
            stage(cache, 1_GB);          // Optional staging
    }
};
```

### Construction and Usage

```cpp
// Stack allocation with staging
{
    Server s;                    // Created in dead state
    try {
        s.awaken();             // Transition to alive state
        s.query("SELECT ...");  // Safe to use after awakening
    } catch(stage_failure& e) {
        // s remains dead, destructs safely
        println("Staging failed: {}", e.message());
    }
} // Automatic cleanup regardless of state

// Heap allocation with staging
auto s = make_staged<Server>();
if (!s->try_awaken(timeout_30s)) {
    s.reset();  // Clean destruction of dead object
}
```

## Integration with Three-Class System

### Staged Data Classes

Data classes can be staged, following normal data class rules:

```cpp
class ConnectionData {
    staged TcpSocket socket;
    staged TlsContext tls;
    peer_address: SocketAddr,
    
    constructed_by: ConnectionManager,
    
    [[stage_function]] void awaken() {
        stage(socket, peer_address);
        stage(tls, socket.get_handle());
    }
}
```

### Functional Classes for Staged Operations

Functional classes manage staged data classes:

```cpp
functional class ConnectionManager {
    fn construct(addr: SocketAddr) -> ConnectionData {
        ConnectionData {
            socket: staged(),        // Uninitialized 
            tls: staged(),           // Uninitialized
            peer_address: addr,
        }
    }
    
    fn destruct(conn: &mut ConnectionData) {
        // Cleanup based on what was successfully staged
        if conn.is_staged(tls) {
            TlsContext::cleanup(&mut conn.tls);
        }
        if conn.is_staged(socket) {
            TcpSocket::close(&mut conn.socket);
        }
    }
    
    fn query(conn: &ConnectionData, sql: &str) -> Result<QueryResult> {
        // This will only be callable on alive objects
        require_alive!(conn);
        TcpSocket::send(&conn.socket, sql.as_bytes())
    }
}
```

### Danger Classes with Staging

Danger classes can also use staging for complex C++ interop:

```cpp
#[danger(stateful_class, ffi)]
class LegacyDriver {
    staged native_handle: *mut c_void,
    staged config_data: ConfigStruct,
    
    [[stage_function]] fn awaken(&mut self) -> Result<()> {
        // Stage native C++ resources
        stage(self.native_handle, unsafe { 
            cpp_create_driver() 
        });
        
        stage(self.config_data, unsafe {
            cpp_load_config(self.native_handle)
        });
        
        Ok(())
    }
    
    fn query_device(&self, command: u32) -> Result<DeviceResponse> {
        require_alive!(self);
        unsafe { cpp_send_command(self.native_handle, command) }
    }
}
```

## Stage Failure Semantics

### Failure Policies

Stage failures can be handled with different policies:

```cpp
enum class stage_failure_policy {
    terminate,    // Default: unhandled failure = program termination
    exception,    // Throw catchable stage_failure exception
    error_code    // Return Result<(), StageError> (must check)
};

template<stage_failure_policy P = terminate>
void stage(staged auto& member, auto&&... args);
```

### Signal Integration

Stage failures integrate with CPrime's signal handling system:

```cpp
signal STAGE_FAILURE {
    member_name: String,
    error_code: StageErrorCode,
    context: StageContext,
    stage_index: u32,
}

class NetworkService {
    staged Database db;
    staged Cache cache;
    
    [[stage_function]] void awaken() except(STAGE_FAILURE) {
        catch(DATABASE_CONNECTION_ERROR) {
            stage(db, connection_string)?;
        } recover {
            // Transform database error to stage failure
            raise(STAGE_FAILURE {
                member_name: "db".to_string(),
                error_code: StageErrorCode::DatabaseInitFailed,
                context: current_stage_context(),
                stage_index: 0,
            });
        }
        
        // Cache staging is optional
        if let Err(cache_error) = try_stage(cache, cache_config) {
            log_warning("Cache staging failed: {}", cache_error);
            // Continue without cache
        }
    }
}
```

## Destructor Behavior

### State-Aware Destruction

Destructors automatically handle partial construction:

```cpp
class Server {
    staged Network network;
    staged Database db; 
    staged Cache cache;
    
    // Compiler-generated destructor
    ~Server() {
        // VTable encodes what needs destruction via stage_mask
        if (stage_mask & NETWORK_STAGED) {
            network.~Network();
        }
        if (stage_mask & DB_STAGED) {
            db.~Database();
        }
        if (stage_mask & CACHE_STAGED) {
            cache.~Cache();
        }
        
        // Switch to dead vtable to prevent post-destruction access
        vptr = &dead_vtable;
    }
}
```

### RAII Compatibility

Staged construction preserves RAII principles:

```cpp
functional class ResourceManager {
    fn construct() -> ResourceData {
        ResourceData {
            file: staged(),
            connection: staged(),
            buffer: staged(),
        }
    }
    
    fn destruct(resource: &mut ResourceData) {
        // RAII cleanup happens automatically
        // No need to check staging state - destructor handles it
    }
}

// Usage - RAII still works
{
    let mut resource = ResourceManager::construct();
    defer ResourceManager::destruct(&mut resource);
    
    resource.awaken()?;  // May fail
    // ... use resource
} // Automatic cleanup regardless of staging state
```

## Move/Copy Restrictions

### Dead Object Restrictions

Dead objects cannot be moved or copied:

```cpp
class Server {
    staged Network network;
    staged Database db;
    
    // Movement restrictions based on state
    fn move(self) -> Self requires(self.is_alive()) {
        // Only alive objects can be moved
        move_implementation(self)
    }
    
    // Copy is always forbidden for staged types
    fn copy(&self) -> Self = delete;
}

// Usage
let dead_server = Server::new();        // Dead state
// let moved = dead_server.move();      // ❌ COMPILE ERROR

dead_server.awaken()?;                  // Transition to alive
let moved = dead_server.move();         // ✓ OK - now alive
```

### Alive Object Movement

Once alive, objects can be moved normally:

```cpp
fn transfer_server(from: &mut ServerContainer, to: &mut ServerContainer) {
    if let Some(server) = from.servers.pop() {
        if server.is_alive() {
            to.servers.push(server.move());  // ✓ Safe move
        } else {
            // Must awaken before transfer
            server.awaken()?;
            to.servers.push(server.move());
        }
    }
}
```

## Composition Rules

### Staged Hierarchies

Staged objects compose naturally:

```cpp
class Application {
    staged Server server;
    staged UI ui;
    staged Logger logger;
    
    [[stage_function]] void awaken() {
        // All members must stage before Application awakens
        stage(logger, LogConfig::default());    // Stage logger first
        stage(server, ServerConfig::from_file("config.json"));
        stage(ui, UIConfig::with_logger(&logger));
        
        // Application becomes alive only after all staging succeeds
    }
}

// Usage
let mut app = Application::new();  // Dead state
app.awaken()?;                     // All-or-nothing staging
app.run();                         // Safe to use - fully initialized
```

### Partial Staging Strategies

For applications requiring partial functionality:

```cpp
class FlexibleService {
    staged CoreEngine engine;       // Required
    staged optional Analytics analytics;    // Optional
    staged optional Monitoring monitoring;  // Optional
    
    [[stage_function]] void awaken() -> StageResult {
        // Core engine is mandatory
        stage(engine, EngineConfig::default())?;
        
        // Optional components can fail gracefully
        if let Err(e) = try_stage(analytics, AnalyticsConfig::default()) {
            log_warning("Analytics unavailable: {}", e);
        }
        
        if let Err(e) = try_stage(monitoring, MonitoringConfig::default()) {
            log_warning("Monitoring unavailable: {}", e);
        }
        
        Ok(())
    }
    
    fn process_request(&self, req: Request) -> Response {
        require_alive!(self);
        
        let response = self.engine.handle(req);
        
        // Optional components used if available
        if self.is_staged(analytics) {
            self.analytics.record_request(&req);
        }
        
        if self.is_staged(monitoring) {
            self.monitoring.record_response(&response);
        }
        
        response
    }
}
```

## Usage Patterns

### Stack Allocation Pattern

```cpp
{
    Server s;                  // Dead state - zero initialization cost
    
    match s.try_awaken() {
        Ok(()) => {
            s.query("SELECT * FROM users");  // Full functionality
            s.process_requests();
        },
        Err(stage_error) => {
            println("Server initialization failed: {}", stage_error);
            // s automatically destructs safely
        }
    }
} // RAII cleanup regardless of alive/dead state
```

### Heap Allocation Pattern

```cpp
let s = make_staged<Server>();         // Dead state on heap

// Asynchronous awakening
spawn async {
    match s.co_await_awaken().await {
        Ok(()) => {
            loop {
                let request = receive_request().await;
                let response = s.handle_request(request).await;
                send_response(response).await;
            }
        },
        Err(e) => {
            log_error("Server failed to initialize: {}", e);
        }
    }
    
    // s automatically cleaned up when dropped
};
```

### Factory Pattern with Staging

```cpp
functional class DatabaseFactory {
    fn create_connection(config: DbConfig) -> DatabaseConnection {
        // Create in dead state
        DatabaseConnection {
            socket: staged(),
            auth_context: staged(),
            query_cache: staged(),
            config,
        }
    }
    
    fn awaken_connection(conn: &mut DatabaseConnection) -> Result<()> {
        // Multi-phase staging
        conn.stage_socket()?;
        conn.stage_auth()?;
        conn.stage_cache()?;
        Ok(())
    }
}

// Usage
let mut db = DatabaseFactory::create_connection(config);
DatabaseFactory::awaken_connection(&mut db)?;
// db is now ready for use
```

## Compiler Enforcement

### Compile-Time Checks

The compiler enforces staged construction rules:

```cpp
class Server {
    staged Database db;
    
    fn query(&self, sql: &str) -> QueryResult {
        // ❌ COMPILE ERROR: Cannot call methods on dead objects
        // Compiler requires require_alive! or awakening check
        self.db.execute(sql)
    }
    
    fn safe_query(&self, sql: &str) -> Option<QueryResult> {
        require_alive!(self);  // ✓ Explicit alive check
        Some(self.db.execute(sql))
    }
}
```

### Runtime Checks

Runtime checks provide safety guarantees:

```cpp
// Debug builds include runtime checks
fn access_staged_member(server: &Server) {
    if !server.is_alive() {
        panic!("Attempted to access dead object");
    }
    
    // In release builds, this becomes a direct call
    server.db.execute("SELECT 1");
}

// Release builds optimize away checks when provable
fn proven_alive_access(server: &Server) {
    server.awaken().unwrap();  // Compiler knows object is now alive
    server.db.execute("SELECT 1");  // No runtime check needed
}
```

## Timeout Handling

### Timeout Specifications

Staging operations can have timeout constraints:

```cpp
class RemoteService {
    staged Connection conn;
    staged AuthToken token;
    
    [[timeout(30s)]] void awaken() {
        // Both operations share the 30-second timeout window
        stage(conn, remote_endpoint)?;     // May take 20s
        stage(token, conn.authenticate())?; // Has remaining 10s
    }
}
```

### Cooperative Timeout Implementation

```cpp
class AsyncService {
    staged HttpClient client;
    staged DatabasePool pool;
    
    [[timeout(60s)]]
    suspend fn awaken() -> StageResult {
        let deadline = Instant::now() + Duration::from_secs(60);
        
        // Cooperative staging with timeout checks
        stage_with_timeout(client, ClientConfig::default(), deadline)?;
        
        if Instant::now() > deadline {
            return Err(StageError::Timeout);
        }
        
        stage_with_timeout(pool, PoolConfig::default(), deadline)?;
        Ok(())
    }
}
```

## Implementation Requirements

### Compiler Requirements

The compiler must:

1. **Generate dual vtables**: Create dead and alive vtable variants for each staged class
2. **Track staging state**: Generate code to maintain stage_mask bitfield  
3. **Enforce access control**: Prevent method calls on dead objects
4. **Optimize alive paths**: Remove runtime checks when staging state is provable
5. **Handle destructor logic**: Generate state-aware cleanup code

### Runtime Requirements

The runtime must:

1. **Atomic vtable switching**: Ensure thread-safe transitions between dead/alive states
2. **Signal integration**: Handle stage failures through the signal system
3. **Memory management**: Manage uninitialized storage for staged members
4. **Timeout enforcement**: Support cooperative and preemptive timeout handling

### Example Compiler Output

```cpp
// Source code
class Server {
    staged Database db;
    void query() { db.execute("SELECT 1"); }
};

// Compiler-generated implementation
struct Server {
    void* vptr;                    // Points to ServerDeadVTable or ServerAliveVTable
    alignas(Database) char db_storage[sizeof(Database)];
    uint8_t stage_mask;
    
    void query() {
        // Runtime check in debug, optimized away in release when provable
        if (vptr == &ServerDeadVTable) [[unlikely]] {
            abort("Dead object accessed");
        }
        
        // Direct access to staged member
        reinterpret_cast<Database*>(db_storage)->execute("SELECT 1");
    }
};

// Generated vtables
ServerDeadVTable = { .any_call = abort_dead_access };
ServerAliveVTable = { .query = Server::query, .destructor = Server::~Server };
```

## Benefits

### Memory Safety

- **No uninitialized access**: Dead objects cannot be used accidentally
- **Atomic initialization**: Objects transition completely or not at all
- **Safe destruction**: Partial construction is handled correctly
- **Clear lifecycle**: Binary dead/alive state eliminates confusion

### Performance Characteristics

- **Zero cost when alive**: No overhead after successful staging
- **Predictable memory layout**: Known storage requirements at compile time
- **Optimization opportunities**: Compiler can eliminate checks when staging state is known
- **Cache efficiency**: Staged members can be laid out optimally

### Debugging Benefits  

- **Clear failure points**: Stage failures are explicit and debuggable
- **State visibility**: Dead/alive state is easily observable
- **Controlled initialization**: Complex initialization is structured and traceable
- **Memory layout inspection**: Tools can visualize staging progress

### Architectural Advantages

- **Separation of concerns**: Construction logic is separate from business logic
- **Composable patterns**: Staged objects compose naturally
- **Error handling**: Stage failures integrate with CPrime's signal system
- **Migration path**: Existing C++ classes can adopt staging gradually

## Costs

### Complexity Overhead

- **New mental model**: Developers must understand dead/alive states
- **Additional syntax**: Staging requires new language constructs  
- **Compiler complexity**: Dual vtables and state tracking require sophisticated implementation
- **Debugging complexity**: New category of bugs related to staging

### Memory Overhead

- **VTable pointer**: Each staged object requires vtable indirection
- **Stage mask**: Bitfield to track what has been staged
- **Storage alignment**: Staged members may require additional alignment
- **Debug metadata**: Development builds may include additional staging information

### Runtime Costs

- **Vtable indirection**: Method calls go through vtable (optimizable)
- **State checks**: Runtime verification of alive state (debug builds)
- **Atomic operations**: Thread-safe vtable switching when required
- **Signal handling**: Stage failure processing overhead

## Open Questions

### 1. Reflection Integration

How should staged state appear in reflection systems?

```cpp
// Possible reflection API
let type_info = TypeInfo::of<Server>();
for field in type_info.fields() {
    if field.is_staged() {
        println("Field {} staging state: {}", field.name(), field.stage_state());
    }
}
```

### 2. Coroutine Integration

Can staging operations be co_awaited for asynchronous initialization?

```cpp
class AsyncService {
    staged RemoteConnection conn;
    
    suspend fn awaken() {
        // Asynchronous staging
        co_await stage_async(conn, remote_config);
    }
}
```

### 3. Module Boundaries

How do staged types export across module boundaries?

```cpp
// Module A exports staged type
module database {
    export class Connection {
        staged TcpSocket socket;
        [[stage_function]] void awaken();
    };
}

// Module B uses staged type
import database::Connection;

let mut conn = Connection::new();  // Dead state
conn.awaken()?;                   // Cross-module staging
```

### 4. Generic Constraints

Should staged types require specific concepts or traits?

```cpp
// Possible concept requirements
template<Stageable T>
class Container {
    staged T item;
    
    void initialize() requires StageConcept<T> {
        stage(item, default_config<T>());
    }
};
```

## Cross-References

- **[Signal Handling](signal-handling.md)**: Stage failures integrate with CPrime's signal system through `STAGE_FAILURE` signals and `except` annotations on staging functions
- **[Memory Management](memory-management.md)**: Staged construction extends RAII patterns with explicit lifecycle control and state-aware destruction
- **[Three-Class System](three-class-system.md)**: Staging works with all three class types - Data classes for staged state, Functional classes for staging operations, and Danger classes for C++ interop
- **[Core Philosophy](core-philosophy.md)**: Staged construction maintains CPrime's principles of explicit control, performance, and C++ compatibility while adding safety guarantees

Staged construction represents a significant advancement in object lifecycle management, providing memory safety and initialization guarantees while preserving the performance characteristics and familiar patterns that make CPrime suitable for systems programming.