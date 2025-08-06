# CPrime Concurrency Model

## Overview

CPrime adopts Go's proven concurrency model with coroutines (like goroutines), channels for communication, and a defer statement for resource management. This provides simple, efficient concurrent programming with M:N threading and structured concurrency patterns.

## Core Concurrency Primitives

### Coroutines (Go-Inspired Goroutines)

Coroutines are lightweight, cooperative threads managed by the runtime:

```cpp
fn main() {
    // Fire and forget coroutines
    spawn process_data();
    spawn handle_network();
    spawn background_cleanup();
    
    // Main thread continues execution
    run_main_loop();
    
    // Structured concurrency - wait for completion
    parallel {
        spawn compute_task_a();
        spawn compute_task_b();
        spawn compute_task_c();
    } // Blocks until all spawned tasks complete
    
    println("All tasks completed");
}

// Any function can be a coroutine
fn fetch_data(url: &str) -> Result<Data> {
    let response = http_client.get(url);  // Blocks coroutine, not thread
    let data = response.json().await;     // Cooperative yield point
    Ok(data)
}
```

### Coroutine Properties

1. **Lightweight**: Minimal memory overhead (few KB stack)
2. **Cooperative**: Yield control at well-defined points
3. **M:N threading**: Many coroutines multiplexed on few OS threads
4. **Structured lifetime**: Clear creation and completion semantics

### Spawn Semantics

```cpp
// Basic spawn - fire and forget
spawn background_task();

// Spawn with return value collection
let handle = spawn_with_handle(compute_result());
let result = handle.await;

// Spawn with explicit lifetime management
{
    let _guard = spawn_scoped(temporary_task());
    // Guard ensures task completion before scope exit
} // temporary_task() guaranteed to complete here
```

## Channel Communication

### Channel Types

Channels are shared concurrent queues where each item is consumed by exactly ONE receiver (competitive consumption for work distribution):

```cpp
// Unbuffered channel - synchronous communication
let ch = channel<Message>();

// Buffered channel - asynchronous up to capacity
let ch = channel<Message>(100);

// Channels are shared - multiple coroutines can send/receive
// but each message is consumed by only ONE receiver
let work_ch = channel<Work>();

// Spawn multiple workers sharing the same channel
for i in 0..4 {
    spawn worker(work_ch.clone());  // All workers compete for work items
}

// Different from broadcast (separate concept for events)
// Broadcast: All observers see each message (event notification)
// Channel: One receiver gets each message (work distribution)
```

### Broadcast Pattern (Different from Channels)

Broadcast is a separate pattern for event notification where all subscribers see each event:

```cpp
// Event broadcast - all subscribers see all events
class EventBus {
    subscribers: Vec<EventHandler>,
    constructed_by: EventBusOps,
}

functional class EventBusOps {
    fn emit(bus: &EventBus, event: Event) {
        // All subscribers receive the event
        for subscriber in &bus.subscribers {
            subscriber.handle(event.clone());
        }
    }
}

// This is fundamentally different from channels:
// - Channels: send(1) → Receiver A OR B OR C gets it (work distribution)
// - Broadcast: emit(1) → Receiver A AND B AND C get it (event notification)
```

### Channel Operations

```cpp
// Sending (blocking if unbuffered/full)
co_await ch.send(message);                 // Async send with co_await
ch.try_send(message)?;                     // Non-blocking send
ch.send_timeout(message, 1.second())?;    // Timeout send

// Receiving - returns Option<T> to handle close
let msg = co_await ch.recv();              // Returns Option<Message>
match msg {
    Some(m) => process(m),                 // Got message
    None => break,                          // Channel closed
}

// Non-blocking receive
let msg = ch.try_recv()?;                  // Non-blocking receive
let msg = ch.recv_timeout(1.second())?;    // Timeout receive

// Channel closing - language-level semantic
ch.close();                                // Mark channel as closed
                                          // Scheduler wakes all waiting coroutines
                                          // Future receives return None

// Iterate until closed
loop {
    match co_await ch.recv() {
        Some(msg) => process(msg),
        None => break,  // Channel closed
    }
}
```

### Channel Close Semantics

Close is a fundamental language-level operation with scheduler-managed lifecycle:

```cpp
// Close behavior:
// 1. Mark channel state as closed
// 2. Wake all coroutines suspended on recv() 
// 3. Future recv() calls return None immediately
// 4. send() on closed channel returns error

// Memory safety through reference counting:
// - Scheduler holds references to channels with waiting coroutines
// - Channel can't be freed while waiters exist
// - Close just marks state, scheduler handles wake

let ch = channel<Work>();

// Worker that handles close gracefully
async fn worker(ch: Channel<Work>) {
    loop {
        match co_await ch.recv() {
            Some(work) => process(work),
            None => {
                println("Channel closed, worker terminating");
                break;
            }
        }
    }
}
```

