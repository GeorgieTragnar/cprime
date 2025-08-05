# CPrime Interfaces

## Overview

CPrime interfaces provide common vtable contracts that supplement the access rights system. While access rights provide inheritance-like polymorphism with explicit memory costs, interfaces offer a way to define shared operations across different types without requiring casting. They serve as the middle tier in CPrime's three-level polymorphism system, bridging the gap between access rights (inheritance-like) and unions (pattern matching).

## Core Concepts

### Interfaces vs Access Rights

Interfaces and access rights serve different purposes in CPrime's polymorphism system:

```cpp
// Access rights: Inheritance-like with memory cost
class Connection {
    handle: DbHandle,
    
    exposes UserOps { handle }     // +8 bytes vtable
    exposes AdminOps { handle }    // +8 bytes vtable
}

// Interface: Common contract without memory overhead per implementation
interface Queryable {
    fn execute_query(&self, sql: &str) -> Result<QueryResult>;
    fn get_connection_info(&self) -> ConnectionInfo;
}

// Access rights implement interfaces
impl Queryable for Connection<UserOps> {
    fn execute_query(&self, sql: &str) -> Result<QueryResult> {
        UserOps::execute_limited_query(self, sql)
    }
    
    fn get_connection_info(&self) -> ConnectionInfo {
        UserOps::get_basic_info(self)
    }
}

impl Queryable for Connection<AdminOps> {
    fn execute_query(&self, sql: &str) -> Result<QueryResult> {
        AdminOps::execute_any_query(self, sql)  // More privileges
    }
    
    fn get_connection_info(&self) -> ConnectionInfo {
        AdminOps::get_detailed_info(self)
    }
}
```

### Benefits of Interfaces

1. **No casting required**: Work with any implementation through the interface
2. **Type safety**: Compiler ensures all methods are implemented
3. **Generic programming**: Write functions that work with any implementer
4. **Multiple implementations**: Different types can implement the same interface

## Interface Definition

### Basic Interface Syntax

```cpp
// Simple interface
interface Display {
    fn fmt(&self) -> String;
}

// Interface with associated types
interface Iterator {
    type Item;
    fn next(&mut self) -> Option<Self::Item>;
}

// Interface with default implementations
interface Readable {
    fn read(&mut self, buf: &mut [u8]) -> Result<usize>;
    
    // Default implementation using read
    fn read_to_string(&mut self) -> Result<String> {
        let mut buffer = Vec::new();
        let mut chunk = [0u8; 1024];
        
        loop {
            match self.read(&mut chunk)? {
                0 => break,
                n => buffer.extend_from_slice(&chunk[..n]),
            }
        }
        
        Ok(String::from_utf8(buffer)?)
    }
}

// Interface with constants
interface MathConstants {
    const PI: f64 = 3.141592653589793;
    const E: f64 = 2.718281828459045;
    
    fn calculate_area(&self) -> f64;
}
```

### Generic Interfaces

```cpp
// Generic interface
interface Container<T> {
    fn len(&self) -> usize;
    fn is_empty(&self) -> bool { self.len() == 0 }
    fn get(&self, index: usize) -> Option<&T>;
    fn insert(&mut self, item: T);
}

// Specialized implementations
impl Container<i32> for Vec<i32> {
    fn len(&self) -> usize { self.len() }
    fn get(&self, index: usize) -> Option<&i32> { self.get(index) }
    fn insert(&mut self, item: i32) { self.push(item); }
}

impl<T> Container<T> for LinkedList<T> {
    fn len(&self) -> usize { self.len() }
    fn get(&self, index: usize) -> Option<&T> { 
        self.iter().nth(index) 
    }
    fn insert(&mut self, item: T) { self.push_back(item); }
}
```

## Interface Implementation

### Implementing for Data Classes

```cpp
class Point {
    x: f64,
    y: f64,
    z: f64,
}

impl Display for Point {
    fn fmt(&self) -> String {
        format!("Point({}, {}, {})", self.x, self.y, self.z)
    }
}

impl MathConstants for Point {
    fn calculate_area(&self) -> f64 {
        // Interpret as sphere with radius from origin
        let radius = (self.x * self.x + self.y * self.y + self.z * self.z).sqrt();
        4.0 * Self::PI * radius * radius
    }
}
```

### Implementing for Access Rights

```cpp
class FileData {
    handle: FileHandle,
    path: String,
    
    exposes ReadOps { handle, path }
    exposes WriteOps { handle }
}

// Different access rights provide different interface implementations
impl Readable for FileData<ReadOps> {
    fn read(&mut self, buf: &mut [u8]) -> Result<usize> {
        ReadOps::read_bytes(self, buf)
    }
}

impl Display for FileData<ReadOps> {
    fn fmt(&self) -> String {
        format!("ReadableFile({})", ReadOps::get_path(self))
    }
}

impl Display for FileData<WriteOps> {
    fn fmt(&self) -> String {
        format!("WritableFile({})", WriteOps::get_path(self))
    }
}
```

