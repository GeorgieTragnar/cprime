# Memoize: Optimization-Only Fields in CPrime

## Core Concept

**Memoize fields are semantically invisible optimization storage** - they exist for performance but cannot affect program correctness. They are the ONLY type of field allowed in functional classes.

## 1. Syntax and Placement Rules

### In Functional Classes (Required)

```cpp
functional class ComputeOps {
    // ONLY memoize fields allowed - no regular state
    memoize cache: HashMap<Input, Output>,
    memoize workspace: Vec<u8>,
    memoize buffer_pool: BufferPool,
}
```

### In Data Classes (Optional)

```cpp
class DataModel {
    // Regular state fields
    id: u64,
    content: String,
    
    // Optimization fields
    memoize hash_cache: Option<u64>,
    memoize validation_cache: HashMap<Field, bool>,
}
```

## 2. Key Properties

### Thread-Local by Default

- Each thread gets its own instance of memoize fields
- No synchronization overhead
- Explicit `memoize static` for shared caches with synchronization

### RAII Managed

- Constructor allocates cache memory
- Destructor automatically frees cache memory
- Tied to functional class instance lifetime

### Semantically Invisible

- Cannot affect program correctness
- Can be cleared/evicted without breaking behavior
- Results should be same with or without cache (only performance differs)

## 3. Compiler Optimizations (Not Restrictions)

### Memory Management

- Thread-local allocation for better cache locality
- NUMA-aware placement on multi-socket systems
- Arena/bump allocators for cache entries
- Automatic eviction under memory pressure

### Performance Hints

- Prefetching for hot cache paths
- Profile-guided pre-sizing
- Cache-line alignment for frequently accessed entries
- LRU/LFU eviction policies

### Static Analysis

- Optional warnings for nondeterministic caching
- Dataflow analysis for optimization opportunities
- Dead cache elimination

## 4. Developer Responsibility Model

### Trust-Based System

```cpp
functional class NetworkOps {
    memoize dns_cache: HashMap<Domain, IP>,
    
    // Developer decides if caching DNS is acceptable
    // Compiler doesn't enforce determinism
    fn resolve(domain: Domain) -> IP {
        dns_cache.get(&domain).unwrap_or_else(|| ...)
    }
}
```

### Not a Borrow Checker

- No purity enforcement
- No determinism requirements
- Developer chooses appropriate cache semantics
- Compiler provides optimizations, not restrictions

## 5. Use Cases

### Pure Computation Caching

```cpp
functional class MathOps {
    memoize fibonacci_cache: HashMap<u32, u128>,
    memoize factorial_cache: HashMap<u32, u128>,
    
    fn fibonacci(n: u32) -> u128 {
        if let Some(cached) = self.fibonacci_cache.get(&n) {
            return *cached;
        }
        
        let result = match n {
            0 | 1 => n as u128,
            _ => MathOps::fibonacci(n - 1) + MathOps::fibonacci(n - 2),
        };
        
        self.fibonacci_cache.insert(n, result);
        result
    }
}
```

### Workspace Buffers

```cpp
functional class StringProcessorOps {
    memoize temp_buffers: Vec<Buffer>,
    memoize scratch_space: Vec<u8>,
    
    fn process_text(data: &TextData, input: &str) -> String {
        // Reuse buffers to avoid allocations
        let mut buffer = self.temp_buffers.pop()
            .unwrap_or_else(|| Buffer::with_capacity(4096));
        
        // Processing logic using buffer
        let result = buffer.process_string(input);
        
        // Return buffer to pool
        buffer.clear();
        self.temp_buffers.push(buffer);
        
        result
    }
}
```

### Resource Pools

```cpp
functional class NetworkOps {
    memoize connection_pool: Vec<TcpSocket>,
    memoize thread_pool: LazyThreadPool,
    
    fn make_request(url: &str) -> Result<Response> {
        let mut socket = self.connection_pool.pop()
            .unwrap_or_else(|| TcpSocket::new());
        
        let response = socket.connect_and_request(url)?;
        
        // Return socket to pool if still valid
        if socket.is_healthy() {
            self.connection_pool.push(socket);
        }
        
        Ok(response)
    }
}
```

### Approximate/Temporal Caching

```cpp
functional class DatabaseOps {
    memoize recent_queries: LRUCache<Query, Result>,
    memoize dns_cache: TimedCache<Domain, IP>,
    
    fn execute_query(conn: &DbConnection, query: &Query) -> Result<QueryResult> {
        // Check recent cache (approximate - may be stale)
        if let Some(cached) = self.recent_queries.get(query) {
            if cached.age() < Duration::from_secs(30) {
                return Ok(cached.result.clone());
            }
        }
        
        // Execute actual query
        let result = conn.execute(query)?;
        
        // Cache for future use
        self.recent_queries.insert(query.clone(), CachedResult::new(result.clone()));
        
        Ok(result)
    }
}
```

## 6. Integration with Three-Class System

### Functional Class Purity Maintained

```cpp
functional class FileOps {
    // Only memoize fields allowed in functional classes
    memoize open_handles: HashMap<PathBuf, FileHandle>,
    memoize read_buffers: Vec<Buffer>,
    
    // ❌ COMPILE ERROR: Regular fields not allowed
    // file_count: u32,  
    // current_dir: PathBuf,
    
    fn read_file(path: &PathBuf) -> Result<String> {
        // Semantic behavior independent of memoize fields
        // Performance may vary based on cache hits
    }
}
```

### Data Class Flexibility

```cpp
class DocumentData {
    // Regular semantic state
    content: String,
    metadata: DocumentMeta,
    
    // Optional optimization storage
    memoize rendered_html: Option<String>,
    memoize word_count: Option<usize>,
    
    constructed_by: DocumentOps,
}

functional class DocumentOps {
    // Only memoize fields in functional classes
    memoize render_cache: HashMap<DocumentId, RenderedOutput>,
    
    fn get_word_count(doc: &DocumentData) -> usize {
        // Check document's own memoize field first
        if let Some(count) = doc.word_count {
            return count;
        }
        
        // Compute and cache in document
        let count = doc.content.split_whitespace().count();
        // Note: In practice, this would require mutable access
        // This is just an example of the concept
        count
    }
}
```

## 7. Comparison with Existing Languages

### Novel Aspects

- **Field-level memoization** (vs function-level decorators)
- **Compiler optimization hints** (vs manual cache management)
- **Type system integration** (functional classes can ONLY have memoize)
- **Semantic invisibility** as language concept

### Similar Concepts

- C++ `mutable` - but without safety guarantees
- Rust `RefCell` - but without runtime overhead
- Python `@lru_cache` - but at field level, not function level

## 8. Runtime and Memory Model

### Thread Safety

```cpp
functional class ConcurrentOps {
    // Thread-local by default
    memoize local_cache: HashMap<Key, Value>,
    
    // Shared across threads with synchronization
    memoize static shared_cache: Arc<Mutex<HashMap<Key, Value>>>,
    
    fn process_item(item: Item) -> Result<ProcessedItem> {
        // local_cache is thread-local - no synchronization needed
        if let Some(result) = self.local_cache.get(&item.key()) {
            return Ok(result.clone());
        }
        
        // shared_cache requires synchronization
        {
            let shared = self.shared_cache.lock().unwrap();
            if let Some(result) = shared.get(&item.key()) {
                // Cache in thread-local for next time
                self.local_cache.insert(item.key(), result.clone());
                return Ok(result.clone());
            }
        }
        
        // Compute result
        let result = expensive_computation(item);
        
        // Cache in both local and shared
        self.local_cache.insert(item.key(), result.clone());
        self.shared_cache.lock().unwrap().insert(item.key(), result.clone());
        
        Ok(result)
    }
}
```

### Memory Lifecycle

```cpp
functional class ProcessorOps {
    memoize work_buffer: Vec<u8>,
    
    // Constructor can pre-allocate memoize fields
    fn construct() -> ProcessorData {
        // Note: This is conceptual - functional classes don't have constructors
        // in the same sense as data classes. This shows the idea.
        ProcessorData {
            // Regular state initialization
        }
    }
    
    // Destructor automatically handles memoize cleanup
    fn destruct(data: &mut ProcessorData) {
        // Compiler automatically generates cleanup for memoize fields
        // No manual cleanup needed
    }
}
```

## 9. Design Benefits

### Clean Separation

- **Data classes**: Own domain state + optional optimization
- **Functional classes**: Own NO state, only optimization
- **Interfaces**: Define contracts without state

### Performance Without Complexity

- Zero-cost when cache miss
- Automatic memory management
- Thread-local by default removes synchronization
- Compiler optimizations based on usage patterns

### Maintains System Invariants

- Functional classes remain stateless (semantically)
- RAII principles preserved
- Orthogonal composition unchanged
- Clear distinction between state and optimization

## 10. Advanced Patterns

### Memoize with Coroutines

```cpp
functional class AsyncOps {
    memoize pending_requests: HashMap<RequestId, CoroHandle>,
    memoize response_cache: LRUCache<Request, Response>,
    
    suspend fn fetch_data(request: Request) -> Response {
        // Check cache first
        if let Some(cached) = self.response_cache.get(&request) {
            return cached.clone();
        }
        
        // Check if already in progress
        if let Some(handle) = self.pending_requests.get(&request.id()) {
            return co_await handle;
        }
        
        // Start new request
        let handle = spawn async {
            let response = make_network_request(request).await;
            self.response_cache.insert(request.clone(), response.clone());
            response
        };
        
        self.pending_requests.insert(request.id(), handle.clone());
        let result = co_await handle;
        self.pending_requests.remove(&request.id());
        
        result
    }
}
```

### Memoize with Channels

```cpp
functional class ChannelOps {
    memoize message_cache: LRUCache<MessageId, Message>,
    memoize subscriber_cache: HashMap<TopicId, Vec<ChannelSender>>,
    
    fn publish_cached(channel: &MessageChannel, msg: Message) -> Result<()> {
        // Deduplicate messages using cache
        if self.message_cache.contains_key(&msg.id()) {
            return Ok(()); // Already published
        }
        
        // Send to all cached subscribers
        if let Some(subscribers) = self.subscriber_cache.get(&msg.topic_id()) {
            for sender in subscribers {
                sender.send(msg.clone())?;
            }
        }
        
        // Cache the message
        self.message_cache.insert(msg.id(), msg);
        Ok(())
    }
}
```

## Summary

`memoize` elegantly solves the tension between "functional classes need to be stateless" and "real code needs caching." By making optimization state explicit but semantically invisible, it enables high-performance code while maintaining clean architecture. The key insight: functional classes can ONLY have memoize fields, making them truly stateless in terms of semantic behavior while still allowing performance optimization.

### Key Principles

1. **Semantic Invisibility**: Memoize fields cannot affect program correctness
2. **Performance Only**: Exists solely for optimization, not behavior
3. **Functional Class Purity**: The only fields allowed in functional classes
4. **Developer Trust**: No compiler enforcement of purity - developer responsibility
5. **Thread Safety**: Thread-local by default, explicit sharing when needed
6. **RAII Integration**: Automatic memory management through class lifecycle
7. **Compiler Optimization**: Extensive optimization opportunities without breaking semantics

The memoize system represents a novel approach to performance optimization in systems programming, providing the benefits of caching without compromising the architectural clarity of the three-class system.