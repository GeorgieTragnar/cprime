# CPrime Channels

## Overview

Channels in CPrime are **shared concurrent queues** where each item is consumed by exactly ONE receiver, providing a powerful work distribution mechanism. Unlike traditional message-passing systems that might broadcast to all listeners, CPrime channels implement **competitive consumption** - multiple coroutines can wait on the same channel, but each message is delivered to only one receiver.

This design follows a three-layer architecture:
1. **Language Layer**: Provides keywords and semantics (`co_await`, `select`, close semantics)
2. **Library Layer**: Implements storage strategies and optimization
3. **Scheduler Layer**: Manages lifecycle, wake decisions, and memory safety

## Core Concepts

### Channels vs Broadcast

CPrime makes a fundamental distinction between two communication patterns:

```cpp
// Channel: Competitive consumption (work distribution)
// Each item consumed by exactly ONE receiver
send(work_item) → Receiver A OR B OR C gets it

// Broadcast: All observers pattern (event notification)  
// All subscribers see each event
emit(event) → Receiver A AND B AND C get it
```

These are fundamentally different mechanisms:
- **Channels**: Destructive read (pop from queue)
- **Broadcast**: Non-destructive read (observers peek/clone)
- **Same underlying mechanism**: Different ownership semantics

### Channel as Shared Queue

```cpp
// Multiple producers, multiple consumers, competitive consumption
let work_queue = channel<WorkItem>(100);

// Spawn multiple workers - all compete for work items
for i in 0..4 {
    spawn worker(work_queue);  // Same channel, competitive receive
}

// Multiple producers can send
spawn producer(work_queue);
spawn producer(work_queue);

// Each work item processed by exactly ONE worker
// Automatic load balancing through competition
```

## Three-Layer Architecture

### Language Layer (Minimal)

The language provides only the essential primitives:

```cpp
// Suspension primitive for channel operations
co_await channel.send(value);
co_await channel.recv();  // Returns Option<T>

// Select statement for multi-channel operations
select {
    x = ch1.recv() => { /* handle x */ },
    y = ch2.recv() => { /* handle y */ },
}

// Close semantics
channel.close();  // Language-level operation

// Result types
enum RecvResult<T> {
    Item(T),      // Received item
    Closed,       // Channel closed
}

enum SendResult {
    Ok,           // Sent successfully
    Closed,       // Channel closed, cannot send
}
```

### Library Layer (Implementation)

Libraries provide different storage strategies and optimizations:

```cpp
// Different storage strategies for different use cases
class WorkQueue<T> {           // Simple concurrent queue
    buffer: CircularBuffer<T>,
    capacity: usize,
    head: AtomicUsize,
    tail: AtomicUsize,
    closed: AtomicBool,
    constructed_by: WorkQueueOps,
}

class EventLog<T> {             // Indexed map for event replay
    events: BTreeMap<EventId, T>,
    next_id: AtomicU64,
    subscriber_positions: HashMap<SubId, EventId>,
    constructed_by: EventLogOps,
}

class RingBuffer<T> {           // Fixed-size circular buffer
    buffer: [T; N],
    write_pos: AtomicUsize,
    read_pos: AtomicUsize,
    constructed_by: RingBufferOps,
}

// Library chooses optimal strategy based on usage pattern
functional class ChannelFactory {
    fn create<T>(capacity: usize) -> Channel<T> {
        if capacity == 0 {
            UnbufferedChannel::new()  // Synchronous handoff
        } else if capacity < 64 {
            SmallChannel::new(capacity)  // Optimized for small buffers
        } else {
            LargeChannel::new(capacity)  // General purpose
        }
    }
}
```

### Scheduler Layer (Lifecycle)

The scheduler manages channel lifecycle and wake decisions:

