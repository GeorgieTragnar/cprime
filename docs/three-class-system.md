# CPrime Three-Class System

## Overview

The three-class system is CPrime's fundamental architectural innovation that replaces traditional inheritance with a clear separation of concerns. Each class type serves a specific purpose and has specific constraints, enforcing good design patterns at the language level.

## The Three Class Types

### 1. Data Classes - Pure State

Data classes contain only state (fields) and basic memory management operations. They cannot have semantic methods or business logic. This includes regular data classes and unions (which are a special type of data class).

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

#### Memory Operation Semantics

- **Move**: Always public, performs bit-wise relocation
- **Copy**: Private by default, accessible to stack containers
- **Assign**: Private by default, accessible to stack containers  
- **Drop**: Called automatically at end of scope (RAII)

#### Union Data Classes (Memory Contracts)

Unions are special data classes that act as **memory contracts** - they reserve enough space for any of their variants but never exist as objects themselves:

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

**Union Characteristics:**
- **Memory contracts**: Reserve space for largest variant, objects live inside
- **No union objects**: You never work with "the union", only objects in union space
- **Unified type system**: Runtime unions use existing vtable system (no duplicate tracking)
- **Template integration**: Enable self-expanding heterogeneous containers
- **Performance**: Compile-time (traditional) or runtime (vtable-based) variants

**Union vs Access Rights:**
- **Unions**: Memory reservation for different objects, direct object access
- **Access Rights**: Same object with different vtable views, casting required

For comprehensive union documentation, see [unions.md](unions.md).

### 2. Functional Classes - Pure Operations

Functional classes contain only stateless operations. They cannot have member variables and serve as namespaces for related functionality.

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
2. **Optional constructor/destructor**: For managing paired data classes or unions
3. **Without constructor = module**: Pure utility functions
4. **All operations are static**: No `self` parameter except in constructor/destructor
5. **Stateless by design**: Cannot maintain state between calls
6. **Can operate on unions**: Pattern matching on union variants is allowed

#### Constructor/Destructor Semantics

- **Constructor**: Returns instance of paired data class or union
- **Destructor**: Cleans up data class/union instance (called automatically)
- **Pairing**: One functional class can manage one data class or union type
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

## Polymorphism in the Three-Class System

The three-class system works with CPrime's three-tier polymorphism approach:

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

### 3. Unions (Memory Contracts)
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

The three-class system enforces good architectural patterns while maintaining the performance characteristics essential for systems programming.