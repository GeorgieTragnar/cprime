# CPrime Execution Runtime System

## Overview

CPrime's **execution runtime system** provides the execution context for programs, determining how signals are handled, how concurrency is managed, and how system resources are utilized. The language separates **signal handling primitives** (syntax and semantics) from **execution runtime behavior** (execution policies), allowing the same code to run with different performance and safety characteristics.

> **Important**: This document discusses the **execution runtime system** (program execution environment), not the **`runtime` keyword** which signals performance costs in language constructs. For the `runtime` keyword, see [Runtime/Comptime Keywords](runtime-comptime-keywords.md).

The execution runtime is selected at program entry using the `fn main() = RuntimeChoice` syntax, and each runtime implements a common interface while providing different capabilities and trade-offs.

For signal handling language primitives and syntax, see [Signal Handling](signal-handling.md).

## Runtime Trait Definition

### Core Runtime Interface

All runtimes implement the `Runtime` trait, which defines the essential lifecycle and capabilities:

```cpp
trait Runtime {
    // Initialization phase
    fn init();
    
    // Signal handling strategy
    fn setup_signals();
    
    // Runtime capabilities (compile-time constants)
    const HAS_COROUTINES: bool = false;
    const HAS_ASYNC: bool = false;
    const HAS_SIGNAL_THREAD: bool = false;
    const HAS_WORK_STEALING: bool = false;
    const HAS_NUMA_AWARENESS: bool = false;
    
    // Resource management
    fn allocate_stack(size: usize) -> *mut u8;
    fn deallocate_stack(ptr: *mut u8, size: usize);
    
    // Cleanup phase
    fn cleanup();
}
```

### Entry Point Declaration

Programs declare their runtime choice at the entry point:

```cpp
// Default single-threaded runtime
fn main() = default {
    // Simple program logic
    println!("Hello, World!");
}

// Coroutine scheduler runtime
fn main() = std::CoroutineScheduler {
    // Async program logic
    spawn async_tasks().await;
}

// Custom runtime
fn main() = CustomGameEngine {
    // Game-specific runtime behavior
    game_loop();
}
```

## Default Runtime (Single-Threaded)

### Runtime Declaration and Characteristics

```cpp
fn main() = default {
    // Single-threaded program
    // No automatic signal handling
    // No coroutines
    // Minimal overhead
}

impl Runtime for default {
    fn init() {
        // Minimal initialization
        // No thread creation
        // No signal threads
        // No work queues
    }
    
    fn setup_signals() {
        // MINIMAL signal setup
        signal(SIGPIPE, SIG_IGN);  // Only ignore SIGPIPE
        
        // Everything else uses OS defaults:
        // SIGINT → terminate (default OS behavior)
        // SIGTERM → terminate (default OS behavior)  
        // SIGSEGV → terminate (default OS behavior)
        
        // NO automatic handlers
        // NO signal polling
        // NO signal thread
    }
    
    const HAS_COROUTINES: bool = false;
    const HAS_ASYNC: bool = false;
    const HAS_SIGNAL_THREAD: bool = false;
    
    fn allocate_stack(size: usize) -> *mut u8 {
        // Simple malloc - no pooling
        unsafe { malloc(size) }
    }
    
    fn cleanup() {
        // Minimal cleanup
        // OS handles process termination
    }
}
```

### Default Runtime Signal Flow

#### Internal Signals (Exceptions)

The default runtime handles internal signals through function stack unwinding:

```
Application raises signal
    ↓
Current function has except(SIGNAL)?
    ↓ NO → terminate()
    ↓ YES
Currently in catch(SIGNAL) block?
    ↓ NO → Propagate to caller
    ↓ YES → Jump to recover block
    ↓
Continue execution after catch/recover
```

Example:

```cpp
fn main() = default {
    catch(FILE_ERROR) {
        let data = read_config_file()?;  // May raise FILE_ERROR
        process_config(data);
    } recover {
        use_default_config();
    }
    
    // Program continues normally
}

fn read_config_file() except(FILE_ERROR) -> ConfigData {
    catch(IO_ERROR) {
        let file = open_file("config.json")?;  // May raise IO_ERROR
        parse_json(file)?
    } recover {
        // Transform IO_ERROR to FILE_ERROR
        raise(FILE_ERROR { 
            message: "Config file inaccessible".to_string(),
            cause: current_signal() 
        });
    }
}
```

#### External Signals (OS Signals)