```cpp
// Scheduler holds references to prevent use-after-free
class Scheduler {
    // Channels with waiting coroutines
    active_channels: HashMap<ChannelId, SharedPtr<ChannelData>>,
    // Suspended coroutines waiting on channels
    channel_waiters: HashMap<ChannelId, Vec<CoroId>>,
    
    constructed_by: SchedulerOps,
}

functional class SchedulerOps {
    fn suspend_on_channel(
        scheduler: &mut Scheduler,
        coro_id: CoroId,
        channel: &Channel
    ) {
        // Add reference to prevent channel deallocation
        let channel_ref = SharedPtr::clone(&channel.data);
        scheduler.active_channels.insert(channel.id, channel_ref);
        
        // Track waiting coroutine
        scheduler.channel_waiters
            .entry(channel.id)
            .or_default()
            .push(coro_id);
    }
    
    fn handle_channel_close(
        scheduler: &mut Scheduler,
        channel_id: ChannelId
    ) {
        // Wake all waiters when channel closes
        if let Some(waiters) = scheduler.channel_waiters.remove(&channel_id) {
            for coro_id in waiters {
                SchedulerOps::wake_coroutine(scheduler, coro_id);
            }
        }
        
        // Remove reference, allowing deallocation if no other refs
        scheduler.active_channels.remove(&channel_id);
    }
}
```

## Channel Implementation with Three-Class System

### Channel as Data Class

```cpp
// Channel state as data class
class Channel<T> {
    // Internal state
    buffer: CircularBuffer<T>,
    capacity: usize,
    closed: AtomicBool,
    
    // Synchronization
    send_waiters: WaitQueue,
    recv_waiters: WaitQueue,
    
    // Reference counting for safety
    ref_count: AtomicUsize,
    
    // Construction control
    constructed_by: ChannelOps<T>,
    
    // Access rights for capabilities
    exposes SendOps<T> { buffer, closed, send_waiters }
    exposes RecvOps<T> { buffer, closed, recv_waiters }
}
```

### Channel Operations as Functional Class

```cpp
functional class ChannelOps<T> {
    fn construct(capacity: usize) -> Channel<T> {
        Channel {
            buffer: CircularBuffer::new(capacity),
            capacity,
            closed: AtomicBool::new(false),
            send_waiters: WaitQueue::new(),
            recv_waiters: WaitQueue::new(),
            ref_count: AtomicUsize::new(1),
        }
    }
    
    fn destruct(channel: &mut Channel<T>) {
        // Mark as closed
        channel.closed.store(true, Ordering::Release);
        
        // Wake all waiters
        SchedulerOps::wake_all(&channel.send_waiters);
        SchedulerOps::wake_all(&channel.recv_waiters);
        
        // Actual deallocation happens when ref_count reaches 0
    }
}

functional class SendOps<T> {
    suspend fn send(channel: &Channel<T>, value: T) -> SendResult {
        if channel.closed.load(Ordering::Acquire) {
            return SendResult::Closed;
        }
        
        // Try to send immediately if space available
        if channel.buffer.try_push(value) {
            // Wake one receiver if any waiting
            if let Some(receiver) = channel.recv_waiters.pop() {
                SchedulerOps::wake_coroutine(receiver);
            }
            return SendResult::Ok;
        }
        
        // Buffer full, suspend until space available
        let current_coro = SchedulerOps::current_coroutine();
        channel.send_waiters.push(current_coro);
        
        // Suspend - scheduler holds channel reference
        co_await SchedulerOps::suspend();
        
        // Resumed - either space available or channel closed
        if channel.closed.load(Ordering::Acquire) {
            SendResult::Closed
        } else {
            channel.buffer.push(value);
            SendResult::Ok
        }
    }
}

functional class RecvOps<T> {
    suspend fn recv(channel: &Channel<T>) -> Option<T> {
        // Try to receive immediately if items available
        if let Some(value) = channel.buffer.try_pop() {
            // Wake one sender if any waiting
            if let Some(sender) = channel.send_waiters.pop() {
                SchedulerOps::wake_coroutine(sender);
            }
            return Some(value);
        }
        
        // Check if closed and empty
        if channel.closed.load(Ordering::Acquire) {
            return None;  // Channel closed, no more items
        }
        
        // Buffer empty, suspend until item available
        let current_coro = SchedulerOps::current_coroutine();
        channel.recv_waiters.push(current_coro);
        
        // Suspend - scheduler holds channel reference
        co_await SchedulerOps::suspend();
        
        // Resumed - either item available or channel closed
        channel.buffer.try_pop()  // Returns None if closed
    }
}
```

