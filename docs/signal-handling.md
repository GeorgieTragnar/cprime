# CPrime Signal Handling

## Overview

CPrime provides a unified signal handling system that treats OS signals and exceptions with the same syntax and semantics. The language provides runtime-agnostic primitives that work consistently across different execution models, from simple single-threaded programs to complex coroutine schedulers.

The system is built on three core principles:
1. **Explicit over implicit** - No hidden signal polling or automatic handlers
2. **Trust the programmer** - Allow potentially unsafe operations with warnings
3. **Progressive safety** - Configurable safety levels from C-style to Rust-level

For runtime-specific signal behavior and execution models, see [Runtime System](runtime-system.md).

## Core Language Primitives

### Signal Definition

CPrime supports both built-in OS signals and custom application signals:

```cpp
// Built-in OS signals (automatically available)
SIGINT, SIGTERM, SIGSEGV, SIGPIPE, SIGUSR1, SIGUSR2

// Custom signals with structured data
signal CUSTOM_ERROR {
    message: String,
    error_code: i32,
    context: ErrorContext,
    timestamp: u64,
}

signal NETWORK_ERROR {
    endpoint: SocketAddr,
    retry_count: u32,
    last_attempt: SystemTime,
}

signal OOM {
    requested_size: usize,
    available_memory: usize,
}

// Simple signals without data
signal TASK_COMPLETE;
signal SHUTDOWN_REQUESTED;
```

### Function Signal Annotations

Functions must declare which signals they can propagate using `except` annotations:

```cpp
// Function that can propagate specific signals
fn process_request(data: &RequestData) except(NETWORK_ERROR, OOM) -> Result<Response> {
    let connection = establish_connection()?;  // Can raise NETWORK_ERROR
    let parsed = parse_large_data(data)?;      // Can raise OOM
    Ok(build_response(parsed))
}

// Function without except annotation cannot propagate signals
fn safe_handler(request: Request) -> Response {
    // Any unhandled signal here will terminate the program
    // Must use catch/recover blocks for local handling
    Response::ok()
}

// Function that handles all signals internally
fn robust_processor(input: &str) -> ProcessResult {
    catch(PARSE_ERROR | VALIDATION_ERROR) {
        let parsed = parse_input(input)?;
        validate_data(&parsed)?;
        process_data(parsed)
    } recover {
        // Handle errors locally - cannot propagate
        ProcessResult::error("Input processing failed")
    }
}
```

### Signal Handling Syntax

The `catch/recover` construct provides structured signal handling:

```cpp
// Basic catch/recover
fn example() {
    catch(FILE_ERROR) {
        let data = read_file("config.txt")?;  // May raise FILE_ERROR
        process_config(data);
    } recover {
        // Executed if FILE_ERROR is raised
        use_default_config();
    }
    
    // Execution continues here regardless of signal
}

// Multiple signal types
fn multi_handler() {
    catch(NETWORK_ERROR | TIMEOUT_ERROR | AUTH_ERROR) {
        let response = make_authenticated_request()?;
        process_response(response);
    } recover {
        // Handle any of the three error types
        match current_signal() {
            NETWORK_ERROR(data) => retry_with_backup(data.endpoint),
            TIMEOUT_ERROR => increase_timeout_and_retry(),
            AUTH_ERROR(auth_data) => refresh_credentials(auth_data),
        }
    }
}

// Nested catch blocks
fn nested_handling() {
    catch(OUTER_ERROR) {
        catch(INNER_ERROR) {
            risky_operation()?;          // May raise INNER_ERROR
            another_risky_operation()?;  // May raise OUTER_ERROR
        } recover {
            handle_inner_error();
        }
        
        final_operation()?;  // May raise OUTER_ERROR
    } recover {
        handle_outer_error();
    }
}
```

### Signal Operations

```cpp
// Raise a signal with data
fn validate_input(input: &str) -> Result<ParsedInput> {
    if input.len() > MAX_INPUT_SIZE {
        raise(VALIDATION_ERROR {
            message: "Input too large".to_string(),
            max_size: MAX_INPUT_SIZE,
            actual_size: input.len(),
        });
    }
    
    Ok(parse_input(input))
}

// Panic for unrecoverable errors (immediate termination)
fn critical_invariant_check(state: &SystemState) {
    if state.is_corrupted() {
        panic!("System state corruption detected");
        // Program terminates immediately - no unwinding
    }
}

// Defer cleanup operations
fn resource_handler() -> Result<()> {
    let handle = acquire_resource()?;
    defer release_resource(handle);  // Runs on ANY function exit
    
    // Complex operations that might signal
    catch(PROCESSING_ERROR) {
        process_with_resource(&handle)?;
        transform_resource(&handle)?;
    } recover {
        log_processing_error();
        // defer still runs - resource is released
    }
    
    Ok(())  // defer runs here too
}
```

## Signal Propagation Rules

### Priority Order

Signal handling follows a strict priority order:

1. **In catch block for that signal** → Execute recover block
2. **In function with `except(signal)`** → Propagate to caller  
3. **In function without `except`** → Terminate program
4. **Nested same signal** → Terminate program (prevents infinite loops)
5. **Signal during unwind** → Terminate program
6. **Different signal in catch** → Terminate program

```cpp
// Example of propagation rules
fn caller() {
    // No except annotation - cannot propagate
    catch(ERROR) {
        propagating_function()?;  // ERROR handled locally
    } recover {
        println!("Handled ERROR locally");
    }
    
    // This would terminate the program:
    // propagating_function()?;  // ERROR has nowhere to go
}

fn propagating_function() except(ERROR) {
    // Can propagate ERROR to caller
    risky_operation()?;  // ERROR propagates upward
}

fn risky_operation() except(ERROR) -> Result<()> {
    if something_bad() {
        raise(ERROR { message: "Something went wrong".to_string() });
    }
    Ok(())
}
```

### Hard Signals

Some signals always terminate the program regardless of context:

```cpp
// These signals cannot be caught or handled:
SIGSEGV    // Segmentation fault
SIGABRT    // Abort signal  
SIGFPE     // Floating point exception
SIGILL     // Illegal instruction

// Example - this will not compile:
fn impossible() {
    catch(SIGSEGV) {        // ❌ Compile error
        dangerous_operation();
    } recover {
        // Cannot handle segmentation faults
    }
}
```

### Signal State Restrictions

Certain signal handling patterns are forbidden to prevent undefined behavior:

```cpp
fn restricted_patterns() {
    catch(ERROR_A) {
        catch(ERROR_A) {        // ❌ Nested same signal
            operation()?;
        } recover {
            // This would create infinite recursion potential
        }
    } recover {
        // Original ERROR_A handler
    }
    
    catch(ERROR_B) {
        operation()?;           // May raise ERROR_B
        
        catch(ERROR_C) {
            other_operation()?;  // May raise ERROR_B  
            // ❌ ERROR_B in nested catch terminates program
        } recover {
            // ERROR_C handling only
        }
    } recover {
        // ERROR_B handling
    }
}
```

## Pattern Matching in Signal Handling

### Destructuring Signal Data

```cpp
// Pattern match on signal data
fn detailed_error_handling() {
    catch(VALIDATION_ERROR { field, expected, actual, .. }) {
        complex_validation()?;
    } recover {
        // field, expected, actual are in scope
        println!("Validation failed on field '{}': expected {}, got {}", 
                field, expected, actual);
    }
}

// Multiple signals with different patterns
fn multi_pattern_handling() {
    catch(NETWORK_ERROR { endpoint, retry_count, .. } | TIMEOUT_ERROR { duration, .. }) {
        network_operation()?;
    } recover {
        match current_signal() {
            NETWORK_ERROR(net_err) => {
                if net_err.retry_count < MAX_RETRIES {
                    schedule_retry(net_err.endpoint);
                } else {
                    mark_endpoint_failed(net_err.endpoint);
                }
            },
            TIMEOUT_ERROR(timeout_err) => {
                if timeout_err.duration > MAX_TIMEOUT {
                    use_faster_algorithm();
                } else {
                    increase_timeout();
                }
            }
        }
    }
}
```

### Guard Patterns

```cpp
// Guard conditions in signal patterns
fn conditional_handling() {
    catch(PROCESSING_ERROR { severity, .. } if severity > CRITICAL_LEVEL) {
        critical_processing()?;
    } recover {
        // Only handle critical processing errors here
        shutdown_system();
        notify_administrators();
    }
    
    catch(PROCESSING_ERROR { retryable: true, attempt, .. } if attempt < MAX_ATTEMPTS) {
        retryable_processing()?;
    } recover {
        // Retry the operation
        schedule_retry_with_backoff(attempt);
    }
    
    catch(PROCESSING_ERROR) {  // Catch all remaining processing errors
        fallback_processing()?;
    } recover {
        // Handle non-critical or non-retryable errors
        log_error_and_continue();
    }
}
```

## Undefined Behavior and Safety Levels

### Warning System

CPrime provides configurable safety levels through compiler warnings:

```cpp
// Potentially unsafe operations generate warnings
extern "C" fn signal_handler(sig: i32) {
    println!("Signal received: {}", sig);  // ⚠️ Warning: not async-signal-safe
    malloc(100);                          // ⚠️ Warning: malloc in signal handler
    
    // Explicit permission to ignore warnings
    #[allow(signal_unsafe)]
    complex_operation();  // Warning suppressed
}

// Memory safety warnings
fn potentially_unsafe() {
    let ptr: *mut u8 = std::ptr::null_mut();
    unsafe {
        *ptr = 42;  // ⚠️ Warning: null dereference
    }
    
    let arr = [1, 2, 3];
    let idx = 10;
    let val = arr[idx];  // ⚠️ Warning: bounds check disabled
}
```

### Compiler Safety Levels

```bash
# Default: Warnings for all potential UB
cprime build main.cpp

# Strict mode: Warnings become errors (Rust-level safety)
cprime build --warnings-as-errors main.cpp

# Permissive mode: Suppress warnings (C-level permissiveness)  
cprime build --no-warnings main.cpp

# Custom warning configuration
cprime build --warn=memory_safety --error=null_dereference main.cpp
```

### Per-Module Safety Configuration

```cpp
// Module-level safety annotations
#![warn_as_error(memory_safety, null_dereference)]  // Critical safety required
mod payment_processing {
    // All memory safety warnings become compile errors
    fn process_payment(amount: Money) -> Result<Receipt> {
        // Must be perfect - no unsafe operations allowed
    }
}

#[allow(bounds_check)]  // Performance-critical module
mod hot_path {
    fn optimized_computation(data: &[f64]) -> f64 {
        // Bounds checking disabled for performance
        data[compute_index()]  // No bounds check warning
    }
}

#[allow(warnings)]  // Legacy code
mod legacy_implementation {
    // Gradually improve safety over time
    fn old_unsafe_function() {
        // Warnings suppressed during migration
    }
}
```

### Project-Level Safety Evolution

```toml
# project.toml - Progressive safety enforcement
[warnings.development]
all = "warn"                    # Start permissive
memory_safety = "warn"         
null_dereference = "error"      # Critical issues only

[warnings.testing]
memory_safety = "error"         # Increase safety for testing
bounds_check = "error"
signal_unsafe = "error"

[warnings.production]  
all = "error"                   # Full safety in production
performance_hints = "warn"      # Allow some flexibility

[warnings.critical_systems]
all = "error"                   # Maximum safety
performance_hints = "error"     # No compromises
```

## Advanced Signal Patterns

### Signal Chaining

```cpp
// Chain signals for error context propagation
fn operation_with_context() except(CONTEXT_ERROR) {
    catch(LOW_LEVEL_ERROR { code, .. }) {
        low_level_operation()?;
    } recover {
        // Transform low-level error into higher-level context
        raise(CONTEXT_ERROR {
            message: "Operation failed in high-level context".to_string(),
            operation: "complex_business_logic".to_string(),
            caused_by: current_signal(),  // Chain the original error
        });
    }
}
```

### Conditional Signal Propagation

```cpp
fn smart_error_handling() except(CRITICAL_ERROR) {
    catch(RECOVERABLE_ERROR { severity, .. }) {
        fallible_operation()?;
    } recover {
        if severity > CRITICAL_THRESHOLD {
            // Escalate to critical error
            raise(CRITICAL_ERROR {
                escalated_from: current_signal(),
                timestamp: SystemTime::now(),
            });
        } else {
            // Handle locally
            attempt_recovery();
        }
    }
}
```

### Resource-Aware Signal Handling

```cpp
fn resource_safe_operation() -> Result<ProcessedData> {
    let resource = acquire_expensive_resource()?;
    defer ResourceManager::release(resource);
    
    let temp_allocation = allocate_working_memory(LARGE_SIZE)?;
    defer deallocate_working_memory(temp_allocation);
    
    catch(PROCESSING_ERROR | OOM) {
        // Complex processing that might fail
        let intermediate = process_phase_1(&resource, &temp_allocation)?;
        let result = process_phase_2(intermediate, &resource)?;
        finalize_processing(result, &resource)?;
    } recover {
        // All defers still run - resources properly cleaned up
        match current_signal() {
            PROCESSING_ERROR(err) => {
                log_processing_failure(err);
                ProcessedData::empty()
            },
            OOM(oom) => {
                log_oom(oom.requested_size, oom.available_memory);
                ProcessedData::partial(temp_allocation.partial_results())
            }
        }
    }
}
```

## Integration with CPrime Features

### Signal Handling in Three-Class System

```cpp
// Data class with signal-aware operations
class ProcessorData {
    input_buffer: Vec<u8>,
    output_buffer: Vec<u8>,
    error_state: Option<ProcessingError>,
    
    constructed_by: ProcessorOps,
}

// Functional class with signal propagation
functional class ProcessorOps {
    fn construct() -> ProcessorData {
        ProcessorData {
            input_buffer: Vec::new(),
            output_buffer: Vec::new(),
            error_state: None,
        }
    }
    
    fn process(processor: &mut ProcessorData, input: &[u8]) except(PROCESSING_ERROR) {
        catch(BUFFER_OVERFLOW) {
            processor.input_buffer.extend_from_slice(input);
            let result = complex_processing(&processor.input_buffer)?;
            processor.output_buffer = result;
        } recover {
            // Transform buffer overflow to processing error
            processor.error_state = Some(ProcessingError::BufferTooSmall);
            raise(PROCESSING_ERROR {
                stage: "input_processing".to_string(),
                cause: current_signal(),
            });
        }
    }
}
```

### Signal Handling with Functional Classes

```cpp
class SecureProcessor {
    sensitive_data: EncryptedData,
    access_log: Vec<AccessEvent>,
    
    exposes UserOps { access_log }
    exposes AdminOps { sensitive_data, access_log }
    
    constructed_by: SecureProcessorManager,
}

functional class AdminOps {
    fn secure_process(processor: &mut SecureProcessor) except(SECURITY_ERROR) {
        catch(DECRYPTION_ERROR | ACCESS_DENIED) {
            let decrypted = decrypt_data(&processor.sensitive_data)?;
            let result = process_sensitive_data(decrypted)?;
            processor.access_log.push(AccessEvent::ProcessingComplete);
        } recover {
            match current_signal() {
                DECRYPTION_ERROR(err) => {
                    processor.access_log.push(AccessEvent::DecryptionFailed);
                    raise(SECURITY_ERROR {
                        level: SecurityLevel::High,
                        cause: "Decryption failure".to_string(),
                    });
                },
                ACCESS_DENIED => {
                    processor.access_log.push(AccessEvent::AccessDenied);
                    raise(SECURITY_ERROR {
                        level: SecurityLevel::Critical,
                        cause: "Unauthorized access attempt".to_string(),
                    });
                }
            }
        }
    }
}
```

## Best Practices

### When to Use Signals vs Results

```cpp
// Use Result<T, E> for expected failures
fn parse_number(input: &str) -> Result<i32, ParseError> {
    // Expected that some inputs won't parse
    input.parse().map_err(|_| ParseError::InvalidFormat)
}

// Use signals for exceptional conditions
fn allocate_memory(size: usize) except(OOM) -> *mut u8 {
    // OOM is exceptional, not expected
    let ptr = unsafe { malloc(size) };
    if ptr.is_null() {
        raise(OOM { requested_size: size, available_memory: get_available_memory() });
    }
    ptr
}

// Use panic! for unrecoverable errors
fn validate_invariant(state: &State) {
    if state.is_impossible_condition() {
        panic!("Impossible state detected - program logic error");
    }
}
```

### Signal Design Guidelines

```cpp
// Good: Structured signal with useful context
signal DATABASE_ERROR {
    query: String,
    connection_id: u32,
    error_code: DbErrorCode,
    retry_suggested: bool,
    timestamp: SystemTime,
}

// Bad: Vague signal without context
signal GENERIC_ERROR {
    message: String,  // Too generic
}

// Good: Specific, actionable signals
signal NETWORK_CONNECTION_LOST {
    endpoint: SocketAddr,
    last_successful_ping: SystemTime,
    retry_count: u32,
}

signal DISK_SPACE_LOW {
    mount_point: PathBuf,
    available_bytes: u64,
    threshold_bytes: u64,
}
```

### Performance Considerations

```cpp
// Signal handling has zero cost when no signals are raised
fn hot_path_function() {
    // No except annotation = no signal handling overhead
    for item in large_collection {
        process_item_fast(item);  // Zero overhead
    }
}

// Signal handling adds overhead only to exceptional paths
fn monitored_operation() except(RARE_ERROR) {
    // Normal path: minimal overhead
    for i in 0..1000000 {
        if unlikely_condition(i) {  // Rare
            raise(RARE_ERROR { iteration: i });
        }
        normal_processing(i);  // Fast path unaffected
    }
}
```

## Cross-References

- **Runtime Behavior**: See [Runtime System](runtime-system.md) for how different runtimes implement signal handling
- **Coroutine Integration**: See [Coroutines](coroutines.md) for signal-to-coroutine conversion in scheduler runtime
- **Language Overview**: See [Language Summary](language-summary.md) for high-level signal handling overview
- **Memory Safety**: See [Memory Management](memory-management.md) for signal handling with RAII
- **Type System**: See [Type System](type-system.md) for signal type checking and inference

CPrime's signal handling system provides a unified, safe, and flexible approach to error handling that scales from simple programs to complex distributed systems, all while maintaining the language's core principles of explicitness, performance, and programmer control.