### Select Statement

The select statement enables non-blocking communication with block-scoped type flow:

```cpp
// Select creates implicit union of outcomes
// Each branch has block-scoped types
select {
    // Variables exist only in their branch scope
    x: i32 = ch1.recv() => {
        // x only exists here as i32
        println("Received i32 from ch1: {}", x);
    }
    s: String = ch2.recv() => {
        // s only exists here as String
        println("Received string from ch2: {}", s);
    }
    result = ch3.send(data) => {
        // Check if send succeeded (channel might be closed)
        if result.is_ok() {
            println("Sent to ch3");
        }
    }
    _ = timer::after(1.second()) => {
        println("Timeout after 1 second");
    }
    default => {
        println("No channels ready, non-blocking");
    }
}

// Complex select with multiple channels
select {
    control_msg = control_ch.recv() => {
        match control_msg {
            ControlMessage::Shutdown => break,
            ControlMessage::Pause => pause_processing(),
            ControlMessage::Resume => resume_processing(),
        }
    }
    data = data_ch.recv() => {
        process_data(data);
    }
    result = result_ch.send(computed_result) => {
        if result.is_err() {
            println("Result channel closed");
            break;
        }
    }
    _ = tick_timer.recv() => {
        perform_periodic_task();
    }
}
```

### Channel Patterns

#### Worker Pool Pattern

```cpp
// Worker pool with competitive consumption
// Each work item processed by exactly ONE worker
fn worker_pool<T, R>(
    work_ch: Channel<T>,     // Shared channel - no cloning needed
    result_ch: Channel<R>,
    worker_count: usize
) where
    T: Send,
    R: Send,
{
    for worker_id in 0..worker_count {
        // All workers share the same channels
        // Each competes to receive work items
        spawn async move {
            loop {
                // Competitive receive - only one worker gets each item
                match co_await work_ch.recv() {
                    Some(task) => {
                        let result = process_task(task);
                        if co_await result_ch.send(result).is_err() {
                            break; // Result channel closed
                        }
                    }
                    None => break, // Work channel closed - mass termination
                }
            }
            println("Worker {} terminating", worker_id);
        };
    }
}

// Poison pill pattern for individual worker termination
union WorkItem<T> {
    Job(T),
    Terminate,  // Poison pill for specific worker
}

fn worker_with_poison<T>(
    work_ch: Channel<WorkItem<T>>
) {
    spawn async move {
        loop {
            match co_await work_ch.recv() {
                Some(WorkItem::Job(job)) => process_job(job),
                Some(WorkItem::Terminate) => {
                    println("Worker received termination signal");
                    break;  // Individual termination
                }
                None => {
                    println("Channel closed, worker terminating");
                    break;  // Mass termination
                }
            }
        }
    };
}
```

#### Fan-out/Fan-in Pattern

```cpp
// Fan-out: distribute work to multiple workers
// Note: This creates separate channels for load distribution
// Different from single shared channel pattern
fn fan_out<T>(input: Channel<T>, workers: Vec<Channel<T>>) {
    spawn async move {
        let mut worker_idx = 0;
        loop {
            match co_await input.recv() {
                Some(item) => {
                    // Round-robin distribution to separate worker channels
                    let worker_ch = &workers[worker_idx % workers.len()];
                    if co_await worker_ch.send(item).is_err() {
                        break; // Worker channel closed
                    }
                    worker_idx += 1;
                }
                None => break, // Input closed
            }
        }
    };
}

// Fan-in: collect results from multiple channels
fn fan_in<T>(worker_channels: Vec<Channel<T>>, output: Channel<T>) {
    spawn async move {
        // Use select to receive from multiple channels
        loop {
            let mut all_closed = true;
            
            // Try to receive from each worker channel
            for worker_ch in &worker_channels {
                match worker_ch.try_recv() {
                    Ok(result) => {
                        if co_await output.send(result).is_err() {
                            return; // Output closed
                        }
                        all_closed = false;
                    }
                    Err(TryRecvError::Empty) => {
                        all_closed = false; // Channel still active
                    }
                    Err(TryRecvError::Closed) => {
                        // This worker channel is closed
                    }
                }
            }
            
            if all_closed {
                break; // All worker channels closed
            }
            
            co_await yield_now(); // Give other coroutines a chance
        }
    };
}

// Alternative: Single shared channel pattern (simpler)
fn shared_channel_workers<T, R>(
    input: Channel<T>,
    output: Channel<R>,
    worker_count: usize
) {
    // All workers share the same input channel
    // Automatic load balancing through competitive consumption
    for _ in 0..worker_count {
        spawn async move {
            loop {
                match co_await input.recv() {
                    Some(work) => {
                        let result = process(work);
                        co_await output.send(result);
                    }
                    None => break,
                }
            }
        };
    }
}
```

