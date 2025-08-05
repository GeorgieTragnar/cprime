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