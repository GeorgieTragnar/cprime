# CPrime Three-Tier Polymorphism System

## Overview

CPrime provides three distinct polymorphism mechanisms that work together to offer complete functional equivalence with C++ inheritance while providing superior architectural constraints. These three tiers offer different trade-offs between performance, flexibility, and design patterns:

1. **Access Rights**: Inheritance-like polymorphism with vtable extensions
2. **Interfaces**: Common vtable contracts without casting overhead  
3. **Unions**: Pattern matching that bypasses vtables entirely

This document demonstrates how these three systems complement each other and achieve full functional equivalence with traditional OOP patterns.

## The Three-Tier Architecture

### Tier 1: Access Rights (Inheritance-Like)

Access rights provide inheritance-like polymorphism by extending data classes with vtable pointers:

```cpp
class Connection {
    handle: DbHandle,
    buffer: [u8; 4096],
    
    // Each access right adds vtable pointer + memory
    exposes UserOps { handle, buffer }      // +8 bytes vtable
    exposes AdminOps { handle, buffer }     // +8 bytes vtable  
    exposes runtime QueryOps { handle }     // +8 bytes vtable + RTTI
}

// Requires casting like C++ inheritance
let conn: Connection = create_connection();
if let Some(admin) = conn.cast::<AdminOps>() {
    AdminOps::delete_table(&admin);
}
```

**Characteristics:**
- Memory overhead per access right (like C++ inheritance)
- Requires casting to access derived operations
- Compile-time (no downcasting) or runtime (full polymorphism) modes
- Vtable dispatch for method resolution

### Tier 2: Interfaces (Common Contracts)

Interfaces provide shared contracts across different types without requiring casting:

```cpp
interface Queryable {
    fn execute_query(&self, sql: &str) -> Result<QueryResult>;
}

// Different access rights implement same interface
impl Queryable for Connection<UserOps> {
    fn execute_query(&self, sql: &str) -> Result<QueryResult> {
        UserOps::query(self, sql)
    }
}

impl Queryable for Connection<AdminOps> {
    fn execute_query(&self, sql: &str) -> Result<QueryResult> {
        AdminOps::query(self, sql)  // More privileges
    }
}

// No casting needed - work through interface
fn run_query(conn: &impl Queryable, sql: &str) {
    conn.execute_query(sql)?;  // Dispatch based on implementation
}
```

**Characteristics:**
- No casting required for common operations
- Multiple types can implement same interface
- Static dispatch (monomorphization) or dynamic dispatch (trait objects)
- Supplements access rights by avoiding frequent casting

### Tier 3: Unions (Memory Contracts)

Unions act as memory reservation systems that integrate with the existing vtable infrastructure:

```cpp
union runtime ConnectionSpace {
    UserConn(runtime Connection<UserOps>),
    AdminConn(runtime Connection<AdminOps>),
    PowerConn(runtime Connection<UserOps, QueryOps>),
}

// Work with actual objects in union space, not "the union"
fn handle_connection(space: &ConnectionSpace) {
    // Same API as access rights - unified type system
    if let Some(user) = space.try_as::<Connection<UserOps>>() {
        UserOps::query(&user, "SELECT ...");  // Direct object method via vtable
    } else if let Some(admin) = space.try_as::<Connection<AdminOps>>() {
        AdminOps::query(&admin, "SELECT ...");  // Direct object method via vtable
    } else if let Some(power) = space.try_as::<Connection<UserOps, QueryOps>>() {
        QueryOps::query(&power, "SELECT ...");  // Direct object method via vtable
    }
}
```

**Characteristics:**
- Memory contracts - reserve space for largest variant
- Unified type system - reuses existing vtable infrastructure
- No duplicate type tracking - objects carry their own type info
- Template integration - enables self-expanding heterogeneous containers

## Choosing the Right Tier

### When to Use Access Rights

**Good for:**
- Capability-based security
- Inheritance-like relationships
- When you need casting and type hierarchies

```cpp
class SecureResource {
    data: SensitiveData,
    
    exposes UserOps { data }      // Limited access
    exposes AdminOps { data }     // Full access
}

// Security through access rights
fn secure_operation(resource: &SecureResource<AdminOps>) {
    AdminOps::dangerous_operation(resource);  // Only admins can call
}
```

### When to Use Interfaces

**Good for:**
- Common operations across unrelated types
- Avoiding frequent casting
- Generic programming

```cpp
interface Serializable {
    fn serialize(&self) -> Vec<u8>;
}

// Generic function works with any Serializable
fn save_to_file<T: Serializable>(item: &T, path: &str) -> Result<()> {
    let data = item.serialize();
    std::fs::write(path, data)
}
```

### When to Use Unions

**Good for:**
- Heterogeneous collections with self-expansion
- Template type explosion solutions
- Memory-efficient polymorphic storage
- Integration with existing vtable systems

```cpp
// Self-expanding heterogeneous container
let mut items: Vec<runtime Any> = Vec::new();

items.push(SmallObject{data: 42});     // Internal union: {SmallObject}
items.push(LargeObject{data: [0; 1024]}); // Union expands -> reallocation!

// Access objects naturally
for item in &items {
    if let Some(processable) = item.try_as::<dyn Processable>() {
        processable.process();  // Interface call via vtable
    }
}
```

## Solving Template Type Explosion

One of the most powerful combinations is using unions to solve template/access-rights type explosion:

```cpp
// Problem: Cannot mix different access rights in same container
Container<UserOps> user_connections;        // Only user connections
Container<AdminOps> admin_connections;      // Only admin connections
Container<UserOps, QueryOps> power_connections;  // Only power connections

// Solution: Self-expanding union container
let mut connection_pool: Vec<runtime ConnectionSpace> = Vec::new();

// Add different connection types - union auto-expands as needed
connection_pool.push(ConnectionSpace::from(create_user_connection()));
connection_pool.push(ConnectionSpace::from(create_admin_connection()));
connection_pool.push(ConnectionSpace::from(create_power_connection()));

// Process all connections with unified API
for conn_space in &connection_pool {
    // Same dynamic casting API as access rights
    if let Some(user) = conn_space.try_as::<Connection<UserOps>>() {
        UserOps::execute(&user, query);
    } else if let Some(admin) = conn_space.try_as::<Connection<AdminOps>>() {
        AdminOps::execute(&admin, query);
    } else if let Some(power) = conn_space.try_as::<Connection<UserOps, QueryOps>>() {
        QueryOps::execute(&power, query);
    }
    
    // Or use interface if available
    if let Some(queryable) = conn_space.try_as::<dyn Queryable>() {
        queryable.execute_query(query);  // Interface method
    }
}
```

## Performance Comparison with C++ Inheritance

### C++ Virtual Inheritance Costs

```cpp
// C++ virtual dispatch
class Base {
    virtual void method() = 0;  // Vtable lookup at runtime
};

class Derived : public Base {
    void method() override;     // Vtable entry
};

// Usage
std::vector<std::unique_ptr<Base>> objects;
for (auto& obj : objects) {
    obj->method();  // Runtime vtable lookup
}
```

### CPrime Three-Tier Performance

**Tier 1 (Access Rights):**
```cpp
// Compile-time access rights - zero overhead when type known
let conn: Connection<AdminOps> = create_admin();
AdminOps::method(&conn);  // Direct call, no vtable

// Runtime access rights - equivalent to C++ vtables
let conn: runtime Connection = create_dynamic();
if let Some(admin) = conn.cast::<AdminOps>() {
    AdminOps::method(&admin);  // Vtable lookup
}
```

**Tier 2 (Interfaces):**
```cpp
// Static dispatch - monomorphized at compile time
fn process<T: Processable>(item: &T) {
    item.process();  // Inlined, no overhead
}

// Dynamic dispatch - equivalent to C++ vtables
fn process_dynamic(item: &dyn Processable) {
    item.process();  // Vtable lookup
}
```

**Tier 3 (Unions):**
```cpp
// Runtime unions - same performance as vtables but unified API
if let Some(obj_a) = union_space.try_as::<ObjectA>() {
    obj_a.process();  // Vtable dispatch, same as access rights
} else if let Some(obj_b) = union_space.try_as::<ObjectB>() {
    obj_b.process();  // Vtable dispatch, unified with type system
}

// Compile-time unions - zero overhead when variant known
fn process_known_result() -> CompileTimeResult<i32, String> {
    CompileTimeResult::Ok(42)  // Compiler knows it's Ok
}
// Pattern matching optimized away when variant is statically known
```

## Complex Pattern Equivalence

### Multiple Inheritance Translation

**C++ Multiple Inheritance:**
```cpp
class Drawable {
    virtual void draw() = 0;
};

class Serializable {
    virtual std::string serialize() = 0;
};

class Shape : public Drawable, public Serializable {
    void draw() override;
    std::string serialize() override;
};
```

**CPrime Three-Tier Approach:**

**Option 1: Access Rights (inheritance-like)**
```cpp
class ShapeData {
    geometry: GeometryData,
    
    exposes DrawOps { geometry }
    exposes SerializationOps { geometry }
    exposes CombinedOps { geometry }  // Both capabilities
}
```

**Option 2: Interfaces (contract-based)**
```cpp
interface Drawable {
    fn draw(&self);
}

interface Serializable {
    fn serialize(&self) -> String;
}

// Implement both interfaces
impl Drawable for ShapeData { ... }
impl Serializable for ShapeData { ... }

// Generic function works with both
fn process<T>(shape: &T) 
where T: Drawable + Serializable 
{
    shape.draw();
    let data = shape.serialize();
}
```

**Option 3: Unions (memory contract based)**
```cpp
union runtime ShapeSpace {
    DrawableShape(runtime ShapeData<DrawOps>),
    SerializableShape(runtime ShapeData<SerializationOps>),
    FullShape(runtime ShapeData<CombinedOps>),
}

fn process_shape(space: &ShapeSpace) {
    // Work with actual objects in union space
    if let Some(drawable) = space.try_as::<ShapeData<DrawOps>>() {
        DrawOps::draw(&drawable);
    } else if let Some(serializable) = space.try_as::<ShapeData<SerializationOps>>() {
        let data = SerializationOps::serialize(&serializable);
    } else if let Some(full) = space.try_as::<ShapeData<CombinedOps>>() {
        CombinedOps::draw(&full);
        let data = CombinedOps::serialize(&full);
    }
    
    // Or use interfaces if available
    if let Some(drawable) = space.try_as::<dyn Drawable>() {
        drawable.draw();  // Interface method
    }
}
```

### Polymorphic Collections

**C++ Virtual Collections:**
```cpp
std::vector<std::unique_ptr<Shape>> shapes;
for (auto& shape : shapes) {
    shape->draw();      // Runtime dispatch
    shape->area();      // Runtime dispatch
}
```

**CPrime Multi-Tier Solutions:**

**Tier 1: Runtime Access Rights**
```cpp
let shapes: Vec<runtime ShapeData> = create_shapes();
for shape in &shapes {
    if let Some(drawable) = shape.cast::<DrawOps>() {
        DrawOps::draw(&drawable);
    }
}
```

**Tier 2: Interface Objects**
```cpp
let shapes: Vec<Box<dyn Drawable>> = create_drawable_shapes();
for shape in &shapes {
    shape.draw();  // Dynamic dispatch through interface
}
```

**Tier 3: Union Collections (Preferred)**
```cpp
union ShapeVariant {
    Circle(CircleData<DrawOps>),
    Rectangle(RectangleData<DrawOps>),
    Triangle(TriangleData<DrawOps>),
}

let shapes: Vec<ShapeVariant> = create_shape_variants(); 
for shape in &shapes {
    match shape {
        ShapeVariant::Circle(c) => CircleDrawOps::draw(c),
        ShapeVariant::Rectangle(r) => RectangleDrawOps::draw(r),
        ShapeVariant::Triangle(t) => TriangleDrawOps::draw(t),
    }
}
```

## Architectural Benefits

### Compared to C++ Inheritance

| Feature | C++ Inheritance | CPrime Three-Tier |
|---------|----------------|-------------------|
| **Performance** | Vtable overhead | Choose: zero-cost, vtable, or pattern matching |
| **Type Safety** | Runtime cast failures | Compile-time verification + exhaustive matching |
| **Memory Overhead** | Hidden vtable costs | Explicit memory costs per tier |
| **Flexibility** | Deep hierarchies | Flat composition with multiple dispatch options |
| **Debugging** | Complex vtable chains | Clear dispatch mechanisms |
| **Template Compatibility** | Inheritance constraints | Trait constraints + pattern matching |

### Design Pattern Support

**Strategy Pattern:**
- **Tier 1**: Different access rights as different strategies
- **Tier 2**: Interface implementations as strategies  
- **Tier 3**: Union variants as strategy choices

**Visitor Pattern:**
- **Tier 1**: Access rights for different visitor capabilities
- **Tier 2**: Visitor interface with multiple implementations
- **Tier 3**: Pattern matching replaces visitor entirely

**State Machine:**
- **Tier 1**: Access rights for state-specific operations
- **Tier 2**: State interface with transitions
- **Tier 3**: Union variants as states (preferred)

## Migration Strategy

### From C++ to CPrime

1. **Analyze inheritance hierarchy depth**
   - Shallow (1-2 levels): Use access rights
   - Medium (3-4 levels): Consider interfaces
   - Deep (5+ levels): Refactor to unions

2. **Identify polymorphism usage**
   - Dynamic casting chains → unions with pattern matching
   - Interface-based code → CPrime interfaces  
   - Capability-based access → access rights

3. **Performance requirements**
   - Critical paths → compile-time variants (unions/access rights)
   - Flexible code → runtime variants or interfaces
   - Mixed requirements → combine tiers

### Example Migration

**C++ Code:**
```cpp
class Vehicle {
    virtual void start() = 0;
    virtual void stop() = 0;
};

class Car : public Vehicle {
    void start() override;
    void stop() override;
    virtual void openTrunk();
};

class Motorcycle : public Vehicle {
    void start() override; 
    void stop() override;
    virtual void popWheelie();
};
```

**CPrime Migration:**
```cpp
// Option 1: Union-based (recommended for closed set)
union Vehicle {
    Car { engine: Engine, trunk: TrunkData },
    Motorcycle { engine: Engine, wheelie_capable: bool },
}

functional class VehicleOps {
    fn start(vehicle: &mut Vehicle) {
        match vehicle {
            Vehicle::Car { engine, .. } => CarEngine::start(engine),
            Vehicle::Motorcycle { engine, .. } => MotorcycleEngine::start(engine),
        }
    }
    
    fn stop(vehicle: &mut Vehicle) {
        match vehicle {
            Vehicle::Car { engine, .. } => CarEngine::stop(engine),
            Vehicle::Motorcycle { engine, .. } => MotorcycleEngine::stop(engine),
        }
    }
}

// Option 2: Interface-based (for open set)
interface Drivable {
    fn start(&mut self);
    fn stop(&mut self);
}

class CarData { engine: Engine, trunk: TrunkData }
class MotorcycleData { engine: Engine, wheelie_capable: bool }

impl Drivable for CarData { ... }
impl Drivable for MotorcycleData { ... }
```

## Coroutine Polymorphism Integration

CPrime's revolutionary coroutine system demonstrates the power of the three-tier polymorphism approach, using all three tiers in a unified architecture:

### Tier 1: Access Rights for Coroutine Capabilities

Coroutines use access rights for capability-based security and functionality:

```cpp
class DatabaseCoro {
    stack_memory: *mut u8,
    connection: DbConnection,
    query_cache: QueryCache,
    
    // Mixin-style access rights for different database capabilities
    exposes ReadOps { connection, query_cache }   // Read-only database operations
    exposes WriteOps { connection }               // Write operations  
    exposes AdminOps { connection, query_cache }  // Full administrative access
    
    constructed_by: DbCoroManager,
}

// Coroutine maintains capabilities across all suspension points
async fn handle_read_request(request: ReadRequest) -> ReadResponse {
    // This coroutine has ReadOps capabilities throughout execution
    let validation = co_await validate_request(&request);
    
    if validation.is_valid() {
        // Still has ReadOps after suspension
        let query_result = co_await ReadOps::execute_select_query(&request.sql);
        ReadResponse::success(query_result)
    } else {
        ReadResponse::unauthorized()
    }
}

// Different coroutine types with different access rights
let read_coro: DatabaseCoro<ReadOps> = DbCoroManager::construct_read_only(connection);
let admin_coro: DatabaseCoro<AdminOps> = DbCoroManager::construct_admin(admin_connection);

// Casting works for coroutines too
if let Some(admin_ops) = admin_coro.cast::<AdminOps>() {
    AdminOps::execute_management_query(&admin_ops, "OPTIMIZE TABLE users");
}
```

### Tier 2: Interfaces for Common Coroutine Operations

Interfaces provide common contracts across different coroutine types:

```cpp
interface CoroLifecycle {
    fn resume(&mut self) -> CoroResult;
    fn suspend(&mut self);
    fn is_complete(&self) -> bool;
    fn get_stats(&self) -> CoroStats;
}

interface Schedulable {
    fn priority(&self) -> Priority;
    fn estimated_runtime(&self) -> Duration;
    fn can_yield(&self) -> bool;
}

// All coroutine size classes implement common interfaces
impl CoroLifecycle for MicroCoro<256> {
    fn resume(&mut self) -> CoroResult {
        MicroCoroManager::resume(self)  // Optimized micro resume
    }
    
    fn suspend(&mut self) {
        MicroCoroManager::suspend(self)
    }
    
    fn is_complete(&self) -> bool {
        self.state == CoroState::Complete
    }
    
    fn get_stats(&self) -> CoroStats {
        CoroStats {
            memory_usage: 256,
            context_switches: self.context_switch_count,
            allocation_source: AllocationSource::StackPool,
        }
    }
}

impl CoroLifecycle for LargeCoro {
    fn resume(&mut self) -> CoroResult {
        LargeCoroManager::resume(self)  // Individual coroutine resume
    }
    
    fn suspend(&mut self) {
        LargeCoroManager::suspend(self)
    }
    
    fn is_complete(&self) -> bool {
        self.state == CoroState::Complete
    }
    
    fn get_stats(&self) -> CoroStats {
        CoroStats {
            memory_usage: self.allocated_size,
            context_switches: self.context_switch_count,
            allocation_source: AllocationSource::Individual,
        }
    }
}

// Generic coroutine processing using interfaces
fn process_coroutines(coros: &mut [&mut dyn CoroLifecycle]) {
    for coro in coros {
        if !coro.is_complete() {
            match coro.resume() {
                CoroResult::Yield => {
                    // Coroutine yielded, continue later
                },
                CoroResult::Complete(_) => {
                    // Coroutine finished
                },
                CoroResult::Error(e) => {
                    log::error!("Coroutine error: {}", e);
                },
            }
        }
    }
}

// Scheduler using interface-based priority scheduling
fn schedule_by_priority(coros: &mut [&mut dyn Schedulable]) {
    // Sort by priority without knowing specific coroutine types
    coros.sort_by_key(|coro| coro.priority());
    
    for coro in coros {
        if coro.can_yield() {
            // Process high-priority coroutines first
            schedule_for_execution(coro);
        }
    }
}
```

### Tier 3: Unions for Coroutine Size Classes

Unions solve the template type explosion problem for heterogeneous coroutine containers:

```cpp
// Union-based storage for different coroutine sizes
union runtime CoroStorage {
    Micro(runtime MicroCoro<256>),      // Stack-allocated pool
    Small(runtime SmallCoro<2048>),     // Heap pool
    Medium(runtime MediumCoro<16384>),  // Heap pool  
    Large(runtime LargeCoro),           // Individual allocation
}

// Self-expanding heterogeneous coroutine scheduler
class CoroScheduler {
    active_coroutines: Vec<runtime CoroStorage>,
    completed_coroutines: Vec<CoroId>,
    constructed_by: SchedulerOps,
}

functional class SchedulerOps {
    fn construct() -> CoroScheduler {
        CoroScheduler {
            // Initially optimized for micro coroutines
            active_coroutines: Vec::with_union_hint::<CoroStorage::Micro>(),
            completed_coroutines: Vec::new(),
        }
    }
    
    fn schedule_task<T>(scheduler: &mut CoroScheduler, task: T) 
    where T: CoroTask
    {
        // Compiler analysis determines size class
        let size_hint = compiler_analyze_task::<T>();
        let coro_storage = CoroStorageManager::allocate_for_size(size_hint);
        
        // Union auto-expands if new size class is added
        scheduler.active_coroutines.push(coro_storage);
    }
    
    fn run_scheduler_loop(scheduler: &mut CoroScheduler) {
        for coro_space in &mut scheduler.active_coroutines {
            // Unified API across all coroutine sizes
            let result = if let Some(micro) = coro_space.try_as::<MicroCoro<256>>() {
                MicroCoroManager::resume(micro)  // 10 cycle resume
            } else if let Some(small) = coro_space.try_as::<SmallCoro<2048>>() {
                SmallCoroManager::resume(small)  // 50 cycle resume
            } else if let Some(medium) = coro_space.try_as::<MediumCoro<16384>>() {
                MediumCoroManager::resume(medium)  // 50 cycle resume
            } else if let Some(large) = coro_space.try_as::<LargeCoro>() {
                LargeCoroManager::resume(large)  // 100 cycle resume
            } else {
                unreachable!("Unknown coroutine size class")
            };
            
            match result {
                CoroResult::Complete(coro_id) => {
                    scheduler.completed_coroutines.push(coro_id);
                },
                CoroResult::Suspended => {
                    // Continue in next iteration
                },
                CoroResult::Error(e) => {
                    log::error!("Coroutine {} failed: {}", coro_space.id(), e);
                },
            }
        }
        
        // Remove completed coroutines
        Self::cleanup_completed(scheduler);
    }
}

// Migration between size classes when needed
fn migrate_coroutine_size(storage: &mut CoroStorage) -> Result<()> {
    let new_storage = match storage {
        CoroStorage::Micro(micro) if micro.needs_more_stack() => {
            // Migrate from stack pool to heap pool
            let migrated = migrate_micro_to_small(micro)?;
            CoroStorage::Small(migrated)
        },
        CoroStorage::Small(small) if small.needs_more_stack() => {
            let migrated = migrate_small_to_medium(small)?;
            CoroStorage::Medium(migrated)
        },
        CoroStorage::Medium(medium) if medium.needs_more_stack() => {
            let migrated = migrate_medium_to_large(medium)?;
            CoroStorage::Large(migrated)
        },
        _ => return Ok(()), // No migration needed
    };
    
    *storage = new_storage;
    Ok(())
}
```

### All Three Tiers Working Together

The most powerful aspect is how all three tiers work together seamlessly:

```cpp
// Complex web server coroutine using all three tiers
class WebServerCoro {
    request: HttpRequest,
    response: HttpResponse,
    connection: NetworkConnection,
    
    // Tier 1: Access rights for different server capabilities (mixin model)
    exposes HttpOps { request, response }          // HTTP protocol handling
    exposes TlsOps { connection }                  // TLS encryption
    exposes CompressionOps { response }            // Response compression
    exposes CacheOps { request, response }         // Caching capabilities
    exposes LoggingOps { request, response }       // Request logging
    exposes MetricsOps { request, response }       // Performance metrics
    
    constructed_by: WebServerManager,
}

// Tier 2: Interface for common web server operations
interface WebRequestHandler {
    fn handle_request(&mut self, request: HttpRequest) -> impl Future<Output = HttpResponse>;
    fn supports_method(&self, method: HttpMethod) -> bool;
    fn max_request_size(&self) -> usize;
}

impl WebRequestHandler for WebServerCoro<HttpOps, TlsOps> {
    async fn handle_request(&mut self, request: HttpRequest) -> HttpResponse {
        // Use access rights for actual implementation
        let parsed = HttpOps::parse_request(&request);
        let secure_response = TlsOps::encrypt_response(&parsed);
        secure_response
    }
    
    fn supports_method(&self, method: HttpMethod) -> bool {
        HttpOps::is_method_supported(method)
    }
    
    fn max_request_size(&self) -> usize {
        HttpOps::max_request_size()
    }
}

// Tier 3: Union storage for different web server coroutine configurations
union runtime WebServerCoroStorage {
    Basic(runtime WebServerCoro<HttpOps>),                           // Basic HTTP
    Secure(runtime WebServerCoro<HttpOps, TlsOps>),                 // HTTPS
    FullFeatured(runtime WebServerCoro<HttpOps, TlsOps, CompressionOps, CacheOps>), // All features
}

// Scheduler handles all variants with unified API
class WebServerScheduler {
    server_coroutines: Vec<runtime WebServerCoroStorage>,
    constructed_by: WebSchedulerOps,
}

functional class WebSchedulerOps {
    fn handle_incoming_request(
        scheduler: &mut WebServerScheduler,
        request: HttpRequest
    ) -> impl Future<Output = HttpResponse> {
        // Select appropriate coroutine based on request requirements
        let coro_storage = Self::select_best_handler(scheduler, &request);
        
        // All three tiers work together:
        match coro_storage {
            WebServerCoroStorage::Basic(basic) => {
                // Tier 1: Access HttpOps capability
                // Tier 2: Use WebRequestHandler interface
                // Tier 3: Union storage provides the coroutine
                basic.handle_request(request).await
            },
            WebServerCoroStorage::Secure(secure) => {
                // Multiple access rights: HttpOps + TlsOps
                secure.handle_request(request).await
            },
            WebServerCoroStorage::FullFeatured(full) => {
                // All capabilities: HTTP + TLS + Compression + Cache
                full.handle_request(request).await
            },
        }
    }
    
    fn select_best_handler(
        scheduler: &WebServerScheduler,
        request: &HttpRequest
    ) -> &mut WebServerCoroStorage {
        // Choose based on request requirements
        if request.requires_tls() && request.accepts_compression() {
            // Need full-featured handler
            scheduler.server_coroutines.iter_mut()
                .find(|coro| matches!(coro, WebServerCoroStorage::FullFeatured(_)))
                .unwrap_or_else(|| create_full_featured_handler())
        } else if request.requires_tls() {
            // Need secure handler
            scheduler.server_coroutines.iter_mut()
                .find(|coro| matches!(coro, WebServerCoroStorage::Secure(_)))
                .unwrap_or_else(|| create_secure_handler())
        } else {
            // Basic handler is sufficient
            scheduler.server_coroutines.iter_mut()
                .find(|coro| matches!(coro, WebServerCoroStorage::Basic(_)))
                .unwrap_or_else(|| create_basic_handler())
        }
    }
}
```

### Performance Benefits of Three-Tier Coroutine Architecture

| Aspect | Traditional Approach | CPrime Three-Tier |
|--------|---------------------|-------------------|
| **Memory Usage** | 64KB+ per coroutine | 256B-16KB per coroutine |
| **Context Switch** | 300-500 cycles | 50-100 cycles |
| **Allocation Cost** | malloc/free overhead | Pool allocation (0-100 cycles) |
| **Polymorphism** | Virtual function overhead | Choose: zero-cost, interface, or union |
| **Type Safety** | Runtime errors | Compile-time verification |
| **Memory Density** | Low (16 coroutines per MB) | High (1000+ micro coroutines per MB) |

The three-tier approach enables:
- **Access rights** for secure capability management
- **Interfaces** for common operations without casting
- **Unions** for efficient heterogeneous storage and migration
- **Unified API** across all coroutine sizes and capabilities
- **Revolutionary performance** through compiler-guided optimization

## Conclusion

CPrime's three-tier polymorphism system provides complete functional equivalence with C++ inheritance while offering superior architectural properties:

### Complete Coverage
- **Access Rights**: Handle inheritance-like relationships and security
- **Interfaces**: Provide common contracts without casting overhead
- **Unions**: Enable pattern matching and solve type explosion

### Performance Spectrum  
- **Zero-cost**: Compile-time access rights, static interfaces, compile-time unions
- **Low-cost**: Pattern matching with branch prediction
- **Standard-cost**: Runtime access rights, dynamic interfaces equivalent to C++ vtables

### Architectural Advantages
- **Explicit costs**: Each tier makes performance implications clear
- **No deep hierarchies**: Flat composition prevents inheritance anti-patterns
- **Flexible dispatch**: Choose the right mechanism for each use case
- **Type safety**: Exhaustive pattern matching and compile-time verification
- **Clear separation**: Each tier serves a distinct purpose

**Result:** CPrime provides all the expressiveness of C++ inheritance with better performance characteristics, clearer architecture, and prevention of common OOP pitfalls through its carefully designed three-tier system.