## Defer Statement and Resource Management

### Basic Defer Usage

The defer statement ensures cleanup code runs at function exit:

```cpp
fn process_file(path: &str) -> Result<Data> {
    let file = File::open(path)?;
    defer file.close();                    // Always called at function exit
    
    let buffer = allocate_buffer(4096);
    defer free_buffer(buffer);             // Called before file.close()
    
    // Multiple exit points - all execute defers in reverse order
    if file.is_empty() {
        return Err("Empty file");          // Defers execute here
    }
    
    let data = file.read_all()?;
    if !validate(data) {
        return Err("Invalid data");        // Defers execute here
    }
    
    Ok(process(data))                      // Defers execute here
}
```

### Defer with Coroutines

Defer works seamlessly with coroutine suspension and resumption:

```cpp
fn async_process_file(path: &str) -> Result<Data> {
    let file = File::open_async(path).await?;
    defer file.close();  // File closed when coroutine completes
    
    let lock = acquire_lock().await?;
    defer release_lock(lock);  // Lock released when coroutine completes
    
    // Coroutine variables live until function exit
    let data = file.read_async().await?;
    
    // References remain valid across suspension points
    let processed = process_async(&data).await?;
    
    Ok(processed)
    // Defers execute: release_lock(lock), then file.close()
}
```

### Defer Execution Order

Defers execute in LIFO (Last In, First Out) order:

```cpp
fn complex_cleanup() {
    defer println("First defer");     // Executes third
    defer println("Second defer");    // Executes second  
    defer println("Third defer");     // Executes first
    
    // Function body
    
    // Output when function exits:
    // Third defer
    // Second defer
    // First defer
}
```

### Conditional Defer

Defers can be conditional based on runtime state:

```cpp
fn conditional_cleanup(should_log: bool) {
    let resource = acquire_resource();
    defer release_resource(resource);  // Always happens
    
    if should_log {
        let log_file = open_log();
        defer close_log(log_file);     // Only if logging enabled
    }
    
    // ... function body
}
```

## Structured Concurrency

### Parallel Blocks

Parallel blocks provide structured lifetime management for concurrent tasks:

```cpp
// All spawned tasks must complete before block exits
parallel {
    spawn download_file("file1.txt");
    spawn download_file("file2.txt");
    spawn download_file("file3.txt");
} // Blocks until all downloads complete

// Collect results from parallel tasks
let results = parallel {
    let result1 = spawn_with_result(compute_task_1());
    let result2 = spawn_with_result(compute_task_2());
    let result3 = spawn_with_result(compute_task_3());
    
    // Return collected results
    [result1.await, result2.await, result3.await]
};
```

### Cancellation and Timeouts

```cpp
// Timeout for entire parallel block
timeout(5.seconds()) {
    parallel {
        spawn long_running_task_1();
        spawn long_running_task_2();
    }
} // All tasks cancelled if timeout reached

// Cancellation context
let cancel_token = CancellationToken::new();

spawn {
    cancel_token.wait_for_cancellation().await;
    println("Received cancellation signal");
};

// Cancel all associated tasks
cancel_token.cancel();
```

### Task Groups

```cpp
// Task group with error handling
let group = TaskGroup::new();

group.spawn(async || -> Result<()> {
    // Task 1 implementation
    Ok(())
});

group.spawn(async || -> Result<()> {
    // Task 2 implementation
    Ok(())
});

// Wait for all tasks, propagate first error
match group.join_all().await {
    Ok(results) => println("All tasks succeeded"),
    Err(error) => {
        // First error encountered
        println("Task failed: {}", error);
        group.cancel_remaining(); // Cancel other tasks
    }
}
```

## Coroutine Lifecycle Management

### Variable Lifetime in Coroutines

All variables in a coroutine function live until the function completes, not until suspension:

```cpp
fn coroutine_lifetime_demo() {
    let local_data = vec![1, 2, 3, 4, 5];  // Lives entire function
    
    // Reference remains valid across suspension
    let data_ref = &local_data;
    
    some_async_operation().await;  // Coroutine suspends here
    
    // data_ref still valid after resumption
    println("Data: {:?}", data_ref);
    
    another_async_operation().await;  // Another suspension
    
    // Still valid
    process_data(data_ref);
    
    // local_data destroyed when function exits, not at suspension points
}
```

### Coroutine Stack Management

```cpp
// Large stack variables are fine in coroutines
fn large_stack_coroutine() {
    let large_buffer = [0u8; 1_000_000];  // 1MB on coroutine stack
    
    // Coroutine runtime manages stack growth
    recursive_async_function(100).await;
    
    // Buffer remains accessible
    process_buffer(&large_buffer);
}

// Stack growth is handled automatically
fn recursive_async_function(depth: usize) -> impl Future<Output = ()> {
    async move {
        if depth > 0 {
            async_operation().await;
            recursive_async_function(depth - 1).await;
        }
    }
}
```

