# CPrime Unions

## Overview

Unions in CPrime are **memory contracts** that reserve space for a collection of types, providing a powerful alternative to inheritance-based polymorphism. Rather than being objects themselves, unions act as namespace-like memory reservations where actual objects live. This design eliminates type tracking duplication and integrates seamlessly with CPrime's existing vtable system for runtime polymorphism.

## Core Concepts

### Unions as Memory Contracts

Unions are **not objects** - they are memory layout contracts that reserve enough space to hold any of their variants:

```cpp
union ConnectionSpace {
    UserConn(Connection<UserOps>),      // 40 bytes
    AdminConn(Connection<AdminOps>),    // 48 bytes  
    PowerConn(Connection<UserOps, AdminOps>), // 56 bytes
}
// Memory contract: Reserve 56 bytes (largest variant)
// No union "object" exists - just properly-sized memory space
```

### How Unions Differ from Other Polymorphism

1. **Access Rights**: Same object, different vtable views, casting required
2. **Interfaces**: Common contracts across types, shared vtable patterns  
3. **Unions**: Memory space for different objects, direct object access

**Key Insight**: You never work with "the union" - you work with objects that live in union-managed memory.

### Memory Layout Strategy

Union memory layout depends on whether the variants have runtime type information:

**Compile-Time Variants (with discriminant):**
```cpp
union CompileTimeMessage {
    Connect { addr: String, port: u16 },
    Data { payload: Vec<u8> },
}
// Layout: [discriminant: u8][padding][data: max(variants)]
// Total: 1 + 7 + 32 = 40 bytes
```

**Runtime Variants (vtable-based):**
```cpp
union runtime RuntimeSpace {
    UserConn(runtime Connection<UserOps>),
    AdminConn(runtime Connection<AdminOps>),
}
// Layout: [data: max(variants)] - no separate discriminant!
// Type identification via existing vtable system
// Total: 56 bytes (just the largest variant)

## Working with Union Memory Spaces

### Creating Objects in Union Space

```cpp
union ConnectionSpace {
    UserConn(Connection<UserOps>),
    AdminConn(Connection<AdminOps>),
}

// You don't create "a union" - you create objects IN union space
fn create_user_connection() -> ConnectionSpace {
    let mut space: ConnectionSpace = uninitialized;
    
    // Construct actual object in the reserved space
    let user_conn = ConnectionFactory::construct_user_in_place(&mut space, creds);
    space  // Returns space containing a constructed Connection<UserOps>
}

// Access the object living in union space
fn process_connection(space: &ConnectionSpace) {
    // Work with the actual object, not "the union"
    if let Some(user) = space.try_as::<Connection<UserOps>>() {
        UserOps::query(&user, "SELECT ...");  // Normal object method call
    } else if let Some(admin) = space.try_as::<Connection<AdminOps>>() {
        AdminOps::delete_table(&admin, "temp");  // Normal object method call
    }
}
```

### Compile-Time vs Runtime Union Spaces

**Compile-Time Unions** use traditional pattern matching:

```cpp
union Message {
    Connect { addr: String, port: u16 },
    Data { payload: Vec<u8> },
    Ping,
}

// Traditional pattern matching with discriminant
fn handle_message(msg: Message) {
    match msg {
        Message::Connect { addr, port } => {
            println!("Connecting to {}:{}", addr, port);
        },
        Message::Data { payload } => {
            process_data(payload);
        },
        Message::Ping => send_pong(),
    }
}
```

**Runtime Unions** leverage the vtable system:

```cpp
union runtime DynamicSpace {
    TextMsg(runtime TextMessage),
    BinaryMsg(runtime BinaryMessage),
    ControlMsg(runtime ControlMessage),
}

// No pattern matching - use existing dynamic casting
fn handle_dynamic(space: &runtime DynamicSpace) {
    // Same API as access rights - no union-specific syntax
    if let Some(text) = space.try_as::<TextMessage>() {
        text.process();  // Normal method call via vtable
    } else if let Some(binary) = space.try_as::<BinaryMessage>() {
        binary.decode();  // Normal method call via vtable
    } else if let Some(control) = space.try_as::<ControlMessage>() {
        control.execute();  // Normal method call via vtable
    }
}
```

## Template Integration and Self-Expanding Unions

One of the most powerful features is template integration with automatic union expansion:

### Self-Expanding Heterogeneous Containers

```cpp
// User writes this simple code:
let mut items: Vec<runtime Any> = Vec::new();