OS signals in the default runtime follow direct OS behavior unless explicitly overridden:

```
OS signal arrives (e.g., SIGINT)
    ↓
Program has installed custom handler?
    ↓ NO → OS default behavior (usually terminate)
    ↓ YES → Execute custom handler
    ↓
Handler completes → Resume or terminate
```

### Manual Signal Handler Installation

The default runtime supports explicit signal handler registration:

```cpp
fn main() = default {
    // Option 1: Quick termination handler
    signal(SIGINT, |sig| {
        write(2, "Interrupted\n", 12);  // Async-safe only
        dump_metrics();
        exit(sig);
    });
    
    // Option 2: Flag-based handling
    static mut INTERRUPTED: sig_atomic_t = 0;
    signal(SIGINT, |sig| {
        unsafe { INTERRUPTED = 1; }
    });
    
    // Main work loop with manual checking
    while unsafe { INTERRUPTED } == 0 {
        do_work();
        // Check flag periodically
    }
    
    // Option 3: Interactive debug handler (infinite loop allowed!)
    signal(SIGUSR1, |sig| {
        loop {
            let cmd = read_debug_command();  // Async-safe only
            match cmd {
                "dump" => dump_program_state(),
                "continue" => return,  // Resume normal execution
                "exit" => exit(0),
            }
        }
    });
    
    // Program continues
    work_forever();
}
```

### Default Runtime Examples

#### Simple Command-Line Tool

```cpp
fn main() = default {
    let args = parse_command_line();
    
    catch(ARGUMENT_ERROR | FILE_ERROR) {
        let input = read_input_file(&args.filename)?;
        let processed = process_data(input)?;
        write_output_file(&args.output, processed)?;
    } recover {
        eprintln!("Error: {}", current_signal().message());
        exit(1);
    }
    
    println!("Processing complete");
}
```

#### System Service with Manual Signal Handling

```cpp
fn main() = default {
    // Install signal handlers for graceful shutdown
    static mut SHUTDOWN_REQUESTED: sig_atomic_t = 0;
    static mut RELOAD_CONFIG: sig_atomic_t = 0;
    
    signal(SIGTERM, |_| unsafe { SHUTDOWN_REQUESTED = 1; });
    signal(SIGINT, |_| unsafe { SHUTDOWN_REQUESTED = 1; });
    signal(SIGHUP, |_| unsafe { RELOAD_CONFIG = 1; });
    
    let mut service = initialize_service();
    
    while unsafe { SHUTDOWN_REQUESTED } == 0 {
        // Check for config reload
        if unsafe { RELOAD_CONFIG } == 1 {
            service.reload_configuration();
            unsafe { RELOAD_CONFIG = 0; }
        }
        
        // Process requests
        catch(SERVICE_ERROR) {
            service.handle_requests()?;
        } recover {
            log_service_error(current_signal());
        }
        
        thread::sleep(Duration::from_millis(100));
    }
    
    service.graceful_shutdown();
}
```

## Coroutine Scheduler Runtime

### Runtime Declaration and Architecture

```cpp
fn main() = std::CoroutineScheduler {
    // Multi-threaded async runtime
    // Dedicated signal thread
    // Work-stealing scheduler
    // Channel-based communication
}

impl Runtime for std::CoroutineScheduler {
    fn init() {
        spawn_signal_thread();
        init_work_stealing_scheduler();
        setup_coroutine_pools();
        init_channel_system();
    }
    
    fn setup_signals() {
        // Block ALL signals in worker threads
        block_all_signals_in_workers();
        
        // Dedicated signal thread handles all OS signals
        spawn signal_handler_thread();
    }
    
    const HAS_COROUTINES: bool = true;
    const HAS_ASYNC: bool = true;
    const HAS_SIGNAL_THREAD: bool = true;
    const HAS_WORK_STEALING: bool = true;
    
    fn allocate_stack(size: usize) -> *mut u8 {
        // Pool-based allocation with size classes
        coroutine_pool_allocate(classify_size(size))
    }
}
```

### Dedicated Signal Thread Architecture

The coroutine scheduler's breakthrough innovation is converting OS signals into scheduled coroutines:

```cpp
// Signal thread implementation (pseudo-code)
fn signal_handler_thread() {
    // This thread receives ALL OS signals
    unblock_all_signals();
    
    loop {
        // Synchronous signal waiting - no race conditions
        let sig = sigwait(&all_signals);
        
        // Look up registered signal handlers
        if let Some(handler_fn) = scheduler.get_signal_handler(sig) {
            // CREATE NEW COROUTINE for signal handling
            let signal_coro = SignalCoroutine::new(sig, handler_fn);
            
            // SCHEDULE the signal coroutine like any other work
            scheduler.work_queue.push_priority(signal_coro, SIGNAL_PRIORITY);
            
            // Signal handling becomes async operation!
        } else {
            // No handler registered
            match sig {
                SIGTERM | SIGINT => graceful_shutdown(),
                _ => terminate(),
            }
        }
    }
}

// Signal-to-coroutine conversion
class SignalCoroutine {
    stack_memory: *mut u8,
    signal_data: SignalData,
    handler_fn: SignalHandlerFn,
    created_at: SystemTime,
    
    constructed_by: SignalCoroOps,
}

functional class SignalCoroOps {
    fn construct(signal: Signal, handler: SignalHandlerFn) -> SignalCoroutine {
        let stack = allocate_coroutine_stack(SIGNAL_CORO_SIZE);
        
        SignalCoroutine {
            stack_memory: stack.ptr,
            signal_data: SignalData::from(signal),
            handler_fn: handler,
            created_at: SystemTime::now(),
        }
    }
    
    async fn execute(coro: &mut SignalCoroutine) -> Result<()> {
        // Signal handler runs as async coroutine
        (coro.handler_fn)(coro.signal_data).await
    }
}
```

### Signal Handler Registration in Scheduler

```cpp
// Register async signal handlers with scheduler
let scheduler = CoroutineScheduler::new()
    .handle(SIGINT, |signal_data| async {
        println!("Graceful shutdown requested");
        initiate_shutdown().await;
    })
    .handle(SIGUSR1, |signal_data| async {
        let stats = collect_runtime_stats().await;
        print_performance_report(stats);
    })
    .handle(OOM, |oom_data| async {
        // Handle out-of-memory as async operation
        free_caches().await;
        if !try_allocate_emergency_memory() {
            emergency_shutdown().await;
        }
    })
    .handle(NETWORK_ERROR, |net_err| async {
        // Network errors become async recovery operations
        let backup_endpoint = find_backup_endpoint().await;
        retry_with_endpoint(backup_endpoint, net_err.original_request).await;
    });
```

### Thread Signal to Coroutine Conversion

When worker threads encounter unhandled signals, they convert them to coroutines before terminating:

```cpp
// Worker thread signal handling
fn worker_thread_signal_handler(signal: Signal, thread_id: ThreadId) {
    // 1. Worker thread receives signal (shouldn't happen normally)
    println!("Worker thread {} received signal: {}", thread_id, signal);
    
    // 2. Thread fully unwinds - run all defers
    run_all_thread_defers();
    
    // 3. Check if scheduler has handler for this signal
    if let Some(handler) = scheduler.get_signal_handler(signal) {
        // 4. DYING THREAD creates coroutine for signal handling
        let signal_coro = SignalCoroutine::new(signal, handler);
        
        // 5. DYING THREAD pushes to scheduler work queue
        scheduler.work_queue.push_high_priority(signal_coro);
        
        // 6. Thread exits cleanly
        println!("Thread {} converted signal to coroutine, exiting", thread_id);
        thread::exit(0);
    } else {
        // No handler - terminate entire program
        eprintln!("Unhandled signal {} in worker thread", signal);
        terminate_program();
    }
}
```

### Scheduler Signal Safety Architecture

The scheduler maintains signal safety through reference management:

```cpp
class CoroutineScheduler {
    // Signal-related coroutines (prevent premature cleanup)
    signal_coroutines: HashMap<CoroId, SignalCoroutine>,
    
    // Active signal handlers
    signal_handlers: HashMap<Signal, AsyncSignalHandler>,
    
    // Signal thread handle
    signal_thread: Option<ThreadHandle>,
    
    // Work queues with priority
    work_queue: PriorityQueue<AnyCoroutine>,
    
    constructed_by: SchedulerOps,
}

functional class SchedulerOps {
    fn register_signal_handler(
        scheduler: &mut CoroutineScheduler,
        signal: Signal,
        handler: AsyncSignalHandler
    ) {
        scheduler.signal_handlers.insert(signal, handler);
        
        // Inform signal thread about new handler
        signal_thread_control_channel.send(
            SignalThreadCommand::RegisterHandler(signal)
        );
    }
    
    fn handle_signal_coroutine_completion(
        scheduler: &mut CoroutineScheduler,
        coro_id: CoroId,
        result: SignalHandlingResult
    ) {
        // Clean up completed signal coroutine
        if let Some(signal_coro) = scheduler.signal_coroutines.remove(&coro_id) {
            deallocate_coroutine_stack(signal_coro.stack_memory);
        }
        
        // Log signal handling completion
        log_signal_handling(coro_id, result);
    }
}
```