## M:N Threading Model

### Thread Pool Configuration

```cpp
// Configure the runtime thread pool
fn main() {
    let runtime = Runtime::builder()
        .worker_threads(num_cpus::get())  // One thread per CPU core
        .max_blocking_threads(512)        // For blocking operations
        .thread_name("cprime-worker")
        .build()
        .expect("Failed to create runtime");
        
    runtime.run(async_main());
}

// Blocking operations don't block the entire thread
fn mixed_operations() {
    // CPU-bound task
    spawn cpu_intensive_work();
    
    // Blocking I/O - runs on blocking thread pool
    spawn blocking {
        let data = std::fs::read("large_file.txt").unwrap();
        process_sync(data);
    };
    
    // Async I/O - runs on async thread pool
    spawn async_network_request();
}
```

### Work Stealing

The runtime uses work stealing for load balancing:

```cpp
// Tasks automatically balanced across worker threads
for i in 0..1000 {
    spawn compute_task(i);  // Distributed across available workers
}

// CPU-bound tasks can yield periodically for fairness
fn cpu_intensive_task() {
    for i in 0..1_000_000 {
        // Intensive computation
        complex_calculation(i);
        
        // Yield periodically to allow other tasks to run
        if i % 1000 == 0 {
            yield_now().await;
        }
    }
}
```

## Error Handling in Concurrent Code

### Channel Error Handling

```cpp
// Handle channel errors gracefully
fn robust_channel_handler(ch: Channel<Message>) {
    spawn async move {
        loop {
            match co_await ch.recv() {
                Some(msg) => {
                    if let Err(e) = process_message(msg) {
                        eprintln("Error processing message: {}", e);
                        // Continue processing other messages
                    }
                }
                None => {
                    println("Channel closed, shutting down");
                    break;  // Clean termination on close
                }
            }
        }
    };
}

// Handling send errors (channel might be closed)
fn sender_with_error_handling(ch: Channel<Message>) {
    spawn async move {
        for msg in generate_messages() {
            match co_await ch.send(msg) {
                Ok(()) => {
                    // Message sent successfully
                }
                Err(SendError::Closed(_)) => {
                    println("Channel closed, cannot send");
                    break;  // Stop sending when channel closes
                }
            }
        }
    };
}
```

### Error Propagation in Parallel Tasks

```cpp
// Collect and handle errors from parallel tasks
fn parallel_with_error_handling() -> Result<Vec<Data>, ProcessingError> {
    let results = parallel {
        let task1 = spawn_with_result(fallible_task_1());
        let task2 = spawn_with_result(fallible_task_2());
        let task3 = spawn_with_result(fallible_task_3());
        
        // Collect all results
        vec![task1.await, task2.await, task3.await]
    };
    
    // Check for any failures
    let mut successful_results = Vec::new();
    let mut errors = Vec::new();
    
    for result in results {
        match result {
            Ok(data) => successful_results.push(data),
            Err(e) => errors.push(e),
        }
    }
    
    if errors.is_empty() {
        Ok(successful_results)
    } else {
        Err(ProcessingError::Multiple(errors))
    }
}
```

## Performance Characteristics

### Coroutine Overhead

- **Memory**: ~8KB stack per coroutine (growable)
- **Creation**: Extremely fast allocation from pre-allocated pools
- **Context switch**: Faster than OS threads (no kernel involvement)
- **Scheduling**: Cooperative, no preemption overhead

### Channel Performance

- **Unbuffered**: Zero-copy message passing when possible
- **Buffered**: Ring buffer implementation with atomic operations
- **Select**: Efficient polling using OS-specific mechanisms (epoll/kqueue)

### Scaling Characteristics

```cpp
// Can handle millions of coroutines
fn massive_concurrency() {
    for i in 0..1_000_000 {
        spawn tiny_task(i);  // Each coroutine uses minimal resources
    }
    
    // Runtime efficiently schedules across available CPU cores
}

// Graceful degradation under load
fn load_shedding_example() {
    let (work_tx, work_rx) = channel(1000);  // Bounded channel
    
    // Producer with backpressure
    spawn async move {
        for work_item in generate_work() {
            match work_tx.try_send(work_item) {
                Ok(()) => {},  // Work queued successfully
                Err(TrySendError::Full(_)) => {
                    // Channel full - apply backpressure
                    metrics.increment("work_dropped");
                    // Drop work or wait
                }
                Err(TrySendError::Closed(_)) => break,
            }
        }
    };
}
```

The concurrency model provides simple, efficient concurrent programming that scales from single-threaded applications to high-performance servers handling millions of connections.