## Access Rights as Subscription

Access rights provide automatic subscription management without explicit subscribe/unsubscribe:

```cpp
// Message bus with channel inside
class MessageBus {
    channel: Channel<Message>,
    
    // Access rights define capabilities
    exposes SendOps { channel }
    exposes RecvOps { channel }
    
    constructed_by: MessageBusOps,
}

functional class MessageBusOps {
    fn construct(capacity: usize) -> MessageBus {
        MessageBus {
            channel: ChannelOps::construct(capacity),
        }
    }
    
    fn create_producer(bus: &MessageBus) -> Producer {
        // Producer gets send capability through access rights
        Producer {
            bus_ref: bus with SendOps,  // Only send access
        }
    }
    
    fn create_consumer(bus: &MessageBus) -> Consumer {
        // Consumer gets receive capability through access rights
        Consumer {
            bus_ref: bus with RecvOps,  // Only receive access
        }
    }
}

// Usage - subscription through access rights
let bus = MessageBusOps::construct(100);

// Create producers with send-only access
spawn producer(MessageBusOps::create_producer(&bus));
spawn producer(MessageBusOps::create_producer(&bus));

// Create consumers with receive-only access
spawn consumer(MessageBusOps::create_consumer(&bus));
spawn consumer(MessageBusOps::create_consumer(&bus));

// Access rights ARE the subscription - no explicit subscribe needed
// When coroutine terminates, access right is dropped automatically
```

## Select Statement

The select statement enables waiting on multiple channels with type-safe branching:

```cpp
// Select generates implicit union of outcomes
select {
    // Each branch has block-scoped type binding
    msg: Message = work_ch.recv() => {
        // msg only exists in this block as Message type
        process_message(msg);
    },
    
    cmd: Command = control_ch.recv() => {
        // cmd only exists in this block as Command type
        match cmd {
            Command::Stop => return,
            Command::Pause => pause_processing(),
        }
    },
    
    // Send operations can also be selected
    result = output_ch.send(data) => {
        if result == SendResult::Closed {
            println("Output channel closed");
            return;
        }
    },
    
    // Timeout branch
    _ = timer::after(5.seconds()) => {
        println("Operation timed out");
    },
    
    // Default for non-blocking select
    default => {
        // No channels ready, do other work
        do_background_work();
    }
}
```

### Select Implementation with Unions

The compiler transforms select into a union-based state machine:

```cpp
// Compiler generates this union for the select
union SelectResult {
    WorkMessage(Message),
    ControlCommand(Command),
    SendComplete(SendResult),
    Timeout,
    Default,
}

// Select execution becomes:
let result: SelectResult = runtime_select(&[
    SelectCase::Recv(&work_ch),
    SelectCase::Recv(&control_ch),
    SelectCase::Send(&output_ch, data),
    SelectCase::Timeout(5.seconds()),
    SelectCase::Default,
]);

match result {
    SelectResult::WorkMessage(msg) => process_message(msg),
    SelectResult::ControlCommand(cmd) => handle_command(cmd),
    SelectResult::SendComplete(result) => check_send_result(result),
    SelectResult::Timeout => handle_timeout(),
    SelectResult::Default => do_background_work(),
}
```

## Channel Close Design

### The Problem

Channels need a way to signal completion to waiting coroutines without causing memory safety issues:

```cpp
// The memory safety challenge:
async fn receiver(ch: Channel<Work>) {
    co_await ch.recv();  // Coroutine suspended here
}

// Meanwhile in another coroutine:
ch.close();
drop(ch);  // DANGER: Memory freed while coroutine has reference!

// When receiver wakes: CRASH - use after free!
```

### The Solution: Reference Counting

Channels use shared pointers with atomic reference counting:

```cpp
// Channel internally uses shared pointer
class Channel<T> {
    data: SharedPtr<ChannelData<T>>,  // Reference counted
}

// Scheduler holds references during suspension
class Scheduler {
    // Prevents deallocation while coroutines wait
    channel_refs: HashMap<CoroId, SharedPtr<ChannelData>>,
}

// Safe close sequence:
// 1. close() marks channel state as closed
// 2. Scheduler wakes all waiting coroutines
// 3. Waiting coroutines see closed state and return None
// 4. When last reference dropped, channel deallocated
```

### Close Semantics

```cpp
// Close is a language-level operation
fn close_semantics() {
    let ch = channel<Work>(10);
    
    // Spawn receivers
    for i in 0..4 {
        spawn async move {
            loop {
                match co_await ch.recv() {
                    Some(work) => process(work),
                    None => break,  // Clean termination on close
                }
            }
        };
    }
    
    // Send some work
    co_await ch.send(Work::new());
    co_await ch.send(Work::new());
    
    // Close channel - all receivers will terminate
    ch.close();
    // 1. Channel marked as closed
    // 2. All waiting receivers wake and get None
    // 3. Future recv() calls return None immediately
    // 4. send() returns Err(Closed)
}
```

## Worker Patterns

### Persistent Worker Problem

Workers often run in infinite loops, requiring careful termination strategies:

```cpp
// Problem: How to terminate specific workers vs all workers?
async fn worker_loop(ch: Channel<Work>) {
    loop {
        match co_await ch.recv() {
            Some(work) => process(work),  // Normal work
            None => break,  // Channel closed - terminate
        }
    }
}
```

### Dual Termination Solution

Use poison pills for individual termination and channel close for mass termination:

```cpp
// Work item that can signal termination
union WorkItem<T> {
    Job(T),        // Normal work
    Terminate,     // Poison pill for specific worker
}

async fn smart_worker(
    id: WorkerId,
    ch: Channel<WorkItem<Work>>
) {
    loop {
        match co_await ch.recv() {
            Some(WorkItem::Job(work)) => {
                println("Worker {} processing job", id);
                process(work);
            },
            Some(WorkItem::Terminate) => {
                println("Worker {} received termination signal", id);
                break;  // Individual termination
            },
            None => {
                println("Channel closed, worker {} terminating", id);
                break;  // Mass termination
            }
        }
    }
    
    // Cleanup code here
    println("Worker {} cleaned up and terminated", id);
}

// Controller can terminate individually or all at once
functional class WorkerController {
    fn terminate_worker(ch: &Channel<WorkItem<Work>>, worker_id: WorkerId) {
        // Send poison pill to terminate specific worker
        co_await ch.send(WorkItem::Terminate);
    }
    
    fn terminate_all(ch: &Channel<WorkItem<Work>>) {
        // Close channel to terminate all workers
        ch.close();
    }
}
```

## Library Implementation Strategies

Different libraries can provide different channel implementations while maintaining the same language-level semantics:

### Work Queue Implementation

Optimized for FIFO work distribution:

```cpp
class WorkQueueChannel<T> {
    buffer: ConcurrentQueue<T>,
    capacity: usize,
    
    constructed_by: WorkQueueOps<T>,
}

functional class WorkQueueOps<T> {
    fn send(ch: &WorkQueueChannel<T>, value: T) -> SendResult {
        // Simple FIFO enqueue
        if ch.buffer.size() < ch.capacity {
            ch.buffer.push(value);
            wake_one_receiver(ch);
            SendResult::Ok
        } else {
            suspend_until_space(ch);
            // ... continuation after suspension
        }
    }
    
    fn recv(ch: &WorkQueueChannel<T>) -> Option<T> {
        // Simple FIFO dequeue
        if let Some(value) = ch.buffer.pop() {
            wake_one_sender(ch);
            Some(value)
        } else if ch.is_closed() {
            None
        } else {
            suspend_until_item(ch);
            // ... continuation after suspension
        }
    }
}
```

### Event Log Implementation

Optimized for event sourcing with replay:

```cpp
class EventLogChannel<T> {
    events: BTreeMap<EventId, T>,
    next_id: AtomicU64,
    subscriber_positions: HashMap<SubId, EventId>,
    
    constructed_by: EventLogOps<T>,
}

functional class EventLogOps<T> {
    fn send(ch: &EventLogChannel<T>, event: T) -> SendResult {
        // Append to log with monotonic ID
        let event_id = ch.next_id.fetch_add(1);
        ch.events.insert(event_id, event);
        
        // Wake all subscribers to check for new events
        wake_all_subscribers(ch);
        SendResult::Ok
    }
    
    fn recv_from(ch: &EventLogChannel<T>, subscriber: SubId) -> Option<T> {
        // Get next event for this subscriber
        let current_pos = ch.subscriber_positions.get(&subscriber);
        
        if let Some(next_event) = ch.events.range(current_pos..).next() {
            ch.subscriber_positions.insert(subscriber, next_event.id + 1);
            Some(next_event.value.clone())  // Non-destructive read
        } else {
            suspend_until_new_event(ch, subscriber);
            // ... continuation after suspension
        }
    }
}
```

### Scheduler Strategy Examples

Different schedulers can implement different wake strategies:

```cpp
// Eager scheduler - wake immediately on close
functional class EagerScheduler {
    fn handle_channel_close(channel_id: ChannelId) {
        // Wake all waiters immediately
        for waiter in get_channel_waiters(channel_id) {
            wake_coroutine_immediately(waiter);
        }
    }
}

// Batch scheduler - periodic wake sweeps
functional class BatchScheduler {
    fn handle_channel_close(channel_id: ChannelId) {
        // Mark for wake in next batch
        mark_for_batch_wake(get_channel_waiters(channel_id));
        // Actual wake happens in periodic sweep
    }
    
    fn periodic_sweep() {
        // Every 10ms, wake all marked coroutines
        for coro in marked_for_wake() {
            wake_coroutine(coro);
        }
    }
}

// Adaptive scheduler - load-based decisions
functional class AdaptiveScheduler {
    fn handle_channel_close(channel_id: ChannelId) {
        let waiter_count = count_channel_waiters(channel_id);
        
        if waiter_count < 10 {
            // Few waiters - wake immediately
            wake_all_immediately(channel_id);
        } else if waiter_count < 100 {
            // Moderate - staggered wake
            wake_staggered(channel_id, 10ms);
        } else {
            // Many waiters - batch to avoid thundering herd
            batch_wake(channel_id);
        }
    }
}
```

## Performance Characteristics

### Memory Overhead

- **Channel structure**: ~64-128 bytes base overhead
- **Per-item overhead**: 0 (items stored directly)
- **Reference counting**: 8 bytes per reference
- **Wait queues**: 16 bytes per waiting coroutine

### Operation Costs

| Operation | Unbuffered | Buffered (space) | Buffered (full) |
|-----------|------------|------------------|-----------------|
| Send | O(1) + context switch | O(1) | O(1) + suspend |
| Receive | O(1) + context switch | O(1) | O(1) + suspend |
| Close | O(n) wake calls | O(n) wake calls | O(n) wake calls |
| Select | O(m) channel checks | O(m) channel checks | O(m) + suspend |

Where n = waiting coroutines, m = channels in select

### Optimization Opportunities