// Behind the scenes, compiler maintains a growing union:
items.push(SmallItem{data: 42});          // Internal union: {SmallItem} - 8 bytes
items.push(MediumItem{data: [1,2,3,4]});  // Internal union: {SmallItem, MediumItem} - 16 bytes -> REALLOCATION!  
items.push(HugeItem{data: [0; 1024]});    // Internal union: {Small, Medium, Huge} - 1024 bytes -> REALLOCATION!

// Access is seamless - no knowledge of internal union required
for item in &items {
    if let Some(processable) = item.try_as::<dyn Processable>() {
        processable.process();  // Interface call via existing vtable
    }
}
```

### Just Like Vector Capacity Growth

```cpp
template<typename... Ts>
class HeterogeneousVec {
private:
    // Compiler maintains hidden expanding union
    union runtime InternalStorage<Ts...> {
        // Variants added automatically as push() is called with new types
    };
    
    Vector<InternalStorage> data;

public:
    template<typename U> 
    void push(runtime U item) {  // Only runtime objects supported
        if constexpr (sizeof(U) > current_union_size) {
            // Expand union and reallocate - just like Vec capacity growth
            expand_union_and_reallocate<U>();
        }
        data.push(InternalStorage::from(item));
    }
    
    // Performance optimization - manual expansion
    template<typename U>
    void reserve_for_type() {
        expand_union_to_include<U>();
    }
};
```

### Compile-Time to Runtime Conversion

Self-expanding containers require runtime objects (with vtables). Compile-time objects must be converted:

```cpp
// Compile-time object (no vtable)
let compile_time_conn = ConnectionFactory::create_user();

// To store in heterogeneous container, convert to runtime
let runtime_conn = compile_time_conn.to_runtime();  // Adds vtable information

// Now can be stored in self-expanding container
heterogeneous_vec.push(runtime_conn);
```

### Why Runtime Objects Only?

- **Type identification**: Self-expanding containers need vtables to identify object types
- **Dynamic casting**: Access requires `try_as::<T>()` which needs runtime type info
- **Memory management**: Proper destruction requires vtable dispatch for complex objects
- **Performance**: Compile-time objects can be converted when flexibility is needed

## Unified Type System Integration

### No Duplicate Type Tracking

The key insight is that runtime unions eliminate duplicate type tracking:

```cpp
// BEFORE: Separate type tracking systems
class RuntimeObject {
    vtable: *const VTable,  // Type info for access rights/interfaces
}

union TraditionalUnion {
    discriminant: u8,       // DUPLICATE type tracking!
    data: [u8; MAX_SIZE],
}

// AFTER: Unified type tracking
union runtime UnifiedSpace {
    UserConn(runtime Connection<UserOps>),    // Type info in vtable
    AdminConn(runtime Connection<AdminOps>),  // Type info in vtable
}
// No separate discriminant - vtable already carries type information!
```

### Seamless API Integration

Runtime unions use the same API as access rights and interfaces:

```cpp
// Same dynamic casting API works for:

// 1. Access rights
if let Some(admin) = connection.cast::<AdminOps>() {
    AdminOps::delete_table(&admin);
}

// 2. Interfaces  
if let Some(drawable) = shape.cast::<dyn Drawable>() {
    drawable.draw();
}

// 3. Runtime unions
if let Some(text_msg) = union_space.try_as::<TextMessage>() {
    text_msg.process();
}
```

## Compile-Time vs Runtime Unions

### Compile-Time Unions (Default)

When the compiler can determine the variant at compile time, it optimizes away the tag check:

```cpp
// Compile-time union - variant known statically
fn create_error_result() -> Result<i32, String> {
    Result::Err("Error message".to_string())  // Compiler knows it's Err
}

// The compiler can optimize pattern matching
fn process_known_error() {
    let result = create_error_result();
    match result {
        Ok(value) => unreachable!(),  // Compiler may eliminate this branch
        Err(e) => println("Error: {}", e),  // Direct access, no tag check
    }
}

// State machines with compile-time variants
union State {
    Idle,
    Processing { task_id: u64 },
    Complete { result: String },
}

// Compile-time state transitions
fn start_processing(state: State<Idle>) -> State<Processing> {
    State::Processing { task_id: generate_id() }
}
```

### Runtime Unions

Runtime unions allow dynamic variant selection and addition:

```cpp
// Runtime union declaration
union runtime DynamicMessage {
    Text(String),
    Binary(Vec<u8>),
    Json(JsonValue),
}

// Can be extended at runtime (in theory)
fn add_variant_at_runtime(msg: &mut runtime DynamicMessage) {
    // If new variant is larger than current max, union expands
    // This is useful for plugin systems, dynamic protocols, etc.
}

// Runtime pattern matching with tag checks
fn process_dynamic(msg: runtime DynamicMessage) {
    match msg {  // Runtime tag check required
        DynamicMessage::Text(s) => process_text(s),
        DynamicMessage::Binary(b) => process_binary(b),
        DynamicMessage::Json(j) => process_json(j),
    }
}
```

## Unions as Data Classes

In CPrime's three-class system, unions are data classes with special properties:

```cpp
// Union as data class
union ConnectionState {
    Disconnected,
    Connecting { attempt: u32 },
    Connected { socket: TcpStream, peer: SocketAddr },
    Error { code: ErrorCode, message: String },
    
    // Unions can specify constructor access
    constructed_by: ConnectionManager,
    
    // Memory operations follow data class rules
    fn move(self) -> Self;
    fn copy(&self) -> Self {
        // Custom copy for non-Copy variants
        match self {
            ConnectionState::Connected { socket, peer } => {
                ConnectionState::Connected { 
                    socket: socket.try_clone()?, 
                    peer: *peer 
                }
            },
            _ => *self,  // Bit-copy for simple variants
        }
    }
}

// Functional class for union operations
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
```

## Solving Template Type Explosion

One of unions' most powerful features is solving the template/access-rights type explosion problem:

```cpp
// Problem: Without unions, need different containers for each access combination
Container<UserOps> user_connections;
Container<AdminOps> admin_connections;
Container<UserOps, QueryOps> power_connections;
// Cannot mix different access rights in same container!

// Solution: Union of all access combinations
union ConnectionVariant {
    UserConn(Connection<UserOps>),
    AdminConn(Connection<AdminOps>),
    PowerConn(Connection<UserOps, QueryOps>),
    ReadOnlyConn(Connection<QueryOps>),
}

// Now one container can hold all variants
class ConnectionPool {
    connections: Vec<ConnectionVariant>,
    constructed_by: PoolManager,
}

functional class PoolManager {
    fn add_connection(pool: &mut ConnectionPool, variant: ConnectionVariant) {
        pool.connections.push(variant);
    }
    
    fn execute_on_all(pool: &ConnectionPool, query: &str) {
        for conn in &pool.connections {
            match conn {
                ConnectionVariant::UserConn(c) => {
                    UserOps::execute(c, query);
                },
                ConnectionVariant::AdminConn(c) => {
                    AdminOps::execute(c, query);
                },
                ConnectionVariant::PowerConn(c) => {
                    // Can use either UserOps or QueryOps
                    QueryOps::execute(c, query);
                },
                ConnectionVariant::ReadOnlyConn(c) => {
                    QueryOps::execute_readonly(c, query);
                },
            }
        }
    }
}
```

## Nested Unions and Complex Patterns

```cpp
union JsonValue {
    Null,
    Bool(bool),
    Number(f64),
    String(String),
    Array(Vec<JsonValue>),
    Object(HashMap<String, JsonValue>),
}

// Nested pattern matching
fn extract_user_id(json: &JsonValue) -> Option<u64> {
    match json {
        JsonValue::Object(map) => {
            match map.get("user")? {
                JsonValue::Object(user_obj) => {
                    match user_obj.get("id")? {
                        JsonValue::Number(n) => Some(*n as u64),
                        _ => None,
                    }
                },
                _ => None,
            }
        },
        _ => None,
    }
}

// Or with nested patterns
fn extract_user_id_v2(json: &JsonValue) -> Option<u64> {
    if let JsonValue::Object(map) = json {
        if let Some(JsonValue::Object(user)) = map.get("user") {
            if let Some(JsonValue::Number(id)) = user.get("id") {
                return Some(*id as u64);
            }
        }
    }
    None
}
```

## Performance Considerations

### Memory Efficiency

```cpp
// Small unions benefit from niche optimization
union SmallResult {
    Ok(u32),
    Err,  // No data needed
}
// Size: 8 bytes (4 for u32, 1 for tag, 3 padding)
// With niche optimization: Could be 4 bytes

// Large unions need careful design
union LargeMessage {
    SmallPing,  // 0 bytes
    HugeData([u8; 65536]),  // 64KB
}
// Size: 64KB + tag - wastes space for SmallPing
```

### Tag Representation Options

```cpp
// Standard tag (u8 or u16)
union StandardUnion {
    A, B, C,  // Tag: 0, 1, 2
}

// Niche optimization for Option-like unions
union Option<T> {
    Some(T),
    None,  // Could use null pointer representation
}

// Tagged pointer optimization (future)
union SmallString {
    Inline([u8; 23]),  // Store in-place
    Heap(String),      // Use LSB of pointer as tag
}
```

## Best Practices

### When to Use Unions

1. **Replacing dynamic_cast chains**: Clean pattern matching instead of casting
2. **Protocol messages**: Different message types in same channel
3. **State machines**: Compile-time state transitions
4. **Error handling**: Result<T, E> pattern
5. **Heterogeneous collections**: Different types in same container

### When NOT to Use Unions

1. **Open-ended extensibility**: Use interfaces or access rights
2. **Large size differences**: Wastes memory for small variants
3. **Frequent variant changes**: Consider dynamic dispatch instead

### Design Guidelines

```cpp
// Good: Variants have similar sizes
union NetworkPacket {
    Data { header: Header, payload: [u8; 1024] },
    Control { header: Header, command: Command },
    Ack { header: Header, sequence: u32 },
}

// Bad: Huge size difference
union BadDesign {
    Tiny(u8),
    Huge([u8; 1_000_000]),  // Wastes 999,999 bytes for Tiny
}

// Good: Clear semantic variants
union AuthResult {
    Success { token: AuthToken, expires: Timestamp },
    InvalidCredentials,
    AccountLocked { until: Timestamp },
    NetworkError { message: String },
}

// Bad: Using unions for unrelated types
union Confused {
    User(UserData),
    TcpSocket(Socket),
    RandomNumber(f64),  // No semantic relationship!
}
```

## Comparison with C++ std::variant

CPrime unions solve the problems with C++ std::variant:

```cpp
// C++ std::variant problems:
// 1. Awkward visitor pattern
// 2. Complex alignment calculations  
// 3. Exception safety issues
// 4. Error-prone index-based access
// 5. No exhaustiveness checking

// CPrime union advantages:
// 1. Clean pattern matching
// 2. Compiler handles alignment
// 3. Move/copy follow data class rules
// 4. Named variants, not indices
// 5. Exhaustive match required
```

## Future Extensions

### Planned Features

1. **Open unions**: Allow adding variants in other modules
2. **Niche optimization**: Automatic null pointer representation
3. **Tagged pointer optimization**: Use pointer bits for small unions
4. **Const evaluation**: Pattern matching in const contexts

### Syntax Extensions

```cpp
// Potential future: Anonymous unions in function returns
fn parse_number(s: &str) -> union { Integer(i64), Float(f64), Invalid } {
    // ...
}

// Potential future: Union spreads
union Extended {
    ...Base,  // Include all variants from Base
    NewVariant(Data),
}
```

Unions provide a powerful, zero-cost abstraction for variant types that integrates cleanly with CPrime's three-class system and offers superior ergonomics compared to traditional inheritance-based polymorphism.