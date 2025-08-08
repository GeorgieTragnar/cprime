# CPrime Type System

## Overview

CPrime's type system provides static type safety, zero-cost abstractions, and C++-compatible performance. It explicitly avoids inheritance hierarchies in favor of composition, traits, and access rights for polymorphism.

## Primitive Types

### Integer Types

CPrime provides explicit-width integer types compatible with C++:

```cpp
// Signed integers
i8      // -128 to 127
i16     // -32,768 to 32,767  
i32     // -2,147,483,648 to 2,147,483,647
i64     // -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807

// Unsigned integers
u8      // 0 to 255
u16     // 0 to 65,535
u32     // 0 to 4,294,967,295
u64     // 0 to 18,446,744,073,709,551,615

// Platform-dependent sizes
isize   // Pointer-sized signed integer
usize   // Pointer-sized unsigned integer (for array indices, memory sizes)

// Type inference and literals
let x = 42;          // Infers i32 by default
let y = 42u64;       // Explicit u64
let z: i16 = 42;     // Type annotation
```

### Floating Point Types

```cpp
f32     // IEEE 754 single precision (32-bit)
f64     // IEEE 754 double precision (64-bit)

// Literals and operations
let pi = 3.14159f32;           // Explicit f32
let e = 2.718281828459045;     // Infers f64 by default
let result: f32 = 1.0 / 3.0;   // Type annotation
```

### Other Primitive Types

```cpp
bool    // true or false
char    // Unicode scalar value (4 bytes, UTF-32)

// Boolean operations
let is_valid = true;
let can_proceed = !is_error && is_ready;

// Character literals
let letter = 'A';
let emoji = '🚀';
let unicode = '\u{1F680}';  // Unicode escape
```

## Compound Types

### Arrays and Slices

```cpp
// Fixed-size arrays (stack allocated)
let numbers: [i32; 5] = [1, 2, 3, 4, 5];
let zeros = [0; 10];                       // Array of 10 zeros
let matrix: [[f64; 3]; 3] = [              // 3x3 matrix
    [1.0, 0.0, 0.0],
    [0.0, 1.0, 0.0], 
    [0.0, 0.0, 1.0],
];

// Slices (fat pointers: pointer + length)
let slice: &[i32] = &numbers[1..4];        // References elements 1, 2, 3
let all: &[i32] = &numbers[..];            // References entire array
let mutable_slice: &mut [i32] = &mut numbers[2..];

// Dynamic arrays (heap allocated)
class Vec<T> {
    data: *mut T,
    len: usize,
    capacity: usize,
    constructed_by: VecOps,
}

functional class VecOps<T> {
    fn construct() -> Vec<T> { ... }
    fn with_capacity(cap: usize) -> Vec<T> { ... }
    fn push(vec: &mut Vec<T>, item: T) { ... }
    fn pop(vec: &mut Vec<T>) -> Option<T> { ... }
}
```

### Tuples

```cpp
// Tuple types
let point: (i32, i32) = (10, 20);
let person: (String, u32, bool) = ("Alice".to_string(), 30, true);
let unit: () = ();  // Unit type (empty tuple)

// Destructuring
let (x, y) = point;
let (name, age, _) = person;  // Ignore third element

// Tuple access
let first = point.0;
let second = point.1;

// Nested tuples
let complex: ((i32, i32), (f64, f64)) = ((1, 2), (3.0, 4.0));
let ((a, b), (c, d)) = complex;
```

### Structures (Data Classes)

```cpp
class Point {
    x: f64,
    y: f64,
    z: f64,
}

class Person {
    name: String,
    age: u32,
    email: Option<String>,
    addresses: Vec<Address>,
}

class Address {
    street: String,
    city: String,
    postal_code: String,
    country: String,
}

// Structure initialization
let origin = Point { x: 0.0, y: 0.0, z: 0.0 };
let person = Person {
    name: "John Doe".to_string(),
    age: 35,
    email: Some("john@example.com".to_string()),
    addresses: Vec::new(),
};
```

## Field Modifiers

CPrime provides a sophisticated three-tier field modifier system that controls mutability at the field level:

### 1. `semconst` - Semantic Preservation

The `semconst` modifier preserves semantic values while allowing memory flexibility:

```cpp
class Configuration {
    semconst api_endpoint: String,      // Can only be replaced atomically
    semconst timeout_seconds: u32,      // No partial modification allowed
    mutable connection_pool: Pool,      // Can be modified incrementally
    
    // Type system enforces 1:1 move pattern for semconst
    fn update_endpoint(&mut self, new_endpoint: String) -> String {
        let old = move(self.api_endpoint);        // Must move out
        self.api_endpoint = move(new_endpoint);   // Move in new value
        old                                       // Return old value
    }
}
```

**Type implications of `semconst`:**
- **Cannot call mutating methods**: `self.api_endpoint.push_str("x")` is forbidden
- **Cannot assign to subfields**: `self.api_endpoint.field = value` is forbidden
- **Can only replace entirely**: Must use 1:1 move pattern
- **Swap operations allowed**: Compiler recognizes swaps as permutations

### 2. `mutable` - Explicit Mutability

The `mutable` modifier provides explicit, unrestricted mutability:

```cpp
class Cache {
    mutable entries: HashMap<String, Value>,    // Full mutability
    mutable hit_count: u64,                    // Can increment/modify
    capacity: usize,                           // Default behavior
    
    fn increment_hits(&mut self) {
        self.hit_count += 1;                   // ✓ Direct modification
        self.entries.insert("recent".into(), value); // ✓ Method calls
    }
}
```

### 3. Default Fields

Fields without explicit modifiers have controlled mutability:

```cpp
class User {
    id: u64,                    // Default: privately mutable
    name: String,               // Default: privately mutable
    semconst created_at: u64,   // Explicitly semantic preservation
    mutable login_count: u64,   // Explicitly mutable
    
    // Can modify default fields internally
    fn rename(&mut self, new_name: String) {
        self.id = generate_new_id();    // ✓ Internal modification
        self.name = new_name;           // ✓ Internal modification
    }
}
```

### Field Modifiers in Generic Types

Field modifiers work with generic types and constraints:

```cpp
class Container<T> {
    semconst data: Vec<T>,              // Generic semconst field
    mutable metadata: ContainerMeta,    // Explicit mutable
    
    // Generic methods respect field modifiers
    fn replace_data(&mut self, new_data: Vec<T>) -> Vec<T> {
        let old = move(self.data);      // Must use move pattern
        self.data = move(new_data);     // for semconst fields
        old
    }
    
    fn update_metadata(&mut self, meta: ContainerMeta) {
        self.metadata = meta;           // Direct assignment for mutable
    }
}

// Specialized behavior for specific types
impl Container<String> {
    fn specialized_operation(&mut self) {
        // Still must follow semconst rules for self.data
        let temp = move(self.data);
        let processed = process_strings(temp);
        self.data = move(processed);
    }
}
```

### Field Modifiers with Type Bounds

```cpp
// Require types to have compatible field modifiers
interface ModifiableContent {
    semconst id: u64;           // Must be semconst
    mutable content: String;    // Must be mutable
}

class Document implements ModifiableContent {
    semconst doc_id: u64,       // Maps to semconst id
    mutable text_content: String, // Maps to mutable content
    revision: u32,              // Additional field
    
    // Implementation must respect field modifiers
    fn update_content(&mut self, new_content: String) {
        self.text_content = new_content;  // ✓ Direct modification (mutable)
        self.revision += 1;               // ✓ Default field modification
        // Cannot do: self.doc_id += 1;   // ❌ semconst requires move pattern
    }
}

// Field access
let x_coord = origin.x;
let person_name = &person.name;
```

## Generic Types

### Generic Data Classes

```cpp
// Generic data class
class Container<T> {
    value: T,
    metadata: String,
}

// Multiple type parameters
class Pair<T, U> {
    first: T,
    second: U,
}

// Type constraints
class NumericContainer<T> 
where T: Numeric 
{
    value: T,
    min: T,
    max: T,
}

// Usage
let int_container = Container { value: 42, metadata: "integer".to_string() };
let str_container = Container { value: "hello".to_string(), metadata: "text".to_string() };
let pair = Pair { first: 10, second: "ten".to_string() };
```

### Generic Functional Classes

```cpp
// Generic operations
functional class ContainerOps<T> {
    fn construct(value: T) -> Container<T> {
        Container { value, metadata: "default".to_string() }
    }
    
    fn get_value(container: &Container<T>) -> &T {
        &container.value
    }
    
    fn set_value(container: &mut Container<T>, new_value: T) {
        container.value = new_value;
    }
}

// Specialized operations for specific types
functional class ContainerOps<String> {
    fn get_length(container: &Container<String>) -> usize {
        container.value.len()
    }
    
    fn append(container: &mut Container<String>, suffix: &str) {
        container.value.push_str(suffix);
    }
}

// Usage
let mut container = ContainerOps::construct("hello".to_string());
ContainerOps::append(&mut container, " world");
let length = ContainerOps::get_length(&container);
```

### Type Constraints and Bounds

```cpp
// Trait-like constraints
trait Comparable {
    fn compare(&self, other: &Self) -> Ordering;
}

trait Numeric {
    fn zero() -> Self;
    fn add(&self, other: &Self) -> Self;
    fn subtract(&self, other: &Self) -> Self;
}

// Bounded generics
functional class SortOps<T> 
where T: Comparable 
{
    fn sort(items: &mut [T]) {
        // Implementation using T::compare
    }
    
    fn binary_search(items: &[T], target: &T) -> Option<usize> {
        // Implementation using T::compare
    }
}

functional class MathOps<T>
where T: Numeric + Copy
{
    fn sum(items: &[T]) -> T {
        let mut result = T::zero();
        for item in items {
            result = result.add(item);
        }
        result
    }
}
```

## Union Types (Memory Contracts)

CPrime unions are **memory contracts** that reserve space for a collection of types. Rather than being objects themselves, unions act as memory reservations where actual objects live, integrating seamlessly with CPrime's vtable system.

### Union as Memory Reservation

```cpp
// Union defines memory contract, not object type
union ConnectionSpace {
    UserConn(Connection<UserOps>),      // 40 bytes
    AdminConn(Connection<AdminOps>),    // 48 bytes
    PowerConn(Connection<UserOps, AdminOps>), // 56 bytes
}
// Memory contract: Reserve 56 bytes (largest variant)
// No "union object" exists - just properly-sized memory space

// You work with objects IN the union space
fn process_connection(space: &ConnectionSpace) {
    // Access actual objects living in union space
    if let Some(user) = space.try_as::<Connection<UserOps>>() {
        UserOps::query(&user, "SELECT ...");  // Normal object method
    } else if let Some(admin) = space.try_as::<Connection<AdminOps>>() {
        AdminOps::delete_table(&admin, "temp");  // Normal object method
    }
}
```

### Compile-Time vs Runtime Unions

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
        Message::Data { payload } => process_data(payload),
        Message::Ping => send_pong(),
    }
}
```

**Runtime Unions** integrate with the vtable system:

```cpp
union runtime DynamicSpace {
    TextMsg(runtime TextMessage),
    BinaryMsg(runtime BinaryMessage),
}

// No pattern matching - use existing dynamic casting API
fn handle_dynamic(space: &DynamicSpace) {
    if let Some(text) = space.try_as::<TextMessage>() {
        text.process();  // Normal method call via vtable
    } else if let Some(binary) = space.try_as::<BinaryMessage>() {
        binary.decode();  // Normal method call via vtable
    }
}
```

### Template Integration and Self-Expansion

Unions enable powerful template integration with automatic expansion:

```cpp
// Self-expanding heterogeneous container
let mut items: Vec<runtime Any> = Vec::new();

// Behind the scenes, compiler maintains growing union:
items.push(SmallItem{data: 42});          // Union: {SmallItem} - 8 bytes
items.push(MediumItem{data: [1,2,3,4]});  // Union: {Small, Medium} - 16 bytes -> REALLOC!
items.push(HugeItem{data: [0; 1024]});    // Union: {Small, Medium, Huge} - 1024 bytes -> REALLOC!

// Access is seamless via unified API
for item in &items {
    if let Some(processable) = item.try_as::<dyn Processable>() {
        processable.process();  // Interface call via vtable
    }
}
```

**Key Constraint**: Self-expanding containers require `runtime` objects (with vtables) for type identification and dynamic access.

### Memory Layout Strategy

Union memory layout depends on variant type:

**Compile-Time Unions** (with discriminant):
```cpp
union NetworkPacket {
    Data { header: Header, payload: [u8; 1024] },    // 1040 bytes
    Control { header: Header, command: Command },     // 20 bytes  
    Ack { header: Header, sequence: u32 },           // 20 bytes
}
// Layout: [discriminant][padding][data: max(variants)]
// Size: 1 + 7 + 1040 = 1048 bytes
```

**Runtime Unions** (vtable-based):
```cpp
union runtime ObjectSpace {
    SmallObj(runtime SmallObject),     // 32 bytes with vtable
    LargeObj(runtime LargeObject),     // 1024 bytes with vtable
}
// Layout: [data: max(variants)] - no separate discriminant!
// Type identification via existing vtable system  
// Size: 1024 bytes (just the largest variant)
```

**Unified Type System**: Runtime unions eliminate duplicate type tracking by reusing vtable infrastructure.

For detailed union documentation, see [unions.md](unions.md).

## Option and Result Types

### Option Type for Nullable Values

```cpp
// Option represents optional values
union Option<T> {
    Some(T),
    None,
}

// Usage
let some_number: Option<i32> = Option::Some(42);
let no_number: Option<i32> = Option::None;

// Pattern matching
match some_number {
    Option::Some(value) => println("Got value: {}", value),
    Option::None => println("No value"),
}

// Convenient methods
if let Option::Some(value) = some_number {
    println("Value is {}", value);
}

let default_value = some_number.unwrap_or(0);
let doubled = some_number.map(|x| x * 2);
```

### Result Type for Error Handling

```cpp
// Result represents success or failure
union Result<T, E> {
    Ok(T),
    Err(E),
}

// Function that can fail
fn divide(a: f64, b: f64) -> Result<f64, String> {
    if b == 0.0 {
        Result::Err("Division by zero".to_string())
    } else {
        Result::Ok(a / b)
    }
}

// Error handling
match divide(10.0, 2.0) {
    Result::Ok(result) => println("Result: {}", result),
    Result::Err(error) => println("Error: {}", error),
}

// Question mark operator for error propagation
fn calculate() -> Result<f64, String> {
    let x = divide(10.0, 2.0)?;  // Propagates error if Err
    let y = divide(x, 3.0)?;     // Continues if Ok
    Result::Ok(y + 1.0)
}
```

## Unions

### Tagged Sum Types

CPrime unions provide tagged sum types with pattern matching, offering a powerful alternative to inheritance-based polymorphism:

```cpp
// Basic union definition
union Message {
    Connect { addr: SocketAddr, timeout: Duration },
    Data { payload: Vec<u8>, compressed: bool },
    Disconnect { reason: String },
    Ping,
    Pong,
}

// Generic unions
union Option<T> {
    Some(T),
    None,
}

union Result<T, E> {
    Ok(T),
    Err(E),
}
```

### Pattern Matching

Unions are consumed through pattern matching with exhaustiveness checking:

```cpp
fn handle_message(msg: Message) {
    match msg {
        Message::Connect { addr, timeout } => {
            println("Connecting to {} with timeout {:?}", addr, timeout);
        },
        Message::Data { payload, compressed } => {
            if compressed {
                process_compressed_data(payload);
            } else {
                process_data(payload);
            }
        },
        Message::Disconnect { reason } => {
            println("Disconnecting: {}", reason);
        },
        Message::Ping => send_pong(),
        Message::Pong => update_heartbeat(),
    }
}

// Compiler ensures all variants are handled
fn get_status(msg: &Message) -> &str {
    match msg {
        Message::Connect { .. } => "connecting",
        Message::Data { .. } => "transferring",
        Message::Disconnect { .. } => "disconnected",
        // Compile error: Missing patterns Ping, Pong
    }
}
```

### Compile-Time vs Runtime Unions

```cpp
// Compile-time unions (default) - zero overhead when variant is known
fn create_error() -> Result<i32, String> {
    Result::Err("Error".to_string())  // Compiler knows it's Err
}

// Runtime unions - allow dynamic variant addition
union runtime DynamicMessage {
    Text(String),
    Binary(Vec<u8>),
    Json(JsonValue),
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

### Union Memory Layout

Unions store the largest variant plus a discriminant tag:

```cpp
union FileResult {
    Success { data: Vec<u8>, size: usize },    // ~32 bytes
    NotFound,                                   // 0 bytes
    PermissionDenied { reason: String },       // ~24 bytes
}

// Memory layout: max(variants) + tag + alignment
// Size: 32 bytes (data) + 1 byte (tag) + 7 bytes (padding) = 40 bytes
```

For more comprehensive union documentation, see [unions.md](unions.md).

## Traits and Interfaces

### Trait Definition and Implementation

```cpp
// Trait definition
trait Display {
    fn fmt(&self) -> String;
}

trait Clone {
    fn clone(&self) -> Self;
}

trait Iterator {
    type Item;
    fn next(&mut self) -> Option<Self::Item>;
}

// Trait implementation for data classes
impl Display for Point {
    fn fmt(&self) -> String {
        format!("Point({}, {}, {})", self.x, self.y, self.z)
    }
}

impl Clone for Point {
    fn clone(&self) -> Self {
        Point { x: self.x, y: self.y, z: self.z }
    }
}

// Generic trait implementations
impl<T: Display> Display for Container<T> {
    fn fmt(&self) -> String {
        format!("Container[{}]: {}", self.metadata, self.value.fmt())
    }
}
```

### Associated Types and Functions

```cpp
trait Collection {
    type Item;
    type Iterator: Iterator<Item = Self::Item>;
    
    fn len(&self) -> usize;
    fn is_empty(&self) -> bool { self.len() == 0 }
    fn iter(&self) -> Self::Iterator;
}

trait Default {
    fn default() -> Self;
}

// Implementation
impl Default for Point {
    fn default() -> Self {
        Point { x: 0.0, y: 0.0, z: 0.0 }
    }
}

// Usage
let default_point = Point::default();
```

## No Inheritance - Composition Instead

### Why No Inheritance

CPrime explicitly avoids inheritance hierarchies:

```cpp
// ❌ NO inheritance in CPrime
// class Animal { ... }
// class Dog : public Animal { ... }  // Not allowed

// ✓ Use composition instead
class AnimalData {
    species: String,
    age: u32,
    weight: f64,
}

class DogData {
    animal: AnimalData,
    breed: String,
    is_good_boy: bool,
}

functional class DogOps {
    fn construct(breed: &str, age: u32, weight: f64) -> DogData {
        DogData {
            animal: AnimalData {
                species: "Canis lupus".to_string(),
                age,
                weight,
            },
            breed: breed.to_string(),
            is_good_boy: true,
        }
    }
    
    fn bark(dog: &DogData) -> String {
        format!("Woof! I'm a {} and I'm {} years old!", dog.breed, dog.animal.age)
    }
}
```

### Polymorphism Through Access Rights

Instead of virtual methods, use access rights for polymorphism:

```cpp
// Data can be accessed through different interfaces
class MediaData {
    content: Vec<u8>,
    format: MediaFormat,
    metadata: HashMap<String, String>,
    
    // Different access patterns
    exposes AudioOps { content, metadata }
    exposes VideoOps { content, metadata }
    exposes ImageOps { content, metadata }
}

functional class AudioOps {
    fn play(media: &MediaData) -> Result<()> {
        // Audio-specific implementation
    }
    
    fn get_duration(media: &MediaData) -> Duration {
        // Parse audio metadata
    }
}

functional class VideoOps {
    fn play(media: &MediaData) -> Result<()> {
        // Video-specific implementation
    }
    
    fn get_resolution(media: &MediaData) -> (u32, u32) {
        // Parse video metadata
    }
}

// Polymorphic behavior through access rights
fn process_media(media: &MediaData, format: MediaFormat) {
    match format {
        MediaFormat::Audio => AudioOps::play(media),
        MediaFormat::Video => VideoOps::play(media),
        MediaFormat::Image => ImageOps::display(media),
    }
}
```

## Type Aliases and Newtype Pattern

### Type Aliases

```cpp
// Simple type aliases
type UserId = u64;
type Temperature = f64;
type Coordinates = (f64, f64);

// Generic type aliases
type Result<T> = Result<T, Error>;
type HashMap<K, V> = std::collections::HashMap<K, V>;

// Complex type aliases
type EventHandler = fn(Event) -> Result<()>;
type AsyncTask = Box<dyn Future<Output = Result<()>>>;

// Usage
let user: UserId = 12345;
let temp: Temperature = 23.5;
let coords: Coordinates = (40.7128, -74.0060);
```

### Newtype Pattern for Type Safety

```cpp
// Distinct types that wrap primitives
class UserId {
    value: u64,
}

class ProductId {
    value: u64,
}

class Price {
    cents: u64,  // Store price in cents to avoid floating point issues
}

functional class UserIdOps {
    fn construct(id: u64) -> UserId {
        UserId { value: id }
    }
    
    fn to_u64(user_id: &UserId) -> u64 {
        user_id.value
    }
}

// Type safety prevents mixing different ID types
fn get_user(user_id: UserId) -> Result<User> { ... }
fn get_product(product_id: ProductId) -> Result<Product> { ... }

// ✓ Correct usage
let user_id = UserIdOps::construct(123);
let user = get_user(user_id)?;

// ❌ Compile error - cannot pass ProductId where UserId expected
// let product_id = ProductIdOps::construct(456);
// let user = get_user(product_id)?;  // Type error
```

## Coroutine Type Integration

CPrime's type system seamlessly integrates with the revolutionary coroutine design, providing compiler-guided optimization and type safety:

### Coroutines as Typed Data Classes

Coroutines follow CPrime's type system principles as specialized data classes:

```cpp
// Coroutine types are parameterized by size and capabilities
class HttpHandlerCoro<Size, Capabilities> 
where Size: CoroSizeTag, Capabilities: AccessRights
{
    // Stack memory typed by size class
    stack_memory: StackMemory<Size>,
    register_context: RegisterContext,
    
    // Application state
    request: HttpRequest,
    response: HttpResponse,
    
    // Capability-based access (type-safe)
    access_rights: Capabilities,
    
    constructed_by: CoroManager<Size, Capabilities>,
}

// Type aliases for common coroutine configurations
type MicroHttpCoro = HttpHandlerCoro<Micro, HttpOps>;
type SecureHttpCoro = HttpHandlerCoro<Small, HttpOps + TlsOps>;
type FullHttpCoro = HttpHandlerCoro<Medium, HttpOps + TlsOps + CacheOps>;

// Compiler generates optimal memory layout for each type
// MicroHttpCoro: 256 bytes total (stack pool allocation)
// SecureHttpCoro: 2KB total (heap pool allocation)
// FullHttpCoro: 16KB total (heap pool allocation)
```

### Compiler-Generated Type Metadata

The compiler generates rich type metadata for coroutine functions:

```cpp
// Compiler analyzes this function and generates metadata
#[coro_analysis(
    estimated_stack_size = 640,
    max_stack_size = 1024,
    is_bounded = true,
    recursion_depth = 0,
    size_class = CoroSizeTag::Small
)]
async fn web_request_handler(request: HttpRequest) -> HttpResponse {
    let headers = parse_headers(&request.headers);        // +128 bytes stack
    let auth_result = co_await authenticate(&headers);    // +256 bytes stack
    let response = build_response(&auth_result);          // +192 bytes stack
    response
}

// Metadata is encoded as type-level information
struct CoroFunctionType<F> {
    function: F,
    metadata: CoroMetadata,
}

impl CoroFunction for typeof(web_request_handler) {
    type Args = (HttpRequest,);
    type Output = HttpResponse;
    type SizeClass = Small;
    type AccessRights = HttpOps;
    
    const METADATA: CoroMetadata = CoroMetadata {
        estimated_stack_size: 640,
        max_stack_size: Some(1024),
        is_bounded: true,
        size_class: CoroSizeTag::Small,
        // ... other metadata
    };
}
```

### Generic Coroutine Types

Coroutines work naturally with CPrime's generic type system:

```cpp
// Generic coroutine type
class GenericCoro<T, R, Size, Caps>
where 
    T: Send,                    // Input type must be sendable between threads
    R: Send,                    // Output type must be sendable
    Size: CoroSizeTag,          // Size class
    Caps: AccessRights          // Capability requirements
{
    input: T,
    output: Option<R>,
    stack_memory: StackMemory<Size>,
    capabilities: Caps,
    
    constructed_by: GenericCoroManager<T, R, Size, Caps>,
}

// Generic functional class for coroutine operations
functional class GenericCoroManager<T, R, Size, Caps> {
    fn construct(input: T) -> GenericCoro<T, R, Size, Caps> {
        let stack = allocate_stack_for_size_class(Size::SIZE_CLASS);
        
        GenericCoro {
            input,
            output: None,
            stack_memory: StackMemory::<Size>::from_raw(stack),
            capabilities: Caps::default(),
        }
    }
    
    fn resume(coro: &mut GenericCoro<T, R, Size, Caps>) -> CoroResult<R> {
        // Type-safe resume with generic input/output
        context_switch_typed::<T, R>(coro.stack_memory, &coro.input)
    }
}

// Specialized instances with type inference
let http_coro: GenericCoro<HttpRequest, HttpResponse, Small, HttpOps> = 
    GenericCoroManager::construct(request);

let db_coro: GenericCoro<SqlQuery, QueryResult, Medium, DbOps> = 
    GenericCoroManager::construct(query);
```

### Union Types for Coroutine Size Classes

Union types provide type-safe heterogeneous coroutine storage:

```cpp
// Union type for different coroutine sizes with preserved type information
union CoroSizeVariant<Input, Output, Caps> 
where Caps: AccessRights
{
    Micro(GenericCoro<Input, Output, Micro, Caps>),
    Small(GenericCoro<Input, Output, Small, Caps>),
    Medium(GenericCoro<Input, Output, Medium, Caps>),
    Large(GenericCoro<Input, Output, Large, Caps>),
}

// Type-safe operations on union variants
functional class CoroSizeManager<I, O, C> {
    fn resume_any(variant: &mut CoroSizeVariant<I, O, C>) -> CoroResult<O> {
        match variant {
            CoroSizeVariant::Micro(micro) => {
                // Compiler knows this is GenericCoro<I, O, Micro, C>
                GenericCoroManager::<I, O, Micro, C>::resume(micro)
            },
            CoroSizeVariant::Small(small) => {
                GenericCoroManager::<I, O, Small, C>::resume(small)
            },
            CoroSizeVariant::Medium(medium) => {
                GenericCoroManager::<I, O, Medium, C>::resume(medium)
            },
            CoroSizeVariant::Large(large) => {
                GenericCoroManager::<I, O, Large, C>::resume(large)
            },
        }
    }
    
    fn migrate_up(variant: &mut CoroSizeVariant<I, O, C>) -> Result<()> {
        // Type-safe migration between size classes
        let new_variant = match variant {
            CoroSizeVariant::Micro(micro) => {
                let migrated = migrate_micro_to_small(micro)?;
                CoroSizeVariant::Small(migrated)
            },
            CoroSizeVariant::Small(small) => {
                let migrated = migrate_small_to_medium(small)?;
                CoroSizeVariant::Medium(migrated)
            },
            CoroSizeVariant::Medium(medium) => {
                let migrated = migrate_medium_to_large(medium)?;
                CoroSizeVariant::Large(migrated)
            },
            CoroSizeVariant::Large(_) => {
                return Err("Already at largest size class");
            },
        };
        
        *variant = new_variant;
        Ok(())
    }
}

// Usage with full type safety
let mut http_variant: CoroSizeVariant<HttpRequest, HttpResponse, HttpOps> = 
    CoroSizeVariant::Small(create_http_handler());

let response = CoroSizeManager::resume_any(&mut http_variant)?;
// Compiler knows response is HttpResponse due to type parameters
```

### Phantom Types for Compile-Time Coroutine States

Phantom types track coroutine lifecycle states at compile time:

```cpp
// Phantom types for coroutine states
struct Created;
struct Running;
struct Suspended;
struct Completed;

// Stateful coroutine type with phantom state parameter
class StatefulCoro<T, R, Size, Caps, State>
where State: CoroState
{
    base: GenericCoro<T, R, Size, Caps>,
    _state: PhantomData<State>,
}

functional class StatefulCoroOps {
    // Type-safe state transitions
    fn create<T, R, Size, Caps>(input: T) -> StatefulCoro<T, R, Size, Caps, Created> {
        StatefulCoro {
            base: GenericCoroManager::construct(input),
            _state: PhantomData,
        }
    }
    
    fn start<T, R, Size, Caps>(
        coro: StatefulCoro<T, R, Size, Caps, Created>
    ) -> StatefulCoro<T, R, Size, Caps, Running> {
        // Can only start a created coroutine
        StatefulCoro {
            base: coro.base,
            _state: PhantomData,
        }
    }
    
    fn suspend<T, R, Size, Caps>(
        coro: StatefulCoro<T, R, Size, Caps, Running>
    ) -> StatefulCoro<T, R, Size, Caps, Suspended> {
        // Can only suspend a running coroutine
        StatefulCoro {
            base: coro.base,
            _state: PhantomData,
        }
    }
    
    fn resume<T, R, Size, Caps>(
        coro: StatefulCoro<T, R, Size, Caps, Suspended>
    ) -> Result<StatefulCoro<T, R, Size, Caps, Running>, StatefulCoro<T, R, Size, Caps, Completed>> {
        // Can only resume a suspended coroutine
        match GenericCoroManager::resume(&mut coro.base) {
            CoroResult::Running => Ok(StatefulCoro {
                base: coro.base,
                _state: PhantomData,
            }),
            CoroResult::Completed(_) => Err(StatefulCoro {
                base: coro.base,
                _state: PhantomData,
            }),
            CoroResult::Error(e) => panic!("Coroutine error: {}", e),
        }
    }
}

// Compile-time state machine enforcement
let coro = StatefulCoroOps::create(http_request);       // Created
let coro = StatefulCoroOps::start(coro);                // Running
let coro = StatefulCoroOps::suspend(coro);              // Suspended
let coro = StatefulCoroOps::resume(coro)?;              // Running or Completed
// StatefulCoroOps::start(coro);                        // ❌ Compile error!
```

### Associated Types for Coroutine Traits

Coroutines work with associated types for flexible generic programming:

```cpp
trait CoroProcessor {
    type Input;
    type Output;
    type SizeClass: CoroSizeTag;
    type Capabilities: AccessRights;
    
    fn process_async(input: Self::Input) -> impl Future<Output = Self::Output>;
    fn size_hint() -> Self::SizeClass;
    fn required_capabilities() -> Self::Capabilities;
}

// Implementation for HTTP processing
struct HttpProcessor;

impl CoroProcessor for HttpProcessor {
    type Input = HttpRequest;
    type Output = HttpResponse;
    type SizeClass = Small;
    type Capabilities = HttpOps;
    
    async fn process_async(request: Self::Input) -> Self::Output {
        // Implementation using appropriate size class and capabilities
        let coro: GenericCoro<Self::Input, Self::Output, Self::SizeClass, Self::Capabilities> = 
            GenericCoroManager::construct(request);
        
        GenericCoroManager::resume(&mut coro)?.into_output()
    }
    
    fn size_hint() -> Self::SizeClass {
        Small::default()
    }
    
    fn required_capabilities() -> Self::Capabilities {
        HttpOps::default()
    }
}

// Generic function works with any processor
fn process_with_processor<P: CoroProcessor>(input: P::Input) -> P::Output {
    // Compiler generates optimal code based on associated types
    P::process_async(input).await
}
```

### Type-Level Size Calculations

The type system can encode stack size calculations at the type level:

```cpp
// Type-level arithmetic for stack size calculations
trait StackSize {
    const SIZE: usize;
}

struct LocalVarSize<T>(PhantomData<T>);

impl<T> StackSize for LocalVarSize<T> {
    const SIZE: usize = std::mem::size_of::<T>();
}

// Compose stack sizes at type level
struct StackFrame<T1, T2, T3> {
    _t1: PhantomData<T1>,
    _t2: PhantomData<T2>, 
    _t3: PhantomData<T3>,
}

impl<T1, T2, T3> StackSize for StackFrame<T1, T2, T3>
where 
    T1: StackSize,
    T2: StackSize,
    T3: StackSize,
{
    const SIZE: usize = T1::SIZE + T2::SIZE + T3::SIZE;
}

// Use in coroutine function analysis
async fn typed_handler() -> HttpResponse {
    let headers: HttpHeaders = parse_headers();     // LocalVarSize<HttpHeaders>
    let auth: AuthResult = authenticate().await;    // LocalVarSize<AuthResult>
    let response: HttpResponse = build_response();  // LocalVarSize<HttpResponse>
    
    response
}

// Compiler can calculate exact stack size at type level
type HandlerFrame = StackFrame<
    LocalVarSize<HttpHeaders>,    // 128 bytes
    LocalVarSize<AuthResult>,     // 64 bytes
    LocalVarSize<HttpResponse>    // 256 bytes
>;
// Total: 448 bytes - fits in Small (2KB) size class

const STACK_SIZE: usize = HandlerFrame::SIZE;  // 448
```

### Memory Layout Optimization

The type system enables precise memory layout control for coroutines:

```cpp
// Explicit memory layout control
#[repr(C)]
class OptimizedCoro {
    // Hot fields first (cache line optimization)
    state: CoroState,           // 1 byte
    priority: Priority,         // 1 byte  
    _padding1: [u8; 6],         // Align to 8 bytes
    
    // Frequently accessed pointers
    stack_ptr: *mut u8,         // 8 bytes
    stack_top: *mut u8,         // 8 bytes
    
    // Less frequently accessed data
    metadata: CoroMetadata,     // 32 bytes
    debug_info: DebugInfo,      // 64 bytes
    
    constructed_by: OptimizedCoroManager,
}

// Packed coroutine for memory-constrained environments
#[repr(packed)]
class PackedMicroCoro {
    stack_memory: [u8; 252],    // 252 bytes stack
    state: CoroState,           // 1 byte
    priority: Priority,         // 1 byte
    context_switches: u16,      // 2 bytes
    // Total: exactly 256 bytes
}

// Compiler verifies size constraints
static_assert!(std::mem::size_of::<PackedMicroCoro>() == 256);
```

This type system integration enables CPrime's coroutines to achieve:
- **Compile-time optimization**: Stack size analysis and allocation strategy selection
- **Type safety**: Statically verified coroutine states and capability usage
- **Memory efficiency**: Precise layout control and size class optimization
- **Zero-cost abstractions**: Generic types compile to optimal assembly
- **Integration**: Seamless work with CPrime's three-class system and polymorphism

## Advanced Type Features

### Associated Constants

```cpp
trait MathConstants {
    const PI: f64 = 3.141592653589793;
    const E: f64 = 2.718281828459045;
}

class Circle {
    radius: f64,
}

impl MathConstants for Circle {}

functional class CircleOps {
    fn area(circle: &Circle) -> f64 {
        Circle::PI * circle.radius * circle.radius
    }
    
    fn circumference(circle: &Circle) -> f64 {
        2.0 * Circle::PI * circle.radius
    }
}
```

### Type-Level Programming

```cpp
// Phantom types for compile-time state tracking
class StateMachine<S> {
    data: InternalData,
    _state: PhantomData<S>,
}

class Initialized;
class Running;
class Stopped;

functional class StateMachineOps {
    fn construct() -> StateMachine<Initialized> {
        StateMachine {
            data: InternalData::new(),
            _state: PhantomData,
        }
    }
    
    fn start(machine: StateMachine<Initialized>) -> StateMachine<Running> {
        // Implementation
        StateMachine {
            data: machine.data,
            _state: PhantomData,
        }
    }
    
    fn stop(machine: StateMachine<Running>) -> StateMachine<Stopped> {
        // Implementation
        StateMachine {
            data: machine.data,
            _state: PhantomData,
        }
    }
}

// Usage enforces state transitions at compile time
let machine = StateMachineOps::construct();        // Initialized
let machine = StateMachineOps::start(machine);     // Running
let machine = StateMachineOps::stop(machine);      // Stopped
// StateMachineOps::start(machine);                // ❌ Error: can't start stopped machine
```

## Memory Layout and Performance

### Zero-Cost Abstractions

CPrime's type system compiles to efficient machine code:

```cpp
// High-level code
class Point3D {
    x: f64,
    y: f64, 
    z: f64,
}

functional class Point3DOps {
    fn distance(p1: &Point3D, p2: &Point3D) -> f64 {
        let dx = p1.x - p2.x;
        let dy = p1.y - p2.y;
        let dz = p1.z - p2.z;
        (dx*dx + dy*dy + dz*dz).sqrt()
    }
}

// Compiles to efficient assembly equivalent to:
// struct Point3D { double x, y, z; };
// double distance(const Point3D* p1, const Point3D* p2) {
//     double dx = p1->x - p2->x;
//     double dy = p1->y - p2->y; 
//     double dz = p1->z - p2->z;
//     return sqrt(dx*dx + dy*dy + dz*dz);
// }
```

### Predictable Memory Layout

```cpp
// Memory layout is predictable and C-compatible
#[repr(C)]
class NetworkPacket {
    header: PacketHeader,   // Offset 0
    length: u32,           // Offset 16 (after header)
    data: [u8; 1024],      // Offset 20
}

// Packed structures for wire protocols
#[repr(packed)]
class BinaryHeader {
    magic: u32,     // 4 bytes
    version: u16,   // 2 bytes
    flags: u8,      // 1 byte
    checksum: u8,   // 1 byte
    // Total: 8 bytes with no padding
}
```

The type system provides strong static guarantees while maintaining zero-cost abstraction and C++ compatibility for maximum performance.