### Coroutine Scheduler Examples

#### Web Server with Async Signal Handling

```cpp
fn main() = std::CoroutineScheduler {
    let scheduler = CoroutineScheduler::new()
        .handle(SIGINT, |_| async {
            println!("Graceful shutdown initiated");
            drain_connection_pool().await;
            close_database_connections().await;
            save_metrics().await;
        })
        .handle(SIGUSR1, |_| async {
            let stats = collect_server_stats().await;
            rotate_log_files().await;
            print_stats_report(stats);
        });
    
    // Start HTTP server with async request handling
    let server = HttpServer::bind("0.0.0.0:8080").await?;
    
    loop {
        let (stream, addr) = server.accept().await?;
        
        // Spawn coroutine for each connection
        spawn async {
            handle_connection(stream, addr).await;
        };
    }
}

async fn handle_connection(stream: TcpStream, addr: SocketAddr) {
    catch(NETWORK_ERROR | TIMEOUT_ERROR) {
        let request = read_request(&stream).await?;
        let response = process_request(request).await?;
        write_response(&stream, response).await?;
    } recover {
        // Signal handling is async here
        let error_response = create_error_response(current_signal()).await;
        let _ = write_response(&stream, error_response).await;
    }
}
```

#### Distributed System with Signal-Driven Coordination

```cpp
fn main() = std::CoroutineScheduler {
    let scheduler = CoroutineScheduler::new()
        .handle(CLUSTER_NODE_FAILED, |node_failure| async {
            // Handle node failures as async operations
            let failed_node = node_failure.node_id;
            redistribute_workload(failed_node).await;
            update_cluster_topology().await;
            notify_monitoring_system(node_failure).await;
        })
        .handle(LOAD_BALANCER_SIGNAL, |lb_signal| async {
            // Adjust load balancing asynchronously
            let new_weights = calculate_new_weights().await;
            update_load_balancer(new_weights).await;
        });
    
    // Start distributed system components
    spawn cluster_heartbeat_monitor();
    spawn workload_distributor();
    spawn health_checker();
    
    // Main coordination loop
    loop {
        select {
            coordinator_msg = coordinator_channel.recv() => {
                handle_coordination_message(coordinator_msg).await;
            },
            health_update = health_channel.recv() => {
                update_cluster_health(health_update).await;
            },
            _ = timer::after(Duration::from_secs(30)) => {
                perform_periodic_maintenance().await;
            }
        }
    }
}
```

## Custom Runtime Examples

### Game Engine Runtime

```cpp
// Game-specific runtime with frame-based signal handling
struct GameEngineRuntime {
    frame_rate: u32,
    render_thread: Option<ThreadHandle>,
    audio_thread: Option<ThreadHandle>,
}

impl Runtime for GameEngineRuntime {
    fn init() {
        init_graphics_system();
        init_audio_system();
        spawn_render_thread();
        spawn_audio_thread();
    }
    
    fn setup_signals() {
        // Game-specific signal handling
        // No signal thread - poll signals each frame
        signal(SIGINT, SIG_IGN);  // Handle in game loop
        signal(SIGUSR1, SIG_IGN); // Debug commands handled per-frame
    }
    
    const HAS_COROUTINES: bool = false;
    const HAS_FRAME_BASED_SIGNALS: bool = true;
    
    fn cleanup() {
        shutdown_audio_system();
        shutdown_graphics_system();
    }
}

fn main() = GameEngineRuntime { frame_rate: 60 } {
    let mut game = initialize_game();
    let mut last_frame = SystemTime::now();
    
    loop {
        // Frame-based signal polling
        check_signals_this_frame();
        
        let now = SystemTime::now();
        let delta_time = now.duration_since(last_frame).as_secs_f32();
        last_frame = now;
        
        catch(GAME_ERROR) {
            game.update(delta_time)?;
            game.render()?;
        } recover {
            // Handle game errors without crashing
            game.handle_error(current_signal());
            game.render_error_screen();
        }
        
        maintain_frame_rate(60);
    }
}
```

### Embedded System Runtime

```cpp
// Real-time embedded system runtime
struct EmbeddedRuntime {
    priority_levels: u8,
    max_interrupt_latency: Duration,
}

impl Runtime for EmbeddedRuntime {
    fn init() {
        disable_virtual_memory();
        lock_memory_pages();
        set_real_time_priority();
        configure_hardware_timers();
    }
    
    fn setup_signals() {
        // Hardware interrupts as signals
        install_interrupt_handlers();
        
        // Critical signals have immediate handlers
        signal(HARDWARE_FAULT, immediate_fault_handler);
        signal(WATCHDOG_TIMER, immediate_reset_handler);
        
        // Non-critical signals can be deferred
        signal(SENSOR_DATA, queue_for_processing);
    }
    
    const HAS_REAL_TIME: bool = true;
    const MAX_INTERRUPT_LATENCY_US: u32 = 50;
}

fn main() = EmbeddedRuntime { 
    priority_levels: 8,
    max_interrupt_latency: Duration::from_micros(50)
} {
    let mut sensor_system = initialize_sensors();
    let mut control_system = initialize_controllers();
    
    loop {
        catch(SENSOR_ERROR | CONTROL_ERROR | HARDWARE_FAULT) {
            // Real-time control loop
            let sensor_data = sensor_system.read_all()?;
            let control_commands = control_system.compute(sensor_data)?;
            control_system.apply_commands(control_commands)?;
        } recover {
            // Fault handling in real-time context
            match current_signal() {
                SENSOR_ERROR(err) => {
                    use_backup_sensors();
                    log_sensor_fault(err);
                },
                CONTROL_ERROR(err) => {
                    enter_safe_mode();
                    signal_fault_to_host();
                },
                HARDWARE_FAULT(fault) => {
                    emergency_shutdown();
                    panic!("Critical hardware fault: {}", fault);
                }
            }
        }
        
        // Precise timing control
        wait_for_next_control_cycle();
    }
}
```

## Complete Signal Flow Examples

### Default Runtime Signal Flow

```cpp
fn main() = default {
    // Manual OS signal handling
    signal(SIGINT, |sig| {
        write(2, "Terminating...\n", 15);
        cleanup_and_exit(0);
    });
    
    // Internal signal handling through catch/recover
    catch(PROCESSING_ERROR | IO_ERROR) {
        let data = load_input_data()?;          // May raise IO_ERROR
        let result = process_data(&data)?;       // May raise PROCESSING_ERROR
        save_results(&result)?;                  // May raise IO_ERROR
    } recover {
        match current_signal() {
            PROCESSING_ERROR(proc_err) => {
                eprintln!("Processing failed: {}", proc_err.message);
                exit(1);
            },
            IO_ERROR(io_err) => {
                eprintln!("I/O error: {} ({})", io_err.path, io_err.error_code);
                exit(2);
            }
        }
    }
    
    println!("Processing completed successfully");
}

// Flow:
// 1. OS signal (SIGINT) → Custom handler → cleanup_and_exit()
// 2. Internal signal → catch/recover → local handling
// 3. Unhandled signal → program termination
```

### Scheduler Runtime Signal Flow

```cpp
fn main() = std::CoroutineScheduler {
    let scheduler = CoroutineScheduler::new()
        .handle(SIGINT, |_| async {
            // OS signal becomes async operation
            initiate_graceful_shutdown().await;
        })
        .handle(DATABASE_ERROR, |db_err| async {
            // Application signal becomes async recovery
            switch_to_backup_database().await;
            retry_failed_operations().await;
        });
    
    // All work happens in coroutines
    spawn async {
        catch(NETWORK_ERROR) {
            handle_network_requests().await?;
        } recover {
            // Signal handling is async
            let recovery_plan = create_recovery_plan(current_signal()).await;
            execute_recovery(recovery_plan).await;
        }
    };
    
    // Scheduler manages everything
    scheduler.run().await;
}

// Flow:
// 1. OS signal → Signal thread → SignalCoroutine → Async handler
// 2. Worker thread signal → Thread unwinds → SignalCoroutine → Scheduler
// 3. Internal signal in coroutine → catch/recover → Local async handling
// 4. Unhandled signal → Coroutine termination → Optional global handler
```

### Custom Runtime Signal Flow

```cpp
fn main() = GameEngineRuntime { frame_rate: 60 } {
    let mut game_state = GameState::new();
    
    loop {
        // Frame-based signal checking
        if check_signal_flags() {
            match get_pending_signal() {
                Some(DEBUG_SIGNAL) => {
                    game_state.toggle_debug_mode();
                    clear_signal_flag(DEBUG_SIGNAL);
                },
                Some(SAVE_SIGNAL) => {
                    game_state.save_to_disk();
                    clear_signal_flag(SAVE_SIGNAL);
                },
                Some(QUIT_SIGNAL) => {
                    break;
                }
                _ => {}
            }
        }
        
        // Game logic with signal handling
        catch(RENDER_ERROR | PHYSICS_ERROR) {
            game_state.update(frame_delta)?;
            game_state.render()?;
        } recover {
            // Error handling within game context
            game_state.handle_subsystem_error(current_signal());
            game_state.render_error_overlay();
        }
        
        wait_for_next_frame();
    }
    
    game_state.cleanup();
}

// Flow:
// 1. OS signal → Set flag → Check next frame → Game-specific handling
// 2. Internal signal → catch/recover → Game error handling
// 3. Critical signal → Immediate handler → Game shutdown
```

## Runtime Comparison

| Feature | Default | Scheduler | Custom |
|---------|---------|-----------|---------|
| **Signal Thread** | No | Yes | Optional |
| **Async Handlers** | No | Yes | Optional |
| **OS Signal Policy** | Direct/Manual | Convert to Coroutines | Domain-specific |
| **Internal Signals** | Stack unwinding | Stack unwinding + Async | Domain-specific |
| **Performance** | Minimal overhead | Coroutine overhead | Optimized for domain |
| **Complexity** | Very simple | Complex scheduler | Varies |
| **Use Cases** | CLI tools, services | Web servers, distributed systems | Games, embedded, real-time |

## Performance Implications

### Default Runtime Performance

```cpp
// Zero overhead when no signals occur
fn hot_path() {
    for i in 0..1_000_000 {
        process_item(i);  // No signal handling overhead
    }
}

// Minimal overhead for signal-aware functions
fn monitored_operation() except(RARE_ERROR) -> Result<()> {
    // Normal execution: ~1-2 extra instructions
    // Signal path: Full exception handling cost
    for i in 0..large_number {
        if rare_condition(i) {
            raise(RARE_ERROR { iteration: i });  // Expensive path
        }
        normal_processing(i);  // Fast path
    }
}
```

### Scheduler Runtime Performance

```cpp
// Coroutine creation overhead for signals
// OS signal → ~1000 cycles (coroutine creation + scheduling)
// vs Default runtime → ~100 cycles (direct handler)

// But enables async signal handling:
async fn complex_signal_handler() {
    // Can perform I/O, wait on channels, etc.
    let backup_data = load_from_backup_server().await;
    notify_administrators(backup_data).await;
    update_monitoring_dashboard().await;
}
```

## Best Practices

### Runtime Selection Guidelines

```cpp
// Use default runtime for:
// - Simple command-line tools
// - System utilities
// - Services with minimal concurrency
// - Maximum performance single-threaded work

// Use scheduler runtime for:
// - Web servers and network services
// - Distributed systems
// - Applications requiring async I/O
// - Complex signal handling workflows

// Use custom runtime for:
// - Games (frame-based signal checking)
// - Embedded systems (real-time constraints)
// - Domain-specific performance requirements
// - Integration with existing runtime systems
```

### Signal Handler Design

```cpp
// Default runtime: Keep handlers simple and fast
signal(SIGUSR1, |sig| {
    // Only async-signal-safe operations
    write(2, "Status requested\n", 17);
    dump_stats_to_file();  // Must be signal-safe
});

// Scheduler runtime: Leverage async capabilities
scheduler.handle(SIGUSR1, |_| async {
    // Full async operations available
    let stats = collect_comprehensive_stats().await;
    let report = format_stats_report(stats).await;
    send_stats_to_monitoring(report).await;
});
```

## Cross-References

- **Signal Syntax**: See [Signal Handling](signal-handling.md) for language primitives and syntax
- **Coroutine Details**: See [Coroutines](coroutines.md) for coroutine-specific signal handling architecture
- **Channel Integration**: See [Channels](channels.md) for signal handling with channel operations
- **Memory Safety**: See [Memory Management](memory-management.md) for signal handling with RAII guarantees
- **Performance**: See [Compilation](compilation.md) for runtime selection impact on compilation

CPrime's runtime system provides flexible execution models that can be optimized for different use cases while maintaining consistent signal handling semantics across all runtime choices.