1. **Lock-free algorithms** for high-throughput scenarios
2. **NUMA-aware channel placement** for multi-socket systems
3. **Specialized channels** for specific patterns (SPSC, MPSC, etc.)
4. **Batch operations** for multiple sends/receives
5. **Zero-copy channels** for large messages

## Best Practices

### Channel Sizing

```cpp
// Unbuffered - strict synchronization
let sync_ch = channel<Message>();  // Sender waits for receiver

// Small buffer - absorb bursts
let burst_ch = channel<Message>(10);  // Handle temporary speed mismatches

// Large buffer - decouple producers/consumers
let async_ch = channel<Message>(1000);  // Significant decoupling

// Bounded vs unbounded tradeoff
// Bounded: Backpressure, prevent memory growth
// Unbounded: No blocking, risk of memory exhaustion
```

### Error Handling

```cpp
// Always handle close gracefully
async fn robust_receiver(ch: Channel<Work>) {
    loop {
        match co_await ch.recv() {
            Some(work) => {
                if let Err(e) = process(work) {
                    log_error(e);  // Log but continue
                }
            },
            None => {
                log_info("Channel closed, terminating");
                break;
            }
        }
    }
    cleanup();  // Always cleanup on termination
}
```

### Avoiding Deadlocks

```cpp
// DANGER: Potential deadlock with unbuffered channels
async fn deadlock_example() {
    let ch1 = channel<i32>();
    let ch2 = channel<i32>();
    
    // Both try to send first - DEADLOCK!
    spawn async { co_await ch1.send(1); co_await ch2.recv(); };
    spawn async { co_await ch2.send(2); co_await ch1.recv(); };
}

// SOLUTION: Use select or buffered channels
async fn fixed_example() {
    let ch1 = channel<i32>(1);  // Buffered
    let ch2 = channel<i32>(1);  // Buffered
    
    spawn async { co_await ch1.send(1); co_await ch2.recv(); };
    spawn async { co_await ch2.send(2); co_await ch1.recv(); };
}
```

### Channel Lifecycle Management

```cpp
// Ensure channels are closed when done
functional class ChannelManager {
    fn with_channel<T, R>(
        capacity: usize,
        body: fn(Channel<T>) -> R
    ) -> R {
        let ch = channel<T>(capacity);
        defer ch.close();  // Always close on exit
        body(ch)
    }
}

// Usage
ChannelManager::with_channel(100, |ch| {
    spawn_workers(ch);
    send_work(ch);
    wait_for_completion();
    // Channel automatically closed here
});
```

## Integration with Coroutine System

Channels integrate seamlessly with CPrime's coroutine system:

```cpp
// Coroutines can suspend on channel operations
async fn channel_coroutine(ch: Channel<Message>) {
    // Coroutine suspends if channel empty
    let msg = co_await ch.recv();
    
    // Process message
    let response = process(msg);
    
    // Coroutine suspends if response channel full
    co_await response_ch.send(response);
}

// Compiler analyzes channel operations for size classification
// Channel operations add to stack usage calculation:
// - Channel reference: 8 bytes
// - Message storage: sizeof(Message)
// - Suspension state: ~32 bytes
```

## Future Directions

### Planned Enhancements

1. **Dynamic channel creation** within access-rights scope
2. **Channel composition** and chaining patterns
3. **Priority channels** with message ordering
4. **Network-transparent channels** for distributed systems
5. **Persistent channels** with durability guarantees

### Research Areas

1. **Lock-free channel algorithms** for extreme performance
2. **NUMA-optimized channels** for large systems
3. **Hardware-accelerated channels** using RDMA/DMA
4. **Formal verification** of channel safety properties
5. **Automatic channel sizing** based on flow analysis

CPrime channels provide a powerful, safe, and efficient communication mechanism that seamlessly integrates with the language's coroutine system while maintaining the flexibility for library-level optimization and innovation.