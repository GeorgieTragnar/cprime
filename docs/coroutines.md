# CPrime Coroutines

## Overview

CPrime coroutines represent a revolutionary approach to cooperative multitasking that combines the simplicity of Go-style coroutines with the control of C++ and the innovation of Rust, while avoiding the pitfalls of all three. The design is built on three key innovations:

1. **Heap-allocated stacks** that never move (preserving pointer validity)
2. **Compiler-analyzed pool allocation** with revolutionary stack-allocated micro pools
3. **Seamless integration** with CPrime's three-class system and polymorphism

## Core Architecture

### Memory Management Philosophy

Unlike traditional C++ coroutines that use compiler-generated state machines, CPrime coroutines use dedicated stack memory for each coroutine instance. This approach also integrates with CPrime's interface memory contract system for N:M composition patterns:

```cpp
// Coroutine as data class
class HttpHandlerCoro {
    // Stack memory (heap-allocated, never moves)
    stack_memory: *mut u8,
    stack_size: usize,
    stack_top: *mut u8,
    
    // Coroutine execution state
    state: CoroState,
    return_address: *const u8,
    
    // Allocation metadata
    size_tag: CoroSizeTag,
    pool_id: Option<PoolId>,
    
    constructed_by: CoroManager,
}

// Functional class managing coroutine lifecycle
functional class CoroManager {
    fn construct<F>(task: F, hint: SizeHint) -> HttpHandlerCoro {
        let size_class = compiler_analyze_size::<F>();
        let stack = allocate_stack(size_class);
        
        HttpHandlerCoro {
            stack_memory: stack.ptr,
            stack_size: stack.size,
            stack_top: stack.ptr.add(stack.size),
            state: CoroState::Ready,
            return_address: null(),
            size_tag: size_class,
            pool_id: stack.pool_id,
        }
    }
    
    fn resume(coro: &mut HttpHandlerCoro) -> CoroResult {
        // Context switch to coroutine stack
        // 50-100 cycle cost vs 300-500 for traditional
        context_switch_to(coro.stack_memory, coro.return_address)
    }
    
    fn suspend(coro: &mut HttpHandlerCoro, continuation: *const u8) {
        coro.return_address = continuation;
        coro.state = CoroState::Suspended;
        // Context switch back to caller
    }
}
```

### Revolutionary Pool Design

The breakthrough innovation is **stack-allocated micro pools** that achieve 100x memory density improvement:

```cpp
// Stack-allocated pool for micro coroutines - THE BREAKTHROUGH
alignas(64) char micro_pool[256 * 1000];  // 256KB for 1000 micro coroutines

class MicroCoroPool {
    pool_memory: &'static mut [u8; 256 * 1000],
    allocation_bitmap: [u64; 16],  // Track 1000 slots with 16 u64s
    free_count: usize,
    constructed_by: PoolManager,
}

functional class PoolManager {
    fn construct(pool_mem: &'static mut [u8]) -> MicroCoroPool {
        MicroCoroPool {
            pool_memory: pool_mem,
            allocation_bitmap: [0; 16],  // All slots free
            free_count: 1000,
        }
    }
    
    fn allocate(pool: &mut MicroCoroPool) -> Option<*mut u8> {
        // Find first free slot in bitmap - O(1) with bit scanning
        if pool.free_count == 0 { return None; }
        
        let slot_idx = find_first_zero_bit(&pool.allocation_bitmap);
        set_bit(&mut pool.allocation_bitmap, slot_idx);
        pool.free_count -= 1;
        
        Some(pool.pool_memory.as_mut_ptr().add(slot_idx * 256))
    }
    
    fn deallocate(pool: &mut MicroCoroPool, ptr: *mut u8) {
        let offset = ptr.offset_from(pool.pool_memory.as_ptr()) as usize;
        let slot_idx = offset / 256;
        
        clear_bit(&mut pool.allocation_bitmap, slot_idx);
        pool.free_count += 1;
    }
}
```

## Size Class System

### Compiler-Analyzed Size Classes

The compiler performs static analysis to categorize coroutines by stack requirements:

```cpp
enum CoroSizeTag {
    Micro,      // <256B  - Stack-allocated pools
    Small,      // <2KB   - Heap pools  
    Medium,     // <16KB  - Heap pools
    Large,      // Unbounded - Individual heap allocation
}

// Compiler generates this metadata at compile time
struct CoroMetadata {
    max_stack_size: usize,
    is_bounded: bool,
    call_depth: u32,
    recursion_detected: bool,
    size_class: CoroSizeTag,
}

// Example of compiler analysis
async fn simple_handler(request: HttpRequest) -> HttpResponse {
    // Compiler analysis:
    // - Local vars: HttpRequest (64B) + HttpResponse (128B) = 192B
    // - No function calls
    // - No recursion
    // - Bounded: true
    // Result: CoroSizeTag::Micro (fits in 256B pool)
    
    let response = HttpResponse::ok();
    response.set_body("Hello World");
    response
}

async fn complex_parser(input: &str) -> ParseResult {
    // Compiler analysis:
    // - Recursive descent parser detected
    // - Unbounded call depth
    // - Local vars: ~500B per recursive call
    // Result: CoroSizeTag::Large (individual heap allocation)
    
    parse_recursive(input, 0)
}
```

### Union-Based Storage

Coroutines use CPrime's union system for efficient heterogeneous storage:

```cpp
union runtime CoroStorage {
    Micro(MicroCoro<256>),      // Pool-allocated
    Small(SmallCoro<2048>),     // Pool-allocated
    Medium(MediumCoro<16384>),  // Pool-allocated
    Large(LargeCoro),           // Individual allocation
}

// Self-expanding coroutine containers
let mut active_coros: Vec<runtime CoroStorage> = Vec::new();

// Add different sized coroutines - union auto-expands
active_coros.push(CoroStorage::from(create_micro_handler()));    // 256B pool
active_coros.push(CoroStorage::from(create_small_service()));    // 2KB pool
active_coros.push(CoroStorage::from(create_complex_parser()));   // Individual heap -> REALLOC!

// Process all coroutines with unified API
for coro_space in &mut active_coros {
    if let Some(micro) = coro_space.try_as::<MicroCoro<256>>() {
        MicroCoroOps::resume(micro);  // Pool-based fast path
    } else if let Some(large) = coro_space.try_as::<LargeCoro>() {
        LargeCoroOps::resume(large);  // Individual allocation
    }
}
```

## Three-Class System Integration

### Coroutines as Data Classes

Coroutines perfectly fit CPrime's three-class system architecture:

```cpp
// Coroutine state as data class
class WebSocketCoro {
    // Stack and execution state
    stack_memory: *mut u8,
    stack_size: usize,
    register_state: RegisterContext,
    
    // Application data
    connection: WebSocketConnection,
    message_buffer: Vec<u8>,
    
    // Construction control
    constructed_by: WebSocketManager,
}

// Operations as functional class
functional class WebSocketManager {
    fn construct(connection: WebSocketConnection) -> WebSocketCoro {
        let size_hint = compiler_size_hint::<WebSocketCoro>();
        let stack = allocate_stack(size_hint);
        
        WebSocketCoro {
            stack_memory: stack.ptr,
            stack_size: stack.size,
            register_state: RegisterContext::new(),
            connection,
            message_buffer: Vec::new(),
        }
    }
    
    fn handle_message(coro: &mut WebSocketCoro, msg: Message) -> CoroResult {
        // Resume coroutine to process message
        let result = CoroManager::resume(coro)?;
        
        match result {
            CoroResult::Yield(response) => Ok(response),
            CoroResult::Complete(final_result) => {
                // Coroutine finished, cleanup stack
                deallocate_stack(coro.stack_memory, coro.size_tag);
                Ok(final_result)
            },
            CoroResult::Error(e) => Err(e),
        }
    }
}

// Usage follows three-class pattern
let mut websocket_handler = WebSocketManager::construct(connection);
defer WebSocketManager::destruct(&mut websocket_handler);

let response = WebSocketManager::handle_message(&mut websocket_handler, incoming_msg)?;
```

### Access Rights for Coroutine Capabilities

Coroutines integrate with CPrime's access rights system for capability-based security:

```cpp
class DatabaseCoro {
    stack_memory: *mut u8,
    connection: DbConnection,
    query_cache: QueryCache,
    
    // Mixin-style access rights (single-level polymorphism)
    exposes ReadOps { connection, query_cache }
    exposes WriteOps { connection }
    exposes AdminOps { connection, query_cache }
    
    constructed_by: DbCoroManager,
}

// Capability-specific operations
functional class ReadOps {
    fn execute_query(coro: &DatabaseCoro, sql: &str) -> QueryResult {
        // Read-only operations
        if is_select_query(sql) {
            CoroManager::resume_with_query(coro, sql)
        } else {
            Err("Read-only access: DML operations not allowed")
        }
    }
}

functional class AdminOps {
    fn execute_any_query(coro: &DatabaseCoro, sql: &str) -> QueryResult {
        // Full access - no restrictions
        CoroManager::resume_with_query(coro, sql)
    }
    
    fn clear_cache(coro: &mut DatabaseCoro) {
        QueryCacheOps::clear(&mut coro.query_cache);
    }
}

// Coroutine preserves capabilities across suspensions
async fn handle_admin_request(request: AdminRequest) -> AdminResponse {
    // This coroutine has AdminOps capabilities
    let query_result = co_await execute_query(&request.sql);  // Full access
    co_await clear_cache();  // Admin-only operation
    
    AdminResponse::success(query_result)
}

// Usage with access rights
let admin_coro = DbCoroManager::construct_admin(admin_connection);
if let Some(admin_ops) = admin_coro.cast::<AdminOps>() {
    AdminOps::execute_any_query(&admin_ops, "DROP TABLE users");  // Allowed
}
```

## Memory Layout and Performance

### Stack Memory Layout

```cpp
// Micro coroutine in stack-allocated pool
Stack Pool Layout:
┌─────────────────────────────────────────────────────────────┐
│ Micro Pool (256KB on stack)                                │
│ ┌─────────┬─────────┬─────────┬─────────┬─────────┬───────┐ │
│ │ Coro 0  │ Coro 1  │ Coro 2  │ Coro 3  │ Coro 4  │  ...  │ │
│ │ 256B    │ 256B    │ 256B    │ 256B    │ 256B    │       │ │
│ └─────────┴─────────┴─────────┴─────────┴─────────┴───────┘ │
└─────────────────────────────────────────────────────────────┘
                           ^
                           │
                    Allocation bitmap tracks free slots

// Individual coroutine stack layout
Coroutine Stack (heap-allocated):
┌─────────────────────────────────────────────────────────────┐
│ Stack Memory (grows downward)                              │
│                                                             │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ Current Frame                                           │ │ 
│ │ ┌─────────────────────────────────────────────────────┐ │ │
│ │ │ Local variables                                     │ │ │
│ │ │ Function parameters                                 │ │ │
│ │ │ Return address                                      │ │ │
│ │ └─────────────────────────────────────────────────────┘ │ │
│ └─────────────────────────────────────────────────────────┘ │
│                                                             │
│ Previous frames...                                          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Performance Characteristics

| Operation | CPrime Coroutines | Traditional C++ | Go Goroutines |
|-----------|------------------|-----------------|---------------|
| **Context Switch** | 50-100 cycles | 300-500 cycles | 200-300 cycles |
| **Memory per Micro** | 256 bytes | 64KB+ | 2KB+ |
| **Allocation Cost** | 0 (pool) | malloc cost | GC overhead |
| **Memory Density** | 1000/256KB | 16/1MB | 128/256KB |
| **Pointer Validity** | Preserved | N/A | Invalidated |

### Allocation Strategy Performance

```cpp
// Micro coroutine allocation - stack pool (fastest)
let micro_coro = MicroCoroManager::construct(simple_task);  // ~10 cycles

// Small coroutine allocation - heap pool (fast)  
let small_coro = SmallCoroManager::construct(medium_task);  // ~100 cycles

// Large coroutine allocation - individual (standard)
let large_coro = LargeCoroManager::construct(complex_task); // ~1000 cycles

// Migration between size classes (rare, but automatic)
let migrated = migrate_to_larger_class(overflowing_coro);   // ~5000 cycles
```

## Coroutine Lifecycle

### Creation and Initialization

```cpp
// Compiler-guided coroutine creation
async fn web_request_handler(request: HttpRequest) -> HttpResponse {
    // Compiler analysis determines this needs Small class (1.5KB stack)
    
    let parsed_headers = co_await parse_headers(&request.headers);
    let auth_result = co_await authenticate(&parsed_headers.auth);
    let response = co_await process_request(&request, &auth_result);
    
    response
}

// Runtime allocation based on compiler analysis
let handler_coro = CoroManager::construct(web_request_handler, request);
// Automatically allocated in Small pool (2KB) based on analysis
```

### Suspension and Resumption

```cpp
// Suspension preserves all local state on coroutine's stack
async fn database_query(connection: &DbConnection, sql: &str) -> QueryResult {
    let prepared = connection.prepare(sql)?;  // Local variable on coro stack
    
    let result = co_await prepared.execute();  // Suspend here
    // Stack contents preserved:
    // - `prepared` statement still valid
    // - `connection` reference still valid
    // - All local variables intact
    
    result.finalize()?;  // Resume continues with valid stack
    Ok(result)
}
```

### Migration Between Size Classes

```cpp
// Automatic migration when stack grows beyond current class
async fn parser_with_growth(input: &str) -> ParseResult {
    // Initially allocated as Small (2KB)
    let mut result = ParseResult::new();
    
    for line in input.lines() {
        // If recursive parsing causes stack overflow:
        let parsed_line = co_await parse_complex_line(line);  // Stack grows
        
        // Runtime detects overflow, migrates to Medium class automatically
        // All stack contents copied to new larger allocation
        // Pointers updated, execution continues seamlessly
        
        result.add_line(parsed_line);
    }
    
    Ok(result)
}
```

## Integration with CPrime's Polymorphism

### Access Rights Across Suspensions

```cpp
class ServiceCoro {
    stack_memory: *mut u8,
    service_context: ServiceContext,
    
    exposes UserServiceOps { service_context }
    exposes AdminServiceOps { service_context }
    
    constructed_by: ServiceManager,
}

async fn handle_user_request(request: UserRequest) -> UserResponse {
    // This coroutine has UserServiceOps capabilities
    let validation = co_await validate_user_input(&request);
    
    if validation.is_valid() {
        let result = co_await UserServiceOps::process_request(&request);
        UserResponse::success(result)
    } else {
        UserResponse::error("Invalid input")
    }
}

// Coroutine maintains access rights across all suspension points
let user_coro = ServiceManager::construct_user_handler(request);
// user_coro can only use UserServiceOps, not AdminServiceOps
```

### Interface Implementation

```cpp
interface CoroLifecycle {
    fn resume(&mut self) -> CoroResult;
    fn suspend(&mut self);
    fn is_complete(&self) -> bool;
}

// All coroutine types implement common lifecycle interface
impl CoroLifecycle for MicroCoro<256> {
    fn resume(&mut self) -> CoroResult {
        MicroCoroOps::resume(self)
    }
    
    fn suspend(&mut self) {
        MicroCoroOps::suspend(self)
    }
    
    fn is_complete(&self) -> bool {
        self.state == CoroState::Complete
    }
}

// Generic coroutine processing
fn process_coroutines(coros: &mut [&mut dyn CoroLifecycle]) {
    for coro in coros {
        if !coro.is_complete() {
            match coro.resume() {
                CoroResult::Yield => continue,
                CoroResult::Complete(_) => {},
                CoroResult::Error(e) => handle_error(e),
            }
        }
    }
}
```

### Union-Based Heterogeneous Collections

```cpp
// Mixed coroutine types in single container
union runtime AnyCoroutine {
    Micro(MicroCoro<256>),
    Small(SmallCoro<2048>),
    Medium(MediumCoro<16384>),
    Large(LargeCoro),
}

class CoroScheduler {
    active_coroutines: Vec<runtime AnyCoroutine>,
    constructed_by: SchedulerOps,
}

functional class SchedulerOps {
    fn add_coroutine(scheduler: &mut CoroScheduler, coro: runtime AnyCoroutine) {
        scheduler.active_coroutines.push(coro);
    }
    
    fn run_all(scheduler: &mut CoroScheduler) {
        for coro_space in &mut scheduler.active_coroutines {
            // Unified API across all coroutine sizes
            if let Some(micro) = coro_space.try_as::<MicroCoro<256>>() {
                MicroCoroOps::resume(micro);
            } else if let Some(small) = coro_space.try_as::<SmallCoro<2048>>() {
                SmallCoroOps::resume(small);
            } else if let Some(medium) = coro_space.try_as::<MediumCoro<16384>>() {
                MediumCoroOps::resume(medium);
            } else if let Some(large) = coro_space.try_as::<LargeCoro>() {
                LargeCoroOps::resume(large);
            }
        }
    }
}
```

## Advanced Features

### Compiler-Library Cooperation

```cpp
// Compiler generates metadata for each coroutine function
#[coro_metadata(
    max_stack_size = 1024,
    is_bounded = true,
    call_depth = 3,
    size_class = CoroSizeTag::Small
)]
async fn analyzed_function(data: InputData) -> OutputData {
    // Compiler has analyzed this function and determined:
    // - Maximum stack usage: 1024 bytes
    // - No unbounded recursion
    // - Maximum call depth: 3
    // - Should use Small pool (2KB)
    
    let step1 = process_step1(&data);      // +256 bytes stack
    let step2 = process_step2(&step1);     // +384 bytes stack  
    let result = finalize(&step2);         // +256 bytes stack
    // Total: 896 bytes (fits in 1024 bound)
    
    result
}

// Library uses metadata for optimal allocation
let coro = CoroManager::construct_with_metadata(
    analyzed_function,
    input_data,
    CoroMetadata::from_attributes()  // Generated by compiler
);
```

### Profile-Guided Optimization

```cpp
// Runtime can collect statistics and feedback to compiler
class CoroProfiler {
    size_class_accuracy: HashMap<FunctionId, f64>,
    migration_frequency: HashMap<FunctionId, u32>,
    average_stack_usage: HashMap<FunctionId, usize>,
    
    constructed_by: ProfilerOps,
}

functional class ProfilerOps {
    fn record_allocation(profiler: &mut CoroProfiler, func_id: FunctionId, actual_size: usize) {
        // Track if compiler's size prediction was accurate
        let predicted_size = get_predicted_size(func_id);
        let accuracy = (actual_size as f64) / (predicted_size as f64);
        
        profiler.size_class_accuracy.insert(func_id, accuracy);
    }
    
    fn record_migration(profiler: &mut CoroProfiler, func_id: FunctionId) {
        // Track how often coroutines need to migrate size classes
        *profiler.migration_frequency.entry(func_id).or_insert(0) += 1;
    }
    
    fn generate_feedback(profiler: &CoroProfiler) -> CompilerFeedback {
        // Generate feedback for next compilation
        CompilerFeedback {
            underestimated_functions: find_underestimated(profiler),
            overestimated_functions: find_overestimated(profiler),
            migration_hotspots: find_migration_hotspots(profiler),
        }
    }
}
```

## Best Practices

### When to Use Different Size Classes

```cpp
// MICRO coroutines (256B) - Use for:
async fn simple_validator(input: &str) -> bool {
    // Simple operations, no deep call stacks
    input.len() > 0 && input.chars().all(|c| c.is_alphanumeric())
}

// SMALL coroutines (2KB) - Use for:
async fn http_request_handler(request: HttpRequest) -> HttpResponse {
    // Moderate complexity, bounded operations
    let headers = parse_headers(&request.headers);
    let body = process_body(&request.body, &headers);
    build_response(body)
}

// MEDIUM coroutines (16KB) - Use for:
async fn json_parser(input: &str) -> JsonValue {
    // Recursive operations with known depth limits
    parse_json_recursive(input, 0, MAX_DEPTH)
}

// LARGE coroutines (unbounded) - Use for:
async fn compiler_backend(ast: SyntaxTree) -> MachineCode {
    // Unbounded recursion, complex algorithms
    compile_recursive(ast)  // Could be arbitrarily deep
}
```

### Performance Optimization Guidelines

```cpp
// 1. Prefer micro coroutines for high-frequency operations
let mut request_validators: Vec<MicroCoro<256>> = Vec::new();
for request in incoming_requests {
    let validator = MicroCoroManager::construct(validate_request, request);
    request_validators.push(validator);  // Zero allocation cost
}

// 2. Use compiler hints when size analysis is insufficient
#[coro_size_hint(CoroSizeTag::Small)]
async fn hint_guided_function(complex_input: ComplexData) -> Result {
    // Compiler can't analyze ComplexData, but we know it fits in Small
    process_complex_data(complex_input)
}

// 3. Pool pre-allocation for predictable workloads
let web_server_pools = CoroPoolManager::construct_pools(CoroPoolConfig {
    micro_pool_size: 10000,    // 10K micro coroutines
    small_pool_size: 1000,     // 1K small coroutines  
    medium_pool_size: 100,     // 100 medium coroutines
    large_individual: true,    // Large coroutines individually allocated
});

// 4. Avoid unnecessary migrations
async fn size_stable_function(input: BoundedInput) -> Output {
    // Process input in chunks to avoid stack growth
    let mut result = Output::new();
    for chunk in input.chunks(MAX_CHUNK_SIZE) {
        let chunk_result = process_chunk(chunk);  // Bounded stack usage
        result.merge(chunk_result);
    }
    result
}
```

## Comparison with Other Approaches

### vs Traditional C++ Coroutines

| Aspect | CPrime | Traditional C++ |
|--------|--------|-----------------|
| **Memory Model** | Dedicated stacks | Compiler state machines |
| **Pointer Validity** | Always preserved | N/A (no stack) |
| **Memory Usage** | 256B-64KB | 64KB+ per coroutine |
| **Context Switch** | 50-100 cycles | 300-500 cycles |
| **Debugging** | Standard stack traces | Complex state inspection |
| **Interop** | Full C compatibility | Limited |

### vs Go Goroutines

| Aspect | CPrime | Go |
|--------|--------|-----|
| **Stack Management** | Fixed, never move | Copying, segmented |
| **Memory Efficiency** | 100x better for micro | Good for medium |
| **Compiler Analysis** | Size prediction | Runtime detection |
| **Type System** | Integrated with classes | Separate runtime |
| **Performance** | Predictable | GC overhead |

### vs Rust Async

| Aspect | CPrime | Rust |
|--------|--------|------|
| **Memory Model** | Stack-based | Zero-cost state machines |
| **Complexity** | Moderate | High (lifetime management) |
| **Memory Usage** | Pool-optimized | Minimal per future |
| **Debugging** | Standard debugging | Future combinators |
| **Learning Curve** | Familiar to C++ | Steep |

## Future Directions

### Planned Enhancements

1. **NUMA-Aware Pool Allocation**: Thread-local pools with work-stealing
2. **Hardware Stack Protection**: Use guard pages for overflow detection
3. **Async I/O Integration**: Kernel bypass for high-performance networking
4. **Real-time Scheduling**: Priority-based coroutine scheduling
5. **Cross-Platform Optimization**: Platform-specific context switching

### Research Areas

1. **Automatic Size Class Learning**: ML-based size prediction improvement
2. **Memory Locality Optimization**: Pool placement for cache efficiency  
3. **Compiler-Runtime Co-design**: Tighter integration between analysis and execution
4. **Formal Verification**: Proving safety properties of the stack management system

## Channel Integration

### Coroutine Suspension on Channels

Coroutines seamlessly suspend and resume on channel operations, with the scheduler managing channel references to prevent use-after-free:

```cpp
// Coroutine suspending on channel operations
async fn channel_worker(work_ch: Channel<WorkItem>) -> Result<()> {
    loop {
        // Coroutine suspends here if channel is empty
        // Scheduler holds reference to channel during suspension
        match co_await work_ch.recv() {
            Some(work) => {
                // Process work item
                let result = process_work_item(work);
                
                // May suspend again if result channel is full
                co_await result_ch.send(result);
            },
            None => {
                // Channel closed - clean termination
                println("Work channel closed, terminating");
                break;
            }
        }
    }
    Ok(())
}

// How suspension works internally
functional class ChannelCoroManager {
    fn suspend_on_recv<T>(
        coro: &mut Coroutine,
        channel: &Channel<T>
    ) -> SuspensionToken {
        // 1. Save coroutine state
        save_register_context(&mut coro.register_context);
        
        // 2. Register with scheduler (prevents channel deallocation)
        let token = SchedulerOps::register_channel_waiter(
            coro.id,
            channel.get_shared_ref()  // Increment ref count
        );
        
        // 3. Add to channel's wait queue
        channel.recv_waiters.push(coro.id);
        
        // 4. Suspend coroutine
        coro.state = CoroState::SuspendedOnChannel(channel.id);
        
        token
    }
    
    fn resume_from_channel<T>(
        coro: &mut Coroutine,
        value: Option<T>
    ) -> CoroResult {
        // 1. Restore register context
        restore_register_context(&coro.register_context);
        
        // 2. Remove from scheduler's channel tracking
        SchedulerOps::unregister_channel_waiter(coro.id);
        
        // 3. Resume with received value
        coro.state = CoroState::Running;
        context_switch_to(coro.stack_memory, value)
    }
}
```

### Scheduler's Role in Channel Lifecycle

The scheduler is critical for channel safety, holding references to prevent premature deallocation:

```cpp
// Scheduler manages channel references for safety
class ChannelScheduler {
    // Channels with suspended coroutines - can't be freed
    active_channels: HashMap<ChannelId, SharedPtr<ChannelData>>,
    
    // Mapping of coroutines to channels they're waiting on
    channel_waiters: HashMap<CoroId, ChannelId>,
    
    // Reverse mapping for efficient wake operations
    waiting_on_channel: HashMap<ChannelId, Vec<CoroId>>,
    
    constructed_by: ChannelSchedulerOps,
}

functional class ChannelSchedulerOps {
    fn register_channel_waiter(
        scheduler: &mut ChannelScheduler,
        coro_id: CoroId,
        channel_ref: SharedPtr<ChannelData>
    ) {
        let channel_id = channel_ref.id();
        
        // Hold reference to prevent deallocation
        scheduler.active_channels.insert(channel_id, channel_ref);
        
        // Track suspension relationship
        scheduler.channel_waiters.insert(coro_id, channel_id);
        scheduler.waiting_on_channel
            .entry(channel_id)
            .or_default()
            .push(coro_id);
    }
    
    fn handle_channel_close(
        scheduler: &mut ChannelScheduler,
        channel_id: ChannelId
    ) {
        // Wake all coroutines waiting on this channel
        if let Some(waiters) = scheduler.waiting_on_channel.remove(&channel_id) {
            for coro_id in waiters {
                // Wake with None to indicate channel closed
                Self::wake_with_closed_signal(scheduler, coro_id);
                
                // Remove waiter tracking
                scheduler.channel_waiters.remove(&coro_id);
            }
        }
        
        // Release scheduler's reference
        // Channel deallocated when last reference dropped
        scheduler.active_channels.remove(&channel_id);
    }
    
    fn wake_with_closed_signal(
        scheduler: &mut ChannelScheduler,
        coro_id: CoroId
    ) {
        // Resume coroutine with None to indicate closed channel
        let coro = get_coroutine_mut(coro_id);
        ChannelCoroManager::resume_from_channel(coro, None);
    }
}
```

### Channel Operations in Coroutine Stack Analysis

The compiler includes channel operations in stack size analysis:

```cpp
// Compiler analysis of channel-using coroutines
async fn analyzed_channel_handler(
    input: Channel<Request>,
    output: Channel<Response>
) -> Result<()> {
    // Compiler analysis:
    // - Local vars: Request (256B) + Response (128B) = 384B
    // - Channel refs: 2 * 8B = 16B
    // - Suspension state: 2 * 32B = 64B (two suspension points)
    // Total: 464B -> CoroSizeTag::Small
    
    loop {
        let request = co_await input.recv()?;     // Suspension point 1
        let response = process_request(request);
        co_await output.send(response)?;          // Suspension point 2
    }
}

// Metadata generated by compiler
CoroMetadata {
    function_id: hash_of!("analyzed_channel_handler"),
    estimated_stack_size: 464,
    max_stack_size: Some(512),
    suspension_point_count: 2,
    size_class: CoroSizeTag::Small,
    uses_channels: true,
}
```

## N:M Composition with Interface Memory Contracts

### Generic Coroutine Operations

Interface memory contracts enable generic coroutine operations that work across different coroutine types:

```cpp
// Interface for coroutines that can be scheduled
interface Schedulable {
    memory_contract {
        coro_id: u64,           // Unique coroutine identifier
        priority: u32,          // Scheduling priority
        state: u32,             // Current execution state
        stack_usage: u32,       // Current stack utilization
    }
    
    fn can_resume(&self) -> bool;
    fn get_next_wake_time(&self) -> Option<SystemTime>;
}

// HTTP handler coroutine implementing Schedulable
class HttpHandlerCoro {
    coroutine_id: u64,      // Maps to coro_id
    request_priority: u32,  // Maps to priority
    execution_state: CoroState, // Maps to state (as u32)
    stack_used: u32,        // Maps to stack_usage
    
    stack_memory: *mut u8,
    response: HttpResponse,
    processing_stage: ProcessingStage,
    
    implements Schedulable {
        coro_id <- coroutine_id,
        priority <- request_priority,
        state <- execution_state as u32,
        stack_usage <- stack_used,
    }
    
    exposes HttpCoroOps { stack_memory, response, processing_stage }
    constructed_by: HttpCoroManager,
}

// Coroutines with semconst fields for atomic state transitions
class StatefulCoro {
    stack_memory: *mut u8,
    stack_size: usize,
    semconst state_snapshot: CoroutineState,  // Atomic state replacement
    mutable working_buffer: Vec<u8>,           // Incremental updates
    
    constructed_by: StatefulCoroManager,
}

functional class StatefulCoroManager {
    suspend fn update_state_atomically(coro: &mut StatefulCoro, new_state: CoroutineState) {
        // Can suspend while building new state
        let processed_state = co_await process_state_change(new_state);
        
        // Atomic replacement - other coroutines see complete transition
        let old_state = move(coro.state_snapshot);
        coro.state_snapshot = move(processed_state);
        
        // semconst ensures no partial states visible across suspension points
    }
}

// Database coroutine implementing same interface
class DatabaseCoro {
    db_task_id: u64,        // Maps to coro_id
    query_priority: u32,    // Maps to priority
    task_state: u32,        // Maps to state
    memory_usage: u32,      // Maps to stack_usage
    
    connection: DbConnection,
    query_buffer: QueryBuffer,
    result_cache: ResultCache,
    
    implements Schedulable {
        coro_id <- db_task_id,
        priority <- query_priority,
        state <- task_state,
        stack_usage <- memory_usage,
    }
    
    exposes DatabaseCoroOps { connection, query_buffer, result_cache }
    constructed_by: DatabaseCoroManager,
}

// Generic scheduler working with any Schedulable coroutine
functional class SchedulerOps<T: Schedulable> {
    fn should_preempt(current: &T, candidate: &T) -> bool {
        // Priority-based scheduling decision
        candidate.priority > current.priority &&
        candidate.can_resume()
    }
    
    fn estimate_completion_time(coro: &T) -> Option<Duration> {
        // Generic estimation based on interface data
        if coro.stack_usage > 80 {
            // High stack usage suggests complex operation
            Some(Duration::from_millis(100))
        } else if coro.priority > 7 {
            // High priority suggests quick task
            Some(Duration::from_millis(10))
        } else {
            coro.get_next_wake_time()
                .map(|wake_time| wake_time.duration_since(SystemTime::now()).unwrap_or_default())
        }
    }
    
    fn calculate_scheduling_score(coro: &T) -> f64 {
        let priority_factor = coro.priority as f64 / 10.0;
        let efficiency_factor = 1.0 - (coro.stack_usage as f64 / 100.0);
        
        priority_factor * efficiency_factor
    }
}

// Usage: Single scheduler implementation works with multiple coroutine types
let http_coro = HttpCoroManager::construct(request);
let db_coro = DatabaseCoroManager::construct(query);

let http_score = SchedulerOps::calculate_scheduling_score(&http_coro);
let db_score = SchedulerOps::calculate_scheduling_score(&db_coro);

if SchedulerOps::should_preempt(&http_coro, &db_coro) {
    scheduler.preempt_and_schedule(db_coro);
}
```

### Multi-Type Coroutine Containers

Interface contracts enable heterogeneous coroutine collections:

```cpp
// Interface for coroutines that can be suspended/resumed
interface Resumable {
    memory_contract {
        task_id: u64,
        resume_point: u32,
        last_activity: u64,
    }
    
    fn resume(&mut self) -> ResumeResult;
    fn can_suspend(&self) -> bool;
}

// Multiple coroutine types implementing Resumable
impl Resumable for HttpHandlerCoro {
    // Maps fields to interface contract
    task_id <- coroutine_id,
    resume_point <- processing_stage as u32,
    last_activity <- last_request_time,
    
    fn resume(&mut self) -> ResumeResult {
        HttpCoroOps::resume_http_processing(self)
    }
    
    fn can_suspend(&self) -> bool {
        matches!(self.processing_stage, ProcessingStage::WaitingForResponse)
    }
}

impl Resumable for DatabaseCoro {
    // Maps different fields to same interface contract
    task_id <- db_task_id,
    resume_point <- current_query_step,
    last_activity <- last_query_time,
    
    fn resume(&mut self) -> ResumeResult {
        DatabaseCoroOps::resume_query_processing(self)
    }
    
    fn can_suspend(&self) -> bool {
        self.connection.is_idle()
    }
}

// Generic coroutine pool using interface contracts
class CoroutinePool {
    // Heterogeneous collection of resumable coroutines
    active_tasks: Vec<Box<dyn Resumable>>,
    task_count: usize,
    
    constructed_by: CoroutinePoolOps,
}

functional class CoroutinePoolOps {
    fn add_task<T: Resumable + 'static>(
        pool: &mut CoroutinePool,
        task: T
    ) {
        pool.active_tasks.push(Box::new(task));
        pool.task_count += 1;
    }
    
    fn resume_all_ready(pool: &mut CoroutinePool) -> Vec<ResumeResult> {
        let mut results = Vec::new();
        
        for task in &mut pool.active_tasks {
            if !task.can_suspend() {
                results.push(task.resume());
            }
        }
        
        results
    }
    
    fn cleanup_completed(pool: &mut CoroutinePool) {
        pool.active_tasks.retain(|task| {
            let age = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap()
                .as_secs() - task.last_activity;
                
            age < 300  // Keep tasks active for 5 minutes
        });
        
        pool.task_count = pool.active_tasks.len();
    }
}

// Usage: Add different coroutine types to same pool
let mut pool = CoroutinePoolOps::construct();

CoroutinePoolOps::add_task(&mut pool, http_handler_coro);
CoroutinePoolOps::add_task(&mut pool, database_coro);
CoroutinePoolOps::add_task(&mut pool, file_io_coro);

// Resume all ready tasks regardless of their concrete type
let results = CoroutinePoolOps::resume_all_ready(&mut pool);
```

This N:M composition approach enables powerful coroutine management patterns where a single scheduler implementation can work with multiple coroutine types, and heterogeneous coroutine collections can be managed uniformly through interface contracts.

The combination of CPrime's three-class system with interface memory contracts makes coroutines extremely composable while maintaining the revolutionary performance characteristics of the underlying implementation.
}
```

### Select Statement in Coroutines

Coroutines can use select to wait on multiple channels efficiently:

```cpp
async fn multi_channel_handler(
    work_ch: Channel<Work>,
    control_ch: Channel<Control>,
    timeout_ms: u64
) -> Result<()> {
    loop {
        // Select suspends coroutine until one channel is ready
        select {
            work = work_ch.recv() => {
                match work {
                    Some(w) => process_work(w),
                    None => return Ok(()),  // Work channel closed
                }
            },
            
            control = control_ch.recv() => {
                match control {
                    Some(Control::Pause) => {
                        co_await pause_processing();
                    },
                    Some(Control::Resume) => {
                        resume_processing();
                    },
                    Some(Control::Stop) | None => {
                        return Ok(());  // Stop or channel closed
                    }
                }
            },
            
            _ = timer::after(timeout_ms) => {
                println("Timeout - no activity");
                check_health();
            }
        }
    }
}

// Select implementation for coroutines
functional class SelectCoroManager {
    fn setup_select(
        coro: &mut Coroutine,
        cases: &[SelectCase]
    ) -> SelectToken {
        // Register with all channels in select
        let mut registrations = Vec::new();
        
        for case in cases {
            match case {
                SelectCase::Recv(channel) => {
                    let reg = SchedulerOps::register_select_waiter(
                        coro.id,
                        channel.get_shared_ref()
                    );
                    registrations.push(reg);
                },
                SelectCase::Send(channel, value) => {
                    let reg = SchedulerOps::register_select_sender(
                        coro.id,
                        channel.get_shared_ref(),
                        value
                    );
                    registrations.push(reg);
                },
                SelectCase::Timeout(duration) => {
                    SchedulerOps::register_timeout(coro.id, duration);
                },
            }
        }
        
        SelectToken { registrations }
    }
    
    fn resume_from_select(
        coro: &mut Coroutine,
        ready_case: SelectResult
    ) -> CoroResult {
        // Clean up all select registrations
        SchedulerOps::cleanup_select_registrations(coro.id);
        
        // Resume with the ready case result
        context_switch_to(coro.stack_memory, ready_case)
    }
}
```

### Coroutine Pools with Channel Affinity

Optimize coroutine allocation for channel-heavy workloads:

```cpp
// Pool optimization for channel-based coroutines
class ChannelCoroPool {
    // Pools organized by channel access pattern
    send_only_pool: MicroCoroPool,      // Send-only coroutines
    recv_only_pool: MicroCoroPool,      // Receive-only coroutines  
    send_recv_pool: SmallCoroPool,      // Both send and receive
    select_pool: MediumCoroPool,        // Select on multiple channels
    
    constructed_by: ChannelCoroPoolOps,
}

functional class ChannelCoroPoolOps {
    fn allocate_for_pattern(
        pool: &mut ChannelCoroPool,
        pattern: ChannelAccessPattern
    ) -> CoroAllocation {
        match pattern {
            ChannelAccessPattern::SendOnly => {
                // Minimal stack needed for send-only
                pool.send_only_pool.allocate()
            },
            ChannelAccessPattern::RecvOnly => {
                // Minimal stack for receive-only
                pool.recv_only_pool.allocate()
            },
            ChannelAccessPattern::SendRecv => {
                // More stack for bidirectional
                pool.send_recv_pool.allocate()
            },
            ChannelAccessPattern::MultiSelect => {
                // Larger stack for complex select
                pool.select_pool.allocate()
            },
        }
    }
}

// Compiler hints for channel patterns
#[channel_pattern(SendOnly)]
async fn producer(ch: Channel<Item>) {
    loop {
        let item = generate_item();
        co_await ch.send(item);
    }
}

#[channel_pattern(RecvOnly)]
async fn consumer(ch: Channel<Item>) {
    loop {
        match co_await ch.recv() {
            Some(item) => process(item),
            None => break,
        }
    }
}
```

### Access Rights with Channel Coroutines

Coroutines preserve channel access rights across suspensions:

```cpp
// Coroutine with channel access rights
class ChannelWorkerCoro {
    stack_memory: *mut u8,
    channel_ref: &Channel<Message>,
    
    // Access rights preserved across suspensions
    exposes SendOps { channel_ref }
    exposes RecvOps { channel_ref }
    
    constructed_by: ChannelWorkerManager,
}

async fn typed_channel_worker(ch: &Channel<Message> with RecvOps) {
    // This coroutine can only receive, not send
    loop {
        match co_await RecvOps::recv(ch) {
            Some(msg) => process_message(msg),
            None => break,
        }
        // co_await SendOps::send(ch, response);  // Compile error!
    }
}

// Spawn coroutines with specific channel capabilities
fn spawn_channel_workers(bus: &MessageBus) {
    // Producers get send-only access
    spawn producer_coro(bus with SendOps);
    spawn producer_coro(bus with SendOps);
    
    // Consumers get receive-only access  
    spawn consumer_coro(bus with RecvOps);
    spawn consumer_coro(bus with RecvOps);
    
    // Supervisor gets both capabilities
    spawn supervisor_coro(bus with SendOps, RecvOps);
}
```

## Signal Integration with Coroutine Scheduler

### Dedicated Signal Thread Architecture

The coroutine scheduler runtime features a revolutionary signal handling architecture that converts OS signals into scheduled coroutines, enabling fully asynchronous signal processing without blocking worker threads.

#### Signal Thread Design

```cpp
// Signal thread implementation within scheduler
class CoroutineScheduler {
    signal_thread: Option<ThreadHandle>,
    signal_handlers: HashMap<Signal, AsyncSignalHandler>,
    signal_coroutine_pool: MicroCoroPool,
    
    constructed_by: SchedulerOps,
}

functional class SchedulerOps {
    fn spawn_signal_thread(scheduler: &mut CoroutineScheduler) {
        // Block all signals in worker threads
        block_all_signals_in_workers();
        
        // Spawn dedicated signal handling thread
        scheduler.signal_thread = Some(thread::spawn(|| {
            signal_handler_thread_main(scheduler.get_shared_ref())
        }));
    }
}

// The signal thread - core of the architecture
fn signal_handler_thread_main(scheduler: SharedPtr<CoroutineScheduler>) {
    // Only this thread receives OS signals
    unblock_all_signals();
    
    loop {
        // Synchronous signal waiting - no race conditions
        let signal = sigwait(&all_signals_mask);
        
        // Convert signal to coroutine
        if let Some(handler) = scheduler.get_signal_handler(signal) {
            // CREATE NEW COROUTINE for async signal handling
            let signal_coro = create_signal_coroutine(signal, handler);
            
            // SCHEDULE coroutine in normal work queue
            scheduler.work_queue.push_high_priority(signal_coro);
            
            // Signal handling is now async!
        } else {
            // Handle unregistered signals
            handle_default_signal(signal);
        }
    }
}
```

#### Signal-to-Coroutine Conversion Process

The scheduler transforms OS signals into coroutines through a structured process:

```cpp
// Signal coroutine structure
class SignalCoroutine {
    // Standard coroutine stack management
    stack_memory: *mut u8,
    stack_size: usize,
    register_context: RegisterContext,
    
    // Signal-specific data
    signal_type: Signal,
    signal_data: SignalData,
    handler_function: AsyncSignalHandler,
    created_at: SystemTime,
    priority: u8,  // Signals get high priority
    
    constructed_by: SignalCoroOps,
}

functional class SignalCoroOps {
    fn create_signal_coroutine(
        signal: Signal,
        handler: AsyncSignalHandler
    ) -> SignalCoroutine {
        // Allocate from signal-specific pool (micro size)
        let stack = allocate_signal_stack(SIGNAL_STACK_SIZE);
        
        SignalCoroutine {
            stack_memory: stack.ptr,
            stack_size: SIGNAL_STACK_SIZE,
            register_context: RegisterContext::new(),
            signal_type: signal,
            signal_data: SignalData::from(signal),
            handler_function: handler,
            created_at: SystemTime::now(),
            priority: SIGNAL_PRIORITY,  // High priority
        }
    }
    
    async fn execute(coro: &mut SignalCoroutine) -> SignalResult {
        // Execute the async signal handler
        (coro.handler_function)(coro.signal_data).await
    }
}
```

#### Signal Handler Registration

Applications register async signal handlers with the scheduler:

```cpp
// Example signal handler registration
fn main() = std::CoroutineScheduler {
    let scheduler = CoroutineScheduler::new()
        .handle(SIGINT, |signal_data| async {
            println!("Graceful shutdown initiated by signal");
            
            // Can perform full async operations
            drain_work_queues().await;
            close_network_connections().await;
            flush_databases().await;
            save_application_state().await;
            
            println!("Shutdown complete");
            std::process::exit(0);
        })
        .handle(SIGUSR1, |signal_data| async {
            // Async debug operations
            let stats = collect_runtime_statistics().await;
            let report = generate_performance_report(stats).await;
            
            // Can interact with channels
            debug_channel.send(report).await;
        })
        .handle(OOM, |oom_data| async {
            // Handle out-of-memory as async operation
            println!("OOM detected: requested {}, available {}", 
                    oom_data.requested_size, oom_data.available_memory);
            
            // Async memory recovery
            free_application_caches().await;
            garbage_collect_unused_coroutines().await;
            
            if try_emergency_allocation(oom_data.requested_size) {
                println!("OOM recovery successful");
            } else {
                initiate_emergency_shutdown().await;
            }
        });
    
    // Start application with signal handling
    run_application(scheduler).await;
}
```

### Worker Thread Signal Conversion

When worker threads receive unhandled signals (which should be rare due to signal masking), they convert the signal to a coroutine before terminating:

```cpp
// Worker thread signal handling
fn worker_thread_signal_conversion(
    thread_id: ThreadId,
    signal: Signal,
    scheduler: &CoroutineScheduler
) {
    println!("Worker thread {} received unexpected signal: {}", thread_id, signal);
    
    // 1. Thread performs complete unwinding
    unwind_thread_stack();
    run_all_thread_defers();
    
    // 2. Check if scheduler can handle this signal
    if let Some(handler) = scheduler.get_signal_handler(signal) {
        // 3. DYING THREAD creates signal coroutine
        let signal_coro = SignalCoroOps::create_signal_coroutine(signal, handler);
        
        // 4. DYING THREAD pushes to scheduler work queue
        if let Ok(_) = scheduler.work_queue.try_push_priority(signal_coro) {
            println!("Thread {} converted signal to coroutine, exiting cleanly", thread_id);
        } else {
            eprintln!("Thread {} failed to queue signal coroutine", thread_id);
        }
        
        // 5. Thread exits cleanly
        thread::exit(0);
    } else {
        // No handler available - terminate program
        eprintln!("Unhandled signal {} in worker thread {}", signal, thread_id);
        std::process::terminate();
    }
}
```

### Integration with Channel System

The scheduler's signal handling integrates seamlessly with the existing channel safety system:

```cpp
// Signal handlers can safely use channels
scheduler.handle(NETWORK_RECONNECT, |reconnect_data| async {
    // Signal handler can send to channels
    network_event_channel.send(NetworkEvent::ReconnectRequested {
        endpoint: reconnect_data.failed_endpoint,
        timestamp: SystemTime::now(),
    }).await;
    
    // Can receive from channels
    match control_channel.recv_timeout(Duration::from_secs(5)).await {
        Some(ControlMessage::ApproveReconnect) => {
            attempt_reconnection(reconnect_data.failed_endpoint).await;
        },
        Some(ControlMessage::DenyReconnect) => {
            mark_endpoint_failed(reconnect_data.failed_endpoint).await;
        },
        None => {
            // Timeout - use default policy
            attempt_reconnection_with_backoff(reconnect_data.failed_endpoint).await;
        }
    }
});

// Scheduler maintains channel references for signal coroutines
functional class SignalChannelManager {
    fn register_signal_channel_usage(
        scheduler: &mut CoroutineScheduler,
        signal_coro_id: CoroId,
        channels: Vec<ChannelId>
    ) {
        // Hold references to prevent channel deallocation
        for channel_id in channels {
            scheduler.signal_channel_refs.insert(
                (signal_coro_id, channel_id),
                scheduler.get_channel_ref(channel_id)
            );
        }
    }
    
    fn cleanup_signal_channel_refs(
        scheduler: &mut CoroutineScheduler,
        completed_signal_coro: CoroId
    ) {
        // Release channel references when signal handling completes
        scheduler.signal_channel_refs.retain(|(coro_id, _), _| {
            *coro_id != completed_signal_coro
        });
    }
}
```

### Signal Coroutine Pool Management

Signal coroutines use specialized pool management for optimal performance:

```cpp
// Signal-specific coroutine pools
class SignalCoroPool {
    // Most signals are simple - use micro pool
    micro_signal_pool: MicroCoroPool,      // 256B stacks
    
    // Complex signal handlers - use small pool
    complex_signal_pool: SmallCoroPool,    // 2KB stacks
    
    // Emergency pool for critical signals
    emergency_pool: SmallCoroPool,         // Always reserved
    
    constructed_by: SignalPoolOps,
}

functional class SignalPoolOps {
    fn allocate_signal_stack(
        pool: &mut SignalCoroPool,
        signal_type: Signal,
        handler_complexity: HandlerComplexity
    ) -> StackAllocation {
        match (signal_type, handler_complexity) {
            // Critical signals get emergency pool
            (SIGSEGV | SIGABRT | OOM, _) => {
                pool.emergency_pool.allocate()
            },
            
            // Simple handlers use micro pool
            (_, HandlerComplexity::Simple) => {
                pool.micro_signal_pool.allocate()
            },
            
            // Complex handlers use small pool
            (_, HandlerComplexity::Complex) => {
                pool.complex_signal_pool.allocate()
            }
        }
    }
}

// Compiler analysis determines handler complexity
#[signal_handler_complexity(Simple)]
async fn simple_debug_handler(signal_data: SignalData) {
    // Simple handlers - fit in 256B
    println!("Debug signal received at {}", SystemTime::now());
    dump_basic_stats();
}

#[signal_handler_complexity(Complex)]
async fn complex_recovery_handler(oom_data: OOMData) {
    // Complex handlers need more stack space
    let memory_stats = analyze_memory_usage().await;
    let recovery_plan = create_recovery_strategy(memory_stats).await;
    execute_memory_recovery(recovery_plan).await;
}
```

### Performance Characteristics of Signal Handling

The scheduler's signal architecture has specific performance implications:

```cpp
// Performance comparison
// Direct signal handler (default runtime): ~100 cycles
// Signal-to-coroutine (scheduler runtime): ~1000 cycles
// But enables full async operations in handlers!

// Micro benchmark example
async fn signal_performance_test() {
    let start = SystemTime::now();
    
    // This creates a coroutine, schedules it, and executes it
    raise(TEST_SIGNAL);
    
    let end = SystemTime::now();
    println!("Signal-to-coroutine conversion: {:?}", end.duration_since(start));
}

// Benefits of coroutine-based signal handling:
// 1. Can perform I/O operations
// 2. Can wait on channels and futures
// 3. Can interact with other coroutines
// 4. Full error handling with catch/recover
// 5. Integrates with scheduler's work stealing
```

### Signal Handling Best Practices in Scheduler Runtime

```cpp
// Good: Async signal handler using scheduler capabilities
scheduler.handle(BACKUP_SIGNAL, |backup_data| async {
    // Leverage async I/O
    let backup_status = check_backup_system_status().await;
    
    if backup_status.is_healthy() {
        // Can perform complex async operations
        let data_snapshot = create_application_snapshot().await;
        write_backup_data(data_snapshot).await;
        
        // Notify through channels
        status_channel.send(BackupStatus::Complete).await;
    } else {
        // Error handling with full async context
        let alternative = find_alternative_backup().await;
        retry_backup_with_alternative(alternative).await;
    }
});

// Bad: Trying to use blocking operations
scheduler.handle(BAD_SIGNAL, |_| async {
    // DON'T DO THIS - blocks the coroutine
    std::thread::sleep(Duration::from_secs(5));  // Bad!
    
    // DO THIS instead - async sleep
    tokio::time::sleep(Duration::from_secs(5)).await;  // Good!
});

// Good: Signal handlers that coordinate with application
scheduler.handle(COORDINATION_SIGNAL, |coord_data| async {
    // Can coordinate with other parts of application
    let coordination_response = coordination_channel
        .send_and_wait_response(coord_data)
        .await;
    
    match coordination_response {
        CoordinationResult::Success => {
            log_coordination_success().await;
        },
        CoordinationResult::Retry => {
            // Schedule retry as new coroutine
            spawn_delayed(Duration::from_secs(30), async {
                retry_coordination().await;
            });
        },
        CoordinationResult::Abort => {
            initiate_graceful_shutdown().await;
        }
    }
});
```

For signal handling language primitives and syntax, see [Signal Handling](signal-handling.md). For runtime selection and policies, see [Runtime System](runtime-system.md).

---

CPrime coroutines represent a significant advance in cooperative multitasking, combining the best aspects of existing approaches while introducing revolutionary optimizations like stack-allocated micro pools and compiler-guided allocation strategies. The seamless integration with channels and the innovative signal-to-coroutine conversion system provides a powerful foundation for concurrent programming with memory safety guaranteed through scheduler-managed reference counting.