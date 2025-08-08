# CPrime Interfaces

## Overview

CPrime interfaces are **polymorphic constructs** (not classes) that act as glue between RAII state holders (data classes) and RAII state modifiers (functional classes). These constructs provide either memory contracts or accessor methods for abstractable work on different types using the same function calls.

**Important**: Interfaces are not objects or classes themselves - they are language constructs that enable polymorphism.

They offer two distinct variants:

1. **Compile-time Interfaces**: Memory contracts with zero overhead - direct memory access
2. **Runtime Interfaces**: Accessor-based with method call overhead - flexible layouts

While functional classes (access rights) provide inheritance-like polymorphism with explicit memory costs, interfaces serve as constructs in CPrime's polymorphism system: functional classes (inheritance-like), traditional interfaces (shared operations via constructs), interface memory contracts (N:M composition via constructs), and unions (pattern matching via constructs).

## Core Concepts

### Interface Constructs vs Functional Classes (Access Rights)

Interface constructs and functional classes (access rights) serve different purposes in CPrime's polymorphism system:

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

## Interface Memory Contracts: N:M Composition

### Overview of Memory Contracts

In addition to traditional functional contracts, CPrime interfaces can serve as **memory copy contracts** that enable N:M composition. This allows multiple data classes to work with multiple functional classes through shared interface bindings.

For comprehensive documentation on interface memory contracts, see [interface-memory-contracts.md](interface-memory-contracts.md).

### Compile-Time vs Runtime Memory Contracts

#### Compile-Time Interface Memory Contracts
Require exact memory layout compliance and provide zero-cost access:

```cpp
interface Cacheable {
    memory_contract {
        id: u64,           // Must be at offset 0
        timestamp: u64,    // Must be at offset 8
        void[32],          // Exactly 32 bytes at offset 16-47
        hash: u32,         // Must be at offset 48
    }
    
    fn cache_key(&self) -> CacheKey;
}

// Data class with exact layout compliance
class UserData {
    user_id: u64,        // Offset 0 - maps to id
    created: u64,        // Offset 8 - maps to timestamp
    profile: UserProfile, // Offset 16 - fits in void[32]
    status_hash: u32,    // Offset 48 - maps to hash
    permissions: Vec<Permission>, // After interface contract
    
    implements Cacheable {
        id <- user_id,
        timestamp <- created,
        hash <- status_hash,
        void[32] <- memory_region(16, 32),
    }
}

// Generic functional class with zero-cost access
functional class CacheOps<T: Cacheable> {
    fn store_in_cache(data: &T) -> Result<()> {
        // Direct memory access - zero overhead
        let key = unsafe { *(data as *const _ as *const u64) };
        let timestamp = unsafe { *(data as *const _ as *const u64).offset(1) };
        
        cache_system::store(key, timestamp, data)
    }
}
```

#### Runtime Interface Memory Contracts
Allow flexible memory layouts with accessor methods:

```cpp
runtime interface FlexibleCacheable {
    data_contract {
        id: u64,           // Must be accessible, any layout
        timestamp: u64,    // Must be accessible, any layout
        hash: u32,         // Must be accessible, any layout
    }
    
    fn cache_key(&self) -> CacheKey;
}

// Data class with any memory layout
class ProductData {
    category: String,       // Any offset
    product_id: u64,       // Anywhere in memory
    name: String,          // Any layout
    modified_at: u64,      // Anywhere in memory
    content_hash: u32,     // Anywhere in memory
    
    implements FlexibleCacheable {
        id <- product_id,        // Generates accessor method
        timestamp <- modified_at, // Generates accessor method
        hash <- content_hash,    // Generates accessor method
    }
    
    // Compiler generates:
    // fn _flexible_cacheable_id(&self) -> u64 { self.product_id }
    // fn _flexible_cacheable_timestamp(&self) -> u64 { self.modified_at }
    // fn _flexible_cacheable_hash(&self) -> u32 { self.content_hash }
}

// Generic functional class with accessor access
functional class FlexibleCacheOps<T: FlexibleCacheable> {
    fn store_in_cache(data: &T) -> Result<()> {
        // Accessor method calls - small performance cost
        let key = data._flexible_cacheable_id();
        let timestamp = data._flexible_cacheable_timestamp();
        
        cache_system::store(key, timestamp, data)
    }
}
```

