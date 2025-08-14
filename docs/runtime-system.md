# CPrime Execution Runtime System

## Overview

CPrime's **execution runtime system** provides the execution context for programs, determining how signals are handled, how concurrency is managed, and how system resources are utilized. The language separates **signal handling primitives** (syntax and semantics) from **execution runtime behavior** (execution policies), allowing the same code to run with different performance and safety characteristics.

> **Important**: This document discusses the **execution runtime system** (program execution environment), not the **`runtime` keyword** which signals performance costs in language constructs. For the `runtime` keyword, see [Runtime/Comptime Keywords](runtime-comptime-keywords.md).

The execution runtime is selected at program entry using the `fn main() = RuntimeChoice` syntax, and each runtime implements a common interface while providing different capabilities and trade-offs.

For signal handling language primitives and syntax, see [Signal Handling](signal-handling.md).

## Entry Point Revolution: Runtime-Controlled Program Entry

### Beyond Traditional main() Function

CPrime revolutionizes program entry points by replacing the fixed `int main(int argc, char* argv[])` signature with **runtime-controlled entry points** that use assignment operator binding. This design separates user program logic from platform-specific initialization and allows complete control over the startup sequence.

#### The Core Innovation

```cpp
// Traditional C++ (limited and inflexible)
int main(int argc, char* argv[]) {
    // Hidden initialization sequence
    // Fixed signature
    // Platform-specific variations
    // Cannot test as regular function
}

// CPrime entry point (flexible and explicit)
return_type function_name(params) = runtime_expression { body }
```

**Key Benefits:**
- **Any function name** - not limited to `main`
- **Any signature** - runtime determines accepted parameters  
- **Explicit initialization** - full control over startup sequence
- **Platform independence** - same source works across targets
- **Testable entry points** - entry functions are regular functions

### Entry Point Assignment Syntax

#### Basic Forms

```cpp
// Regular function (NOT an entry point)
int main() { 
    return 0;
}

// Entry point with default runtime
int main() = default {
    println("Hello, World!");
    return 0;
}

// Entry point with custom runtime
task<> server() = coroutine {
    co_await run_server();
}

// Entry point with bare metal runtime  
void boot() = bare_metal {
    initialize_hardware();
    run_kernel();
}
```

#### Runtime Parameterization

Runtimes can accept constructor-style parameters for configuration:

```cpp
// Parameterized default runtime
int app() = default(
    .heap_size = 4_MB,
    .stack_size = 64_KB
) {
    run_application();
    return 0;
}

// Coroutine runtime with configuration
task<> server() = coroutine(
    .scheduler = work_stealing,
    .thread_pool = 8,
    .max_coroutines = 10000
) {
    co_await run_async_server();
}

// Game engine with custom settings
void game() = game_engine(
    .frame_rate = 60,
    .renderer = vulkan,
    .audio_threads = 2
) {
    run_game_loop();
}
```

### Staged Initialization System

#### The Problem with Traditional Initialization

Traditional C++ startup has a hidden, uncontrollable sequence:
```
_start → __libc_start_main → global_ctors → main
```

Problems:
- **Opaque sequence** - developer has no control
- **Ordering issues** - global constructor dependencies
- **Platform variations** - different startup on each platform
- **No customization** - one size fits all approach

#### CPrime's Solution: Explicit Stages

CPrime allows explicit control over initialization through **stage objects**:

```cpp
// Stage objects define initialization phases
struct heap_stage {
    size: usize,
    static void init(size: usize) {
        initialize_heap(size);
    }
}

struct globals_stage {
    static void init() {
        run_global_constructors();
    }
}

struct signal_stage {
    defer: bool,
    static void init(defer: bool) {
        setup_signal_handlers(defer);
    }
}
```

#### Staged Entry Points

Entry points can specify explicit initialization stages:

```cpp
// Entry point with staged initialization
int server() = production_runtime(
    heap_stage{.size = 1_GB},           // 1. Initialize heap
    logging_stage{"server.log"},         // 2. Setup logging  
    globals_stage{},                     // 3. Run constructors
    network_stage{.port = 8080},         // 4. Initialize network
    signal_stage{.defer = true}          // 5. Setup signals
) {
    // User code runs after all stages complete
    return run_server();
}
```

#### Stage Composition and Dependencies

Stages can be composed and have dependencies:

```cpp
// Complex staged initialization
void embedded_system() = bare_metal(
    // Hardware initialization stages
    vectors_stage{.base = 0x0000},       // Interrupt vectors first
    memory_stage{.size = 512_KB},        // Memory management  
    clock_stage{.frequency = 100_MHz},   // System clock
    
    // Conditional stages (compile-time)
    #[cfg(feature = "uart")]
    uart_stage{.baud = 115200},
    
    #[cfg(feature = "networking")]
    ethernet_stage{.dhcp = false},
    
    // Optional stages (runtime)
    optional sensor_stage{.i2c_bus = 1},
    optional display_stage{.spi_bus = 0},
    
    // Final system stage
    system_ready_stage{}
) {
    main_control_loop();
}
```

### Runtime Types and Signature Flexibility

#### Default Runtime

The default runtime maintains C++ compatibility:

```cpp
// Traditional signature still supported
int main() = default {
    return 0;
}

// Extended default runtime
int app(args: &[String]) = default {
    process_arguments(args);
    return 0;
}

// With staged initialization
int service() = default(
    logging_stage{"/var/log/service.log"},
    globals_stage{},
    signal_stage{.graceful_shutdown = true}
) {
    run_service();
    return 0;
}
```

#### Bare Metal Runtime

For embedded systems and kernels:

```cpp
// Minimal embedded entry point
void kernel() = bare_metal(
    stack_stage{.size = 4_KB},
    vectors_stage{.base = 0x0000}
) {
    // No standard library initialization
    // No global constructors unless explicit
    initialize_hardware();
    kernel_main_loop();
    // Never returns
}

// Bootloader entry point
void bootloader() = bare_metal(
    memory_stage{.size = 64_KB}
) {
    early_hardware_init();
    load_kernel();
    jump_to_kernel();
}
```

#### Coroutine Runtime

For async applications:

```cpp
// Web server entry point  
task<> server() = coroutine(
    scheduler_stage{.type = work_stealing, .threads = 8},
    async_io_stage{.epoll_size = 1024}
) {
    let listener = TcpListener::bind("0.0.0.0:8080").await?;
    
    loop {
        let (stream, addr) = listener.accept().await?;
        spawn handle_connection(stream, addr);
    }
}

// Database application
result<()> db_app(connection_string: String) = coroutine(
    thread_pool_stage{.workers = 4},
    connection_pool_stage{.max_connections = 100}
) {
    let db = Database::connect(&connection_string).await?;
    co_await run_database_operations(&db);
    Ok(())
}
```

#### Domain-Specific Runtimes

Custom runtimes for specific domains:

```cpp
// Game engine runtime
void game() = game_engine(
    renderer_stage{.api = vulkan, .vsync = true},
    audio_stage{.sample_rate = 44100},
    physics_stage{.timestep = 16_ms}
) {
    let mut game_state = GameState::new();
    
    loop {
        game_state.update();
        game_state.render();
        
        if game_state.should_quit() {
            break;
        }
    }
}

// Web server runtime
response handle_request(request: HttpRequest) = http_server(
    tls_stage{.cert = "server.pem", .key = "server.key"},
    routing_stage{.static_dir = "./public"},
    middleware_stage{.compression = true}
) {
    match request.path() {
        "/api/users" => handle_users_api(request),
        "/api/posts" => handle_posts_api(request),
        _ => Response::not_found()
    }
}

// Real-time system runtime
void control_loop() = real_time(
    priority_stage{.level = RealTimePriority::High},
    memory_stage{.locked = true, .size = 1_MB},
    timing_stage{.period = Duration::from_micros(100)}
) {
    let mut controller = PIDController::new();
    
    loop {
        let sensor_data = read_sensors();
        let control_output = controller.update(sensor_data);
        apply_control_output(control_output);
        
        wait_for_next_period();
    }
}
```

### Implementation Architecture

#### Compiler Responsibilities

The compiler handles entry point transformation:

```cpp
// Source code
int app() = default { return run(); }

// Compiler generates platform-specific entry point
#ifdef __linux__
extern "C" int main(int argc, char* argv[]) {
    // Initialize default runtime
    default_runtime::init();
    
    // Set up staged initialization (if any)
    default_runtime::run_stages();
    
    // Call user entry point  
    int result = app();
    
    // Runtime cleanup
    default_runtime::cleanup();
    return result;
}
#endif

#ifdef _WIN32
extern "C" int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                             LPSTR lpCmdLine, int nCmdShow) {
    default_runtime::init_windows(hInstance, lpCmdLine);
    default_runtime::run_stages();
    int result = app();
    default_runtime::cleanup();
    return result;
}
#endif
```

#### Platform Adaptation

Same source code works across platforms:

```cpp
// Single source file
int app() = default {
    println("Hello from CPrime!");
    return 0;
}

// Compiler generates appropriate symbols:
// --target=linux    → generates 'main' symbol
// --target=windows  → generates 'WinMain' symbol  
// --target=embedded → generates '_start' symbol
// --target=wasm     → generates '_start' and export table
```

#### Zero-Overhead Guarantee

Entry point binding has zero runtime overhead:

```cpp
// Simple entry point
int app() = default { return 0; }

// Compiles to identical code as:
int main() { return 0; }

// Staged initialization is inlined when possible
int app() = default(
    heap_stage{.size = 1_MB}
) { 
    return 0; 
}

// Becomes:
int main() {
    initialize_heap(1048576);  // Inlined
    return 0;
}
```

### Advanced Features

#### Multiple Entry Points

Different entry points for different build configurations:

```cpp
// Production entry point
int production() = default(
    logging_stage{"/var/log/app.log"},
    globals_stage{},
    signal_stage{.graceful_shutdown = true}
) {
    run_production_server();
    return 0;
}

// Debug entry point  
int debug() = debug_runtime(
    logging_stage{stdout},
    debug_stage{.memory_tracking = true},
    globals_stage{}
) {
    run_debug_server();
    return 0;
}

// Test entry point
int test() = test_runtime {
    run_all_tests();
    return 0;
}

// Selected at link time:
// cprime build --entry=production  # Links production()
// cprime build --entry=debug      # Links debug()  
// cprime build --entry=test       # Links test()
```

#### Testing Support

Entry points become testable regular functions:

```cpp
// Entry point
int app() = default {
    return run();
}

// Testable function (extracted logic)
int run() {
    let config = load_configuration();
    let result = process_data(config);
    save_results(result);
    return 0;
}

// Unit tests
#[test]
fn test_run_with_valid_config() {
    // Test run() directly without entry point overhead
    assert_eq!(run(), 0);
}

#[test] 
fn test_run_with_invalid_config() {
    // Test error conditions
    assert_eq!(run(), 1);
}
```

#### Error Handling in Stages

Stage failures can be handled gracefully:

```cpp
// Stage with error handling
int server() = production_runtime(
    heap_stage{.size = 1_GB} catch {
        // Fallback to smaller heap
        heap_stage{.size = 512_MB}
    },
    
    network_stage{.port = 8080} catch {
        // Try alternative port
        network_stage{.port = 8081}
    } catch {
        // Final fallback
        eprintln("Cannot initialize network");
        exit(1);
    },
    
    globals_stage{}
) {
    run_server();
    return 0;
}
```

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

## Integration with CPrime's Architecture

### Three-Class System Integration

Runtime and stage objects follow CPrime's three-class system patterns:

```cpp
// Stage objects as data classes
class NetworkStage {
    port: u16,
    bind_address: IpAddr,
    max_connections: u32,
    
    constructed_by: NetworkStageOps,
}

// Stage operations as functional classes  
functional class NetworkStageOps {
    fn construct(port: u16) -> NetworkStage {
        NetworkStage {
            port,
            bind_address: IpAddr::any(),
            max_connections: 1000,
        }
    }
    
    fn init(stage: &NetworkStage) -> Result<()> {
        let listener = TcpListener::bind((stage.bind_address, stage.port))?;
        listener.set_max_connections(stage.max_connections);
        GLOBAL_LISTENER.store(listener);
        Ok(())
    }
}

// Runtime data classes hold initialization state
class DefaultRuntime {
    stages: Vec<Box<dyn StageInit>>,
    initialized: bool,
    cleanup_handlers: Vec<CleanupFn>,
    
    constructed_by: DefaultRuntimeOps,
}

functional class DefaultRuntimeOps {
    fn construct() -> DefaultRuntime {
        DefaultRuntime {
            stages: Vec::new(),
            initialized: false,
            cleanup_handlers: Vec::new(),
        }
    }
    
    fn add_stage(runtime: &mut DefaultRuntime, stage: Box<dyn StageInit>) {
        runtime.stages.push(stage);
    }
    
    fn initialize(runtime: &mut DefaultRuntime) -> Result<()> {
        for stage in &runtime.stages {
            stage.init()?;
        }
        runtime.initialized = true;
        Ok(())
    }
}
```

### Signal Handling Integration

Entry point runtime selection affects signal handling behavior:

```cpp
// Default runtime: Manual signal handling
int server() = default {
    signal(SIGINT, |_| {
        write(2, "Shutting down\n", 14);
        cleanup_and_exit(0);
    });
    
    run_server_loop();
    return 0;
}

// Coroutine runtime: Async signal handling
task<> server() = coroutine {
    // Signals automatically converted to async operations
    scheduler.handle(SIGINT, |_| async {
        println("Graceful shutdown initiated");
        shutdown_all_connections().await;
        save_state().await;
    });
    
    co_await run_async_server();
}

// Staged initialization with signal setup
int server() = default(
    logging_stage{"/var/log/server.log"},
    globals_stage{},
    signal_stage{.graceful_shutdown = true, .reload_config = true}
) {
    // Signals already configured by signal_stage
    run_server_with_signals();
    return 0;
}
```

### Comptime Integration

Entry point binding can leverage comptime execution:

```cpp
// Comptime-generated entry points
comptime {
    registerSyntaxPattern("@main $func", (match) => {
        return quote {
            int generated_main() = default {
                return $(match.func)();
            }
        };
    });
}

// Usage
@main my_application  // Generates entry point automatically

// Comptime stage generation
comptime {
    fn generate_database_stages(config: DatabaseConfig) -> StageList {
        let mut stages = Vec::new();
        
        stages.push(quote { heap_stage{.size = $(config.heap_size)} });
        
        if config.enable_logging {
            stages.push(quote { logging_stage{$(config.log_file)} });
        }
        
        if config.enable_metrics {
            stages.push(quote { metrics_stage{.port = $(config.metrics_port)} });
        }
        
        stages
    }
}

// Generated entry point with computed stages
int database_server() = production_runtime(
    $(generate_database_stages(DATABASE_CONFIG))
) {
    run_database_server();
    return 0;
}
```

### Memory Management Integration

Entry points and stages respect RAII principles:

```cpp
// RAII-compliant stage objects
class TlsStage {
    certificate_path: String,
    private_key_path: String,
    
    constructed_by: TlsStageOps,
}

functional class TlsStageOps {
    fn construct(cert: String, key: String) -> TlsStage {
        TlsStage {
            certificate_path: cert,
            private_key_path: key,
        }
    }
    
    fn destruct(stage: &mut TlsStage) {
        // Automatic cleanup of TLS resources
        cleanup_tls_context();
        clear_private_key_memory();
    }
    
    fn init(stage: &TlsStage) -> Result<()> {
        let cert = load_certificate(&stage.certificate_path)?;
        let key = load_private_key(&stage.private_key_path)?;
        initialize_tls_context(cert, key);
        Ok(())
    }
}

// RAII cleanup happens automatically
int https_server() = production_runtime(
    tls_stage{"server.crt", "server.key"},  // Auto-cleanup on exit
    network_stage{.port = 443}              // Auto-cleanup on exit
) {
    run_https_server();
    return 0;  // All stages automatically destructed
}
```

## Cross-References

- **[Signal Handling](signal-handling.md)**: Entry point runtime selection determines signal handling behavior (direct handlers vs async coroutines)
- **[Coroutines](coroutines.md)**: Coroutine runtime enables async entry points and signal-to-coroutine conversion
- **[Channels](channels.md)**: Channel-based communication patterns work with all runtime types
- **[Memory Management](memory-management.md)**: Stage objects follow RAII principles for automatic cleanup
- **[Three-Class System](three-class-system.md)**: Runtime and stage objects use Data/Functional class patterns
- **[Comptime Execution](comptime-execution.md)**: Entry point binding can use comptime for stage generation and syntax transformation
- **[Compilation Model](compilation.md)**: Entry point selection affects generated symbols and platform adaptation
- **[Runtime/Comptime Keywords](runtime-comptime-keywords.md)**: Entry point system uses performance indication principles

## Conclusion

CPrime's runtime-controlled entry point system represents a revolutionary approach to program initialization that:

### Eliminates Traditional Limitations
- **Fixed signatures** → Flexible, runtime-determined signatures
- **Hidden initialization** → Explicit, controllable staged initialization  
- **Platform lock-in** → Cross-platform source compatibility
- **Untestable entry points** → Regular, testable functions

### Enables Powerful Capabilities
- **Staged initialization** with explicit ordering and dependencies
- **Multiple entry points** for different build configurations
- **Domain-specific runtimes** optimized for specific use cases
- **Zero-overhead abstraction** with compile-time stage inlining

### Maintains CPrime's Core Principles
- **Explicit control** over initialization sequence
- **Performance transparency** with zero-cost abstractions
- **Memory safety** through RAII-compliant stage objects
- **Architectural clarity** using the three-class system

The entry point revolution transforms program startup from a hidden, inflexible process into an explicit, controllable, and optimizable system that adapts to any domain while maintaining the performance and safety characteristics essential for systems programming.