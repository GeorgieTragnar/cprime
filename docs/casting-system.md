# CPrime Casting and Type System

## Overview

CPrime provides a comprehensive casting system that combines C++ compatibility with language-specific improvements for type safety, performance clarity, and syntactic elegance. The system includes three distinct casting mechanisms for different use cases: compile-time performance, runtime safety, and control flow type inspection.

**Key Design Principles:**
- **C++ Compatibility**: Direct equivalents to `static_cast` and `dynamic_cast`
- **Performance Clarity**: Explicit cost signaling for runtime vs compile-time operations
- **Type Safety**: Safe failure modes with predictable behavior
- **Syntactic Innovation**: Enhanced `auto` keyword for cast result access

## I. Core Casting Operations

### 1. static_cast<T>() - Fast Unsafe Casting

**Semantics**: Direct C++ equivalent with identical behavior
- **No runtime checks** - trusts programmer completely
- **Always "succeeds"** - performs pointer arithmetic/vtable adjustment as if cast is valid
- **Undefined behavior** if actual object type doesn't match cast target
- **Zero runtime overhead** - compile-time operation only

```cpp
// Fast unsafe casting - identical to C++
class Connection {
    exposes UserOps { handle }
    exposes AdminOps { handle }
}

let conn: Connection = create_connection();
let admin = static_cast<AdminOps>(conn);  // Fast, no checks, UB if wrong
AdminOps::delete_table(&admin, "temp");  // Proceeds assuming cast was valid
```

**Use Cases:**
- Performance-critical code paths
- When type correctness is guaranteed by program logic
- Casting between known compatible types

### 2. dynamic_cast<T>() - Safe Runtime Casting

**Semantics**: Direct C++ equivalent with CPrime Result/Option integration
- **Runtime RTTI/vtable checking** - verifies actual object type
- **Safe failure modes** - returns `Option<T>` instead of nullptr
- **Polymorphic types only** - requires access rights or runtime interfaces
- **Runtime overhead** - type checking cost

```cpp
// Safe runtime casting with Option return
if let Some(admin) = dynamic_cast<AdminOps>(conn) {
    AdminOps::delete_table(&admin, "temp");  // Safe - type verified
} else {
    println("Connection doesn't have admin access");
}

// Or with panic on failure
let admin = dynamic_cast<AdminOps>(conn)?;  // Panics if cast fails
AdminOps::dangerous_operation(&admin);
```

**Use Cases:**
- Unknown object types from external sources
- Plugin systems and dynamic loading
- Type validation in untrusted contexts

### 3. cast() - Type Inspection for Control Flow

**Semantics**: CPrime innovation for switch/conditional type inspection
- **Returns actual runtime type** - not the object itself
- **Only usable in control flow** - switch statements and if conditionals
- **Enables pattern matching** on runtime types
- **Compiler type narrowing** - provides type-safe access in each branch

```cpp
// Type inspection in switch statements
switch (cast(funcCall())) {
    case AdminOps:
        auto.delete_table("temp");  // auto = funcCall() result as AdminOps
        break;
    case UserOps:
        auto.query("SELECT ...");   // auto = funcCall() result as UserOps
        break;
    default:
        println("Unknown access type");
        break;
}

// Type inspection in conditionals
if (cast(connection) == AdminOps) {
    auto.dangerous_operation();  // auto = connection as AdminOps
}

// Capability checking (replaces separate has_capability keyword)
if (cast(connection) == AdminOps) {
    // We know connection has AdminOps capabilities
    auto.admin_only_operation();
}
```

**Use Cases:**
- Pattern matching on runtime types
- Clean type-based control flow
- Capability checking (replaces separate has_capability keyword)
- Avoiding repeated casting in conditional logic

## II. The auto Keyword Innovation

### Cast Result Access

Within cast conditional blocks, `auto` provides direct access to the type-narrowed object:

```cpp
if (cast(expensive_function()) == AdminOps) {
    // auto refers to expensive_function() result, cast to AdminOps
    auto.delete_table("temp");        // Method call
    auto[0] = new_value;             // Subscript access
    let result = auto.query("...");   // Variable assignment
    return auto;                     // Return cast result
}
```