### Single Declaration Principle for Interface Constructs

Each interface construct can be declared as **either** compile-time or runtime, but **never both**:

```cpp
// Option A: Compile-time interface (zero-cost, inflexible)
interface Serializable {
    memory_contract { ... }
    fn serialize(&self) -> Vec<u8>;
}

// Option B: Runtime interface (flexible, accessor cost)
runtime interface FlexibleSerializable {
    data_contract { ... }  // Same fields, different access method
    fn serialize(&self) -> Vec<u8>;
}

// FORBIDDEN: Cannot have both versions of same interface name
```

This prevents function explosion and maintains uniform access patterns within each interface type.

### Benefits of Memory Contracts

1. **N:M Composition**: Multiple data classes work with multiple functional classes
2. **Zero-Cost Abstractions**: Compile-time interfaces provide direct memory access
3. **Flexible Composition**: Runtime interfaces support dynamic layouts
4. **Type Safety**: Compiler verifies interface implementations
5. **Performance Control**: Explicit choice between speed and flexibility

### Interface Exposure with `semconst` Fields

CPrime's two-dimensional access control shines when combining `semconst` fields with interface exposure:

```cpp
class SecureConfig {
    semconst api_key: String,        // Class can only replace atomically
    semconst database_url: String,   // Class can only replace atomically
    mutable cache: HashMap,           // Class can modify freely
    
    // Expose semconst fields with different permissions
    exposes AdminInterface {
        api_key as mutable,      // Admin can modify semconst field!
        database_url as mutable, // Admin can modify semconst field!
        cache as mutable,        // Admin has full access
    }
    
    exposes UserInterface {
        api_key as const,        // User can only read
        database_url as const,   // User can only read
        // cache not exposed     // User cannot access cache
    }
}

// Interface implementations respect field exposure
interface Configurable {
    fn get_connection_string(&self) -> &String;
    fn update_connection(&mut self, url: String);
}

impl Configurable for SecureConfig<AdminInterface> {
    fn get_connection_string(&self) -> &String {
        &self.database_url  // Read access
    }
    
    fn update_connection(&mut self, url: String) {
        // Admin can modify semconst through interface!
        self.database_url = url;  // ✓ Allowed via AdminInterface
    }
}

impl Configurable for SecureConfig<UserInterface> {
    fn get_connection_string(&self) -> &String {
        &self.database_url  // Read access only
    }
    
    fn update_connection(&mut self, url: String) {
        // Compile error - UserInterface has const access only
        // self.database_url = url;  // ❌ COMPILE ERROR
        panic!("Users cannot update configuration");
    }
}
```

#### Memory Contracts with Field Modifiers

Interface memory contracts can specify field modifiers for precise control:

```cpp
interface Versioned {
    memory_contract {
        id: semconst u64,       // ID is semantically preserved
        version: mutable u32,    // Version can be incremented
        timestamp: u64,          // Default modifier
    }
    
    fn increment_version(&mut self);
}

class Document {
    semconst document_id: u64,    // Maps to semconst id
    mutable revision: u32,         // Maps to mutable version
    last_modified: u64,            // Maps to timestamp
    content: String,               // Not in interface
    
    implements Versioned {
        id <- document_id,
        version <- revision,
        timestamp <- last_modified,
    }
}

// Generic operations respect field modifiers
functional class VersionOps<T: Versioned> {
    fn increment_version(item: &mut T) {
        item.version += 1;  // ✓ Can modify mutable field
        // item.id = 0;     // ❌ Cannot modify semconst field
    }
    
    fn replace_id(item: &mut T, new_id: u64) {
        // Must use 1:1 move pattern for semconst
        let old_id = move(item.id);
        item.id = move(new_id);
    }
}
```

### Integration with Traditional Interfaces

Memory contracts and traditional interfaces can coexist:

```cpp
// Both memory contract AND traditional interface
interface Cacheable {
    memory_contract {
        id: u64,
        timestamp: u64,
        hash: u32,
    }
    
    // Traditional interface methods
    fn cache_key(&self) -> CacheKey;
    fn is_expired(&self, max_age: Duration) -> bool;
}

// Implementation provides both
class UserData implements Cacheable {
    // Memory layout linking
    id <- user_id,
    timestamp <- last_login,
    hash <- profile_hash,
    
    // Traditional interface implementation
    fn cache_key(&self) -> CacheKey {
        CacheKey::from_user(self.user_id)
    }
    
    fn is_expired(&self, max_age: Duration) -> bool {
        let age = SystemTime::now().duration_since(
            UNIX_EPOCH + Duration::from_secs(self.last_login)
        ).unwrap_or_default();
        age > max_age
    }
}
```

This dual capability makes CPrime interfaces uniquely powerful, supporting both traditional polymorphism and revolutionary N:M composition patterns.

### Library Interface Extensibility

Interfaces can be marked with extension modes to control how they can be extended across library boundaries:

#### Extension Control Matrix for Interfaces

```cpp
// Fully sealed interface - no user extensions
#[extension(data_closed, access_closed)]
interface SecureProtocol {
    fn authenticate(&self, credentials: &Credentials) -> AuthResult;
    fn execute_secure_operation(&self, op: &Operation) -> Result<Response>;
    
    // Users can implement this interface but cannot:
    // - Add new methods
    // - Implement on new data types (library controls which types)
}

// Classic extensible interface - users implement on their types  
#[extension(data_open, access_closed)]
interface Serializable {
    fn serialize(&self) -> Vec<u8>;
    fn deserialize(data: &[u8]) -> Result<Self>;
    
    // Users can: implement on any data type
    // Users cannot: add new methods to the interface
}

// Operations extensible - fixed data, new operations
#[extension(data_closed, access_open)]
interface DatabaseConnection {
    memory_contract {
        handle: semconst Handle,
        status: mutable Status,
    }
    
    fn execute_query(&self, sql: &str) -> Result<QueryResult>;
    // Users can add methods but must work with fixed memory contract
}

// Fully extensible protocol
#[extension(data_open, access_open)]
interface PluginProtocol {
    type Config;
    type State;
    
    fn initialize(&mut self, config: Self::Config) -> Result<()>;
    fn process(&mut self, input: &[u8]) -> Result<Vec<u8>>;
    
    // Users can: implement on any type AND add new methods
}
```

#### Library Distribution Strategies

| Extension Mode | Library Provides | Users Can |
|----------------|------------------|-----------|
| `[closed, closed]` | Pre-compiled implementations only | Use existing implementations |
| `[data_open, closed]` | Interface definition + headers | Implement on custom types |
| `[closed, access_open]` | Data types + interface headers | Add operations to library types |
| `[open, open]` | Interface protocol + full headers | Complete custom implementations |

#### Example: Database Library Evolution

```cpp
// v1.0 - Conservative start
#[extension(data_closed, access_closed)]
interface DatabaseOps {
    fn connect(&self, config: &Config) -> Result<Connection>;
    fn execute(&self, conn: &Connection, sql: &str) -> Result<QueryResult>;
}

module Database {
    // Only these implementations exist
    extern template Connection<MySQLOps>;
    extern template Connection<PostgresOps>;
    extern template Connection<SQLiteOps>;
}

// v2.0 - Open operations based on user feedback
#[extension(data_closed, access_open)]  
interface DatabaseOps {
    // Same methods, but now users can add operations
    fn connect(&self, config: &Config) -> Result<Connection>;
    fn execute(&self, conn: &Connection, sql: &str) -> Result<QueryResult>;
}

// Users can now create:
functional class CustomAnalyticsOps implements DatabaseOps {
    fn connect(&self, config: &Config) -> Result<Connection> {
        // Delegate to library implementation
        StandardOps::connect(config)
    }
    
    fn execute(&self, conn: &Connection, sql: &str) -> Result<QueryResult> {
        // Add analytics tracking
        let start_time = SystemTime::now();
        let result = StandardOps::execute(conn, sql);
        self.record_query_metrics(sql, start_time.elapsed());
        result
    }
    
    // New operations users can add
    fn generate_query_report(&self, timeframe: Duration) -> AnalyticsReport {
        // Custom functionality
    }
}

// v3.0 - Full extensibility (if needed)
#[extension(data_open, access_open)]
interface DatabaseOps {
    // Now users can implement complete custom database backends
}
```

#### Interface Memory Contracts with Extensions

Extension modes apply to memory contracts as well:

```cpp
#[extension(data_closed, access_open)]
interface Cacheable {
    memory_contract {
        cache_key: semconst u64,     // Library controls structure
        timestamp: mutable u64,      // Users can modify
        metadata: [u8; 32],         // Fixed layout
    }
    
    fn get_cache_key(&self) -> u64;
}

// Library provides the data structure
class LibraryData {
    id: u64,                        // Maps to cache_key
    last_accessed: u64,             // Maps to timestamp  
    extra_data: [u8; 32],          // Maps to metadata
    private_fields: InternalState,  // Not exposed
    
    implements Cacheable {
        cache_key <- id,
        timestamp <- last_accessed,
        metadata <- memory_region(&extra_data, 32),
    }
}

// Users can add operations but not change memory layout
functional class UserCacheOps<T: Cacheable> {
    fn cache_with_ttl(item: &mut T, ttl: Duration) {
        // Can update timestamp (mutable in contract)
        item.timestamp = SystemTime::now().as_secs() + ttl.as_secs();
        
        // Cannot modify cache_key (semconst in contract)
        // let new_key = generate_key();  
        // item.cache_key = new_key;  // ❌ COMPILE ERROR
    }
    
    fn custom_cache_operation(item: &T) -> CustomResult {
        // New operations users can add
        let key = item.get_cache_key();
        // Custom logic
    }
}
```

#### Compilation Implications

Extension modes affect how interfaces are compiled and distributed:

```cpp
// [closed, closed] - Binary only
interface BinaryOnlyOps {
    #[extension(data_closed, access_closed)]
    fn process(&self, data: &[u8]) -> Vec<u8>;
}
// Distributed as: libfoo.so (no headers needed)

// [data_open, access_closed] - Headers for implementation  
interface ImplementableOps {
    #[extension(data_open, access_closed)]
    fn serialize(&self) -> Vec<u8>;
}
// Distributed as: libfoo.so + interface_headers.h

// [closed, access_open] - Headers for extension
interface ExtensibleOps {
    #[extension(data_closed, access_open)]
    memory_contract {
        id: u64,
        data: Vec<u8>,
    }
    fn process(&self, input: &str) -> String;
}
// Distributed as: libfoo.so + data_headers.h + interface_headers.h

// [open, open] - Full headers
interface FullProtocol {
    #[extension(data_open, access_open)]
    type Config;
    fn configure(&mut self, config: Self::Config) -> Result<()>;
}
// Distributed as: libfoo.so + full_source_headers.h
```

For comprehensive documentation on library linking and extension strategies, see [library-linking.md](library-linking.md).

## Interface Design Evolution

CPrime's interface system has evolved to support increasingly sophisticated composition patterns:

1. **Traditional Interfaces**: Shared vtable contracts (like other languages)
2. **Memory Contracts**: Direct memory access patterns for performance
3. **N:M Composition**: Multiple data classes with multiple functional classes
4. **Hybrid Interfaces**: Both traditional methods and memory contracts

This evolution maintains backward compatibility while enabling new architectural patterns that were previously impossible or costly.

Interfaces provide a clean, type-safe way to define both common operations and memory access patterns across different types, complementing CPrime's access rights system by avoiding the need for frequent casting while enabling powerful N:M composition patterns and maintaining compile-time safety and performance.