### Conditional Implementations

```cpp
// Implement interface only if certain conditions are met
impl<T> Display for Vec<T> 
where T: Display 
{
    fn fmt(&self) -> String {
        let items: Vec<String> = self.iter()
            .map(|item| item.fmt())
            .collect();
        format!("[{}]", items.join(", "))
    }
}

// Implement for access rights with specific capabilities
impl<A> Queryable for Connection<A>
where A: DatabaseAccess
{
    fn execute_query(&self, sql: &str) -> Result<QueryResult> {
        A::execute(self, sql)
    }
    
    fn get_connection_info(&self) -> ConnectionInfo {
        A::get_info(self)
    }
}
```

## Using Interfaces

### Static Dispatch

```cpp
// Function that works with any Display implementer
fn print_item<T>(item: &T) 
where T: Display 
{
    println!("{}", item.fmt());
}

// Compiler generates specialized versions
let point = Point { x: 1.0, y: 2.0, z: 3.0 };
let vector = vec![1, 2, 3, 4];

print_item(&point);  // Uses Point's implementation
print_item(&vector); // Uses Vec<i32>'s implementation
```

### Dynamic Dispatch

```cpp
// Trait objects for runtime polymorphism
fn print_items(items: &[&dyn Display]) {
    for item in items {
        println!("{}", item.fmt());  // Dynamic dispatch
    }
}

// Mixed types through trait objects
let point = Point { x: 1.0, y: 2.0, z: 3.0 };
let text = "Hello World";
let number = 42;

let displayable: &[&dyn Display] = &[&point, &text, &number];
print_items(displayable);
```

### Interface Objects

```cpp
// Interface objects for heterogeneous collections
class ServiceRegistry {
    services: Vec<Box<dyn Queryable>>,
    constructed_by: RegistryOps,
}

functional class RegistryOps {
    fn construct() -> ServiceRegistry {
        ServiceRegistry {
            services: Vec::new(),
        }
    }
    
    fn add_service(registry: &mut ServiceRegistry, service: Box<dyn Queryable>) {
        registry.services.push(service);
    }
    
    fn query_all(registry: &ServiceRegistry, sql: &str) -> Vec<Result<QueryResult>> {
        registry.services
            .iter()
            .map(|service| service.execute_query(sql))
            .collect()
    }
}
```

## Advanced Interface Features

### Associated Types

```cpp
interface GraphTraversal {
    type Node;
    type Edge;
    type Iterator: Iterator<Item = Self::Node>;
    
    fn neighbors(&self, node: &Self::Node) -> Self::Iterator;
    fn get_edge(&self, from: &Self::Node, to: &Self::Node) -> Option<&Self::Edge>;
}

// Implementation specifies concrete types
impl GraphTraversal for AdjacencyList {
    type Node = NodeId;
    type Edge = EdgeWeight;
    type Iterator = NeighborIterator;
    
    fn neighbors(&self, node: &Self::Node) -> Self::Iterator {
        NeighborIterator::new(self, *node)
    }
    
    fn get_edge(&self, from: &Self::Node, to: &Self::Node) -> Option<&Self::Edge> {
        self.edges.get(&(*from, *to))
    }
}
```

### Higher-Ranked Trait Bounds

```cpp
interface Closure<Args> {
    type Output;
    fn call(&self, args: Args) -> Self::Output;
}

// Function that works with any closure
fn apply_twice<F, T>(f: F, value: T) -> T
where 
    F: Closure<T, Output = T>
{
    let intermediate = f.call(value);
    f.call(intermediate)
}
```

### Interface Composition

```cpp
// Compose multiple interfaces
interface ReadWrite: Readable + Writable {
    fn copy_from(&mut self, source: &mut impl Readable) -> Result<u64> {
        let mut buffer = [0u8; 8192];
        let mut total = 0;
        
        loop {
            let bytes_read = source.read(&mut buffer)?;
            if bytes_read == 0 { break; }
            
            self.write_all(&buffer[..bytes_read])?;
            total += bytes_read as u64;
        }
        
        Ok(total)
    }
}

// Automatically implemented for types that implement both traits
impl<T> ReadWrite for T 
where T: Readable + Writable 
{}
```

## Interfaces with Access Rights Integration

### Access Rights Implementing Common Interfaces

```cpp
// Common interface across different access levels
interface DatabaseOperation {
    fn execute(&self, query: &str) -> Result<QueryResult>;
    fn get_permissions(&self) -> PermissionSet;
}

// Different access rights provide different implementations
impl DatabaseOperation for Connection<UserOps> {
    fn execute(&self, query: &str) -> Result<QueryResult> {
        if is_safe_query(query) {
            UserOps::execute_query(self, query)
        } else {
            Err("Permission denied: unsafe query")
        }
    }
    
    fn get_permissions(&self) -> PermissionSet {
        PermissionSet::USER
    }
}

impl DatabaseOperation for Connection<AdminOps> {
    fn execute(&self, query: &str) -> Result<QueryResult> {
        AdminOps::execute_query(self, query)  // No restrictions
    }
    
    fn get_permissions(&self) -> PermissionSet {
        PermissionSet::ADMIN
    }
}

// Generic function works with any database connection
fn run_migration<T>(conn: &T, migrations: &[&str]) -> Result<()>
where T: DatabaseOperation
{
    for migration in migrations {
        if conn.get_permissions().allows_ddl() {
            conn.execute(migration)?;
        } else {
            return Err("Insufficient permissions for migration");
        }
    }
    Ok(())
}
```

### Runtime Interface Resolution

```cpp
// Dynamic interface dispatch with access rights
fn process_dynamic_connection(conn: &runtime Connection) -> Result<String> {
    // Try different interfaces based on runtime type
    if let Some(queryable) = conn.as_interface::<dyn Queryable>() {
        let result = queryable.execute_query("SELECT version()")?;
        Ok(format!("Database version: {}", result.to_string()))
    } else if let Some(displayable) = conn.as_interface::<dyn Display>() {
        Ok(displayable.fmt())
    } else {
        Err("Connection doesn't implement any known interfaces")
    }
}
```

## Pattern: Interface Adapters

Convert between different interface types:

```cpp
// Adapter pattern using interfaces
class FileToReader<T> {
    inner: T,
    constructed_by: AdapterOps,
}

functional class AdapterOps {
    fn construct<T>(file: T) -> FileToReader<T>
    where T: FileOps
    {
        FileToReader { inner: file }
    }
}

// Implement standard interface for file types
impl<T> Readable for FileToReader<T>
where T: FileOps
{
    fn read(&mut self, buf: &mut [u8]) -> Result<usize> {
        T::read_bytes(&mut self.inner, buf)
    }
}

// Now any FileOps can be used as Readable
fn process_readable_data<R: Readable>(reader: &mut R) -> Result<Vec<u8>> {
    let mut data = Vec::new();
    reader.read_to_string()?;  // Uses default implementation
    Ok(data)
}

let file = FileOps::open("data.txt")?;
let mut adapter = AdapterOps::construct(file);
let data = process_readable_data(&mut adapter)?;
```

## Best Practices

### Interface Design Guidelines

```cpp
// Good: Focused, cohesive interface
interface Writer {
    fn write(&mut self, data: &[u8]) -> Result<usize>;
    fn flush(&mut self) -> Result<()>;
}

// Bad: Too many unrelated methods
interface Kitchen {
    fn cook_pasta(&mut self) -> Pasta;
    fn wash_dishes(&mut self) -> Result<()>;
    fn pay_bills(&mut self) -> Result<()>;  // Unrelated!
    fn write_novel(&mut self) -> Novel;     // Also unrelated!
}

// Good: Small, composable interfaces
interface Read {
    fn read(&mut self, buf: &mut [u8]) -> Result<usize>;
}

interface Write {
    fn write(&mut self, data: &[u8]) -> Result<usize>;
}

interface Seek {
    fn seek(&mut self, pos: SeekFrom) -> Result<u64>;
}

// Compose as needed
interface RandomAccess: Read + Write + Seek {}
```

### When to Use Interfaces vs Access Rights

```cpp
// Use interfaces for:
// 1. Common operations across unrelated types
interface Serializable {
    fn serialize(&self) -> Vec<u8>;
    fn deserialize(data: &[u8]) -> Result<Self>;
}

// 2. Generic programming
fn save_to_file<T: Serializable>(item: &T, path: &str) -> Result<()> {
    let data = item.serialize();
    std::fs::write(path, data)
}

// Use access rights for:
// 1. Capability-based security
class SecureData {
    secret: Secret,
    exposes AdminOps { secret }  // Only admins can access
}

// 2. Inheritance-like relationships
class NetworkSocket {
    fd: FileDescriptor,
    exposes TcpOps { fd }     // TCP-specific operations
    exposes UdpOps { fd }     // UDP-specific operations
}
```

### Interface Object Performance

```cpp
// Static dispatch - zero overhead
fn process_items<T: Display>(items: &[T]) {
    for item in items {
        println!("{}", item.fmt());  // Inlined
    }
}

// Dynamic dispatch - vtable lookup
fn process_items_dynamic(items: &[&dyn Display]) {
    for item in items {
        println!("{}", item.fmt());  // Vtable call
    }
}

// Choose static when types are known, dynamic for flexibility
```

Interfaces provide a clean, type-safe way to define common operations across different types, complementing CPrime's access rights system by avoiding the need for frequent casting while maintaining compile-time safety and performance.