**Key Benefits:**
- **Single evaluation** - `expensive_function()` called only once
- **Zero overhead** - `auto` is a reference to the temporary, not a copy
- **Type safety** - Compiler ensures `auto` has the correct type in each branch
- **RAII compliance** - Temporary destroyed at end of conditional scope

### Conditional Variable Declaration with Ultimate Precedence

CPrime introduces a precedence rule that makes `auto` assignment bind first in expressions:

```cpp
// Written:
if (auto temp = funcCall() == someValue && someFlag) {
    temp.method();  // temp is funcCall() result, not boolean
}

// Parsed as (ultimate precedence):
if ((auto temp = funcCall()) == someValue && someFlag) {
    temp.method();  // temp has correct type
}
```

**Traditional C++ Equivalent (Verbose):**
```cpp
if (auto temp = funcCall(); temp == someValue && someFlag) {
    temp.method();
}
```

**CPrime Advantage:**
- **Eliminates semicolon syntax** - single expression instead of two
- **Intuitive behavior** - assignment happens first, then comparison
- **Cleaner conditionals** - more readable complex conditions

## III. Syntax Rules and Disambiguation

### No Whitespace in Subscript Operator

**Rule**: CPrime prohibits whitespace in `[]` operator usage everywhere

```cpp
// Valid (no whitespace)
myVec[0];           // ✅ Standard subscript
myArray[index];     // ✅ Variable subscript  
auto[0];            // ✅ Cast result subscript

// Invalid (compile error)
myVec [0];          // ❌ Whitespace not allowed
myArray [ index ];  // ❌ Whitespace not allowed
auto [0];           // ❌ Would be confused with structured binding
```

**Purpose**: Eliminates ambiguity with structured bindings:
```cpp
if (cast(funcCall()) == ArrayType) {
    auto [x, y] = tuple;    // ✅ Structured binding (space required)
    auto[0] = value;        // ✅ Cast result subscript (no space allowed)
}
```

### Auto Usage Contexts

#### 1. Cast Conditionals - Special Semantics
```cpp
if (cast(obj) == SomeType) {
    auto.method();      // ✅ Cast result access
    auto[index];        // ✅ Cast result subscript
    return auto;        // ✅ Return cast result
}
```

#### 2. Variable Declarations - Standard C++ Semantics
```cpp
auto temp = getValue();     // ✅ Normal type deduction
auto [x, y] = tuple;        // ✅ Structured binding
auto lambda = [](){};       // ✅ Lambda type deduction
```

#### 3. Coexistence in Same Scope
```cpp
if (cast(funcCall()) == AdminOps) {
    auto temp = 5.2f;              // ✅ Normal variable declaration
    auto query_result = "SELECT";  // ✅ Normal variable declaration
    
    auto.callMemberFunc(temp);     // ✅ Cast result method call
    auto.delete_table(query_result); // ✅ Cast result operations
    return auto;                   // ✅ Return cast result
}
```

## IV. Integration with CPrime Type System

### Compile-Time vs Runtime Casting

#### Compile-Time Unions
```cpp
union Message { Text(String), Data(Vec<u8>) }
let msg: Message = Message::Text("hello");

// Pattern matching - no runtime overhead
match msg {
    Message::Text(s) => println("Text: {}", s),
    Message::Data(d) => println("Data size: {}", d.len()),
}
```

#### Runtime Unions
```cpp
union runtime DynamicMessage { Text(String), Binary(Vec<u8>) }
let msg: runtime DynamicMessage = create_dynamic_message();

// Runtime casting with vtable lookup
if (cast(msg) == Text) {
    auto.process_text();  // auto = msg as Text variant
}
```

#### Functional Class Access Rights
```cpp
class Connection {
    exposes UserOps { handle }
    exposes AdminOps { handle }
}

// Compile-time access rights
let user_conn: Connection<UserOps> = create_user_connection();
UserOps::query(&user_conn, "SELECT ...");  // Direct access, no casting

// Runtime access rights casting
let conn: runtime Connection = create_dynamic_connection();
if let Some(admin) = dynamic_cast<AdminOps>(conn) {
    AdminOps::dangerous_operation(&admin);
}
```

### Error Handling Patterns

#### Static Cast - Undefined Behavior
```cpp
// Programmer responsibility - no safety net
let admin = static_cast<AdminOps>(conn);  // UB if conn is not AdminOps
AdminOps::delete_table(&admin);  // May crash or corrupt data
```

#### Dynamic Cast - Safe Failure
```cpp
// Option-based error handling
match dynamic_cast<AdminOps>(conn) {
    Some(admin) => AdminOps::delete_table(&admin),
    None => println("Access denied - not admin connection"),
}

// Or with ? operator
let admin = dynamic_cast<AdminOps>(conn)?;  // Early return if cast fails
AdminOps::delete_table(&admin);
```

#### Cast Type Inspection - No Failure
```cpp
// Type inspection cannot fail - just provides information
match cast(conn) {
    AdminOps => auto.dangerous_operation(),
    UserOps => auto.safe_operation(),
    _ => println("Unknown connection type"),
}
```

## V. Operator Overloading Policy

### Prohibited Operator Overloads (Eliminates Antipatterns)

CPrime prohibits overloading operators that create confusion or break fundamental language semantics:

```cpp
// PROHIBITED - These cannot be overloaded in CPrime

operator.         // Member access - eliminates cast result ambiguity
operator->        // Pointer member access - eliminates indirection confusion  
operator,         // Comma operator - preserves sequencing semantics
operator&&        // Logical AND - preserves short-circuiting
operator||        // Logical OR - preserves short-circuiting
operator&         // Address-of - preserves pointer semantics
operator->*       // Pointer-to-member - too obscure, rarely needed
```

**Rationale**: These operators create antipatterns in C++ that lead to subtle bugs and confusion. CPrime eliminates these footguns while maintaining useful operator overloading.

### Allowed Operator Overloads

```cpp
// ALLOWED - These provide clear, useful functionality

// Arithmetic operators
operator+, operator-, operator*, operator/, operator%

// Comparison operators  
operator==, operator!=, operator<, operator>, operator<=, operator>=

// Assignment operators
operator=, operator+=, operator-=, operator*=, operator/=, operator%=

// Container operators
operator[]        // Subscript access (with no-whitespace rule)
operator()        // Function call operator

// Stream operators
operator<<, operator>>    // Input/output streaming
```

### Whitespace Rules

**Subscript Operator**: No whitespace allowed between object and brackets
```cpp
container[index];     // ✅ Valid
container [index];    // ❌ Compile error
```

**All Other Operators**: Standard C++ whitespace rules apply
```cpp
a + b;               // ✅ Valid
a+b;                 // ✅ Valid  
a   +   b;           // ✅ Valid
```

## VI. Performance Implications

### Cast Operation Costs

| Cast Type | Compile-Time Cost | Runtime Cost | Safety |
|-----------|------------------|--------------|--------|
| `static_cast<T>()` | Pointer arithmetic only | Zero | Unsafe (UB on error) |
| `dynamic_cast<T>()` | RTTI generation | Vtable lookup + type check | Safe (Option return) |
| `cast()` | Type info generation | Vtable lookup | Safe (inspection only) |

### Auto Keyword Overhead

#### Cast Result Access - Zero Overhead
```cpp
if (cast(expensive_func()) == AdminOps) {
    auto.delete_table("temp");  // Zero cost - reference to existing temporary
}

// Equivalent to:
{
    const auto& temp_ref = expensive_func();  // Single evaluation
    if (get_runtime_type(temp_ref) == AdminOps) {
        static_cast<const AdminOps&>(temp_ref).delete_table("temp");
    }
}  // Temporary destructor called here
```

#### Variable Declaration - Standard C++ Cost
```cpp
auto temp = expensive_func();  // Same cost as C++ auto declaration
```

### Memory Layout Impact

#### Compile-Time Access Rights
```cpp
class FileData {
    handle: FileHandle,     // 8 bytes
    path: String,          // 24 bytes
    
    exposes ReadOps { handle, path }    // +8 bytes vtable
    exposes WriteOps { handle }         // +8 bytes vtable
}

// Memory layout for FileData<ReadOps>:
// [handle: 8][path: 24][ReadOps vtable: 8] = 40 bytes
```

#### Runtime Access Rights
```cpp
class RuntimeFileData {
    handle: FileHandle,
    
    runtime exposes ReadOps { handle }
    runtime exposes WriteOps { handle }
}

// Memory layout includes RTTI:
// [handle: 8][vtable ptr: 8][type_info: 8+] = 24+ bytes
```

## VII. Best Practices

### When to Use Each Cast Type

#### Use static_cast When:
- **Type correctness is guaranteed** by program logic
- **Maximum performance** is required
- **Working with compile-time known types**

```cpp
// Performance-critical inner loop
for (int i = 0; i < connections.size(); ++i) {
    let admin = static_cast<AdminOps>(connections[i]);  // Fast path
    AdminOps::process_batch(&admin, batch_data[i]);
}
```

#### Use dynamic_cast When:
- **Type is unknown** at compile time
- **Safety is more important** than performance  
- **Working with plugin systems** or dynamic loading

```cpp
// Plugin interface - unknown types
fn process_plugin(plugin: &runtime PluginInterface) -> Result<()> {
    if let Some(audio) = dynamic_cast<AudioPlugin>(plugin) {
        audio.process_audio()?;
    } else if let Some(video) = dynamic_cast<VideoPlugin>(plugin) {
        video.process_video()?;
    }
    Ok(())
}
```

#### Use cast() When:
- **Pattern matching** on runtime types
- **Clean control flow** based on type
- **Multiple operations** on same cast result

```cpp
// Clean type-based dispatch
fn handle_connection(conn: &runtime Connection) {
    match cast(conn) {
        AdminOps => {
            auto.setup_admin_session();
            auto.load_admin_privileges();
            auto.start_admin_monitoring();
        },
        UserOps => {
            auto.setup_user_session();
            auto.apply_user_restrictions();
        },
    }
}
```

### Auto Keyword Usage Guidelines

#### Cast Conditionals - Use auto for Result Access
```cpp
// Good - auto provides clean access to cast result
if (cast(get_connection()) == AdminOps) {
    auto.dangerous_operation();
    let result = auto.query_system_tables();
    return auto;
}

// Avoid - repeated casting
if (dynamic_cast<AdminOps>(get_connection()).is_some()) {
    let admin = dynamic_cast<AdminOps>(get_connection()).unwrap();  // Double cast!
    admin.dangerous_operation();
}
```

#### Variable Declarations - Use for Complex Types
```cpp
// Good - let compiler deduce complex types
auto lambda = [&captured_data](const Request& req) -> Response {
    return process_request(req, captured_data);
};

auto connection_pool = ConnectionPoolBuilder::new()
    .max_connections(100)
    .timeout(Duration::seconds(30))
    .build()?;
```

#### Mixed Usage - Clear Context Separation
```cpp
if (cast(get_user_session()) == AdminSession) {
    // Cast result access
    auto.enable_debug_mode();
    
    // Normal variable declarations
    auto start_time = SystemTime::now();
    auto config = auto.get_admin_config();  // auto.method() for cast, auto var for result
    
    // More cast result operations
    auto.execute_admin_query(config.default_query);
}
```

## VIII. Advanced Usage Patterns

### Nested Cast Conditionals

```cpp
fn handle_nested_access(conn: &runtime Connection) {
    if (cast(conn) == AdminOps) {
        if (cast(auto.get_session()) == SuperuserSession) {
            // auto refers to get_session() result in inner scope
            auto.execute_system_command();
            auto.access_root_privileges();
        } else {
            // auto refers to original conn in outer scope  
            auto.execute_admin_command();
        }
    }
}
```

### Switch with Complex Logic

```cpp
fn process_message(msg: &runtime NetworkMessage) -> ProcessResult {
    switch (cast(msg)) {
        case TextMessage => {
            auto decoded_text = auto.decode_utf8()?;
            auto filtered_text = apply_content_filter(decoded_text);
            return ProcessResult::Text(filtered_text);
        },
        
        case BinaryMessage => {
            if auto.get_format() == ImageFormat::JPEG {
                let processed = auto.process_image()?;
                return ProcessResult::Image(processed);
            } else {
                return ProcessResult::UnsupportedBinary;
            }
        },
        
        case ControlMessage => {
            match auto.get_command() {
                Command::Shutdown => return ProcessResult::Shutdown,
                Command::Restart => return ProcessResult::Restart,
                _ => return ProcessResult::UnknownCommand,
            }
        },
    }
}
```

### Integration with Coroutines

```cpp
async fn handle_async_connection(conn: &runtime Connection) -> ConnectionResult {
    match cast(conn) {
        AdminOps => {
            // Async operations on cast result
            co_await auto.authenticate_admin();
            let privileges = co_await auto.load_privileges();
            co_await auto.setup_admin_environment(privileges);
            
            ConnectionResult::AdminReady
        },
        
        UserOps => {
            co_await auto.authenticate_user();
            co_await auto.apply_user_restrictions();
            
            ConnectionResult::UserReady
        },
    }
}
```

## IX. Error Handling and Debugging

### Cast Failure Debugging

```cpp
fn debug_cast_failures(obj: &runtime SomeType) {
    // Type inspection never fails - always provides information
    println("Object runtime type: {:?}", cast(obj));
    
    // Dynamic cast with detailed error handling
    match dynamic_cast<TargetType>(obj) {
        Some(target) => {
            println("Cast succeeded - processing as TargetType");
            target.process();
        },
        None => {
            println("Cast failed - object is {:?}, not TargetType", cast(obj));
            // Fallback logic or error reporting
        }
    }
}
```

### Compile-Time Cast Validation

```cpp
// Compiler can detect impossible casts at compile time
let user_conn: Connection<UserOps> = create_user_connection();

// This would be a compile warning/error:
let admin = static_cast<AdminOps>(user_conn);  // Warning: impossible cast
let admin = dynamic_cast<AdminOps>(user_conn); // Warning: will always return None
```

### Runtime Type Information

```cpp
fn inspect_runtime_types(objects: &[&runtime SomeInterface]) {
    for obj in objects {
        println("Object type: {:?}", cast(obj));
        
        // Runtime type queries
        if obj.has_capability::<Serializable>() {
            println("  - Can be serialized");
        }
        
        if obj.has_capability::<NetworkSendable>() {
            println("  - Can be sent over network");  
        }
        
        // Capability-based processing
        if let Some(serializable) = dynamic_cast<Serializable>(obj) {
            let data = serializable.serialize();
            send_over_network(data);
        }
    }
}
```

## X. Migration from Other Languages

### From C++

```cpp
// C++ pattern
if (auto* admin = dynamic_cast<AdminOps*>(connection)) {
    admin->delete_table("temp");
}

// CPrime equivalent  
if let Some(admin) = dynamic_cast<AdminOps>(connection) {
    AdminOps::delete_table(&admin, "temp");
}

// Or with cast() for cleaner syntax
if (cast(connection) == AdminOps) {
    auto.delete_table("temp");
}
```

### From Rust

```cpp
// Rust pattern
match connection.downcast_ref::<AdminOps>() {
    Some(admin) => admin.delete_table("temp"),
    None => println!("Not admin connection"),
}

// CPrime equivalent
match dynamic_cast<AdminOps>(connection) {
    Some(admin) => AdminOps::delete_table(&admin, "temp"),
    None => println("Not admin connection"),
}

// Or with cast()
match cast(connection) {
    AdminOps => auto.delete_table("temp"),
    _ => println("Not admin connection"),
}
```

### From Java

```java
// Java pattern
if (connection instanceof AdminConnection) {
    AdminConnection admin = (AdminConnection) connection;
    admin.deleteTable("temp");
}

// CPrime equivalent
if (cast(connection) == AdminOps) {
    auto.delete_table("temp");
}
```

## Summary

CPrime's casting system provides:

1. **C++ Compatibility**: Direct equivalents to `static_cast` and `dynamic_cast`
2. **Enhanced Safety**: Option-based error handling and type inspection
3. **Performance Clarity**: Explicit runtime vs compile-time cost signaling
4. **Syntactic Innovation**: `auto` keyword for cast result access
5. **Clean Integration**: Seamless work with CPrime's type system
6. **Antipattern Prevention**: Prohibited operator overloads eliminate common bugs

This design maintains familiar C++ semantics while adding CPrime-specific improvements for type safety, performance predictability, and developer ergonomics.