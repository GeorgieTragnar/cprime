# CPrime Danger Zones

## Overview

Danger zones in CPrime provide escape hatches for operations that cannot be statically verified as safe. They enable FFI integration, raw pointer manipulation, and gradual migration from C++ codebases while maintaining clear boundaries between safe and unsafe code.

## Danger Attributes

### Core Danger Categories

```cpp
// Fine-grained permission system
#[danger(ffi)]               // Foreign function calls
#[danger(raw_pointers)]      // Raw pointer manipulation
#[danger(thread_bound)]      // Pin to specific OS thread
#[danger(stateful_class)]    // Traditional OOP class with state + methods
#[danger(unchecked_access)]  // Bypass access control
#[danger(inline_assembly)]   // Inline assembly code
#[danger(transmute)]         // Type punning and bit casting
```

### Attribute Combination

Multiple danger attributes can be combined for maximum flexibility:

```cpp
#[danger(ffi, raw_pointers, thread_bound)]
class OpenGLContext {
    gl_context: *mut c_void,
    window_handle: *mut c_void,
    thread_id: ThreadId,
    
    fn new() -> Result<Self> {
        // Ensure we're on the main thread (OpenGL requirement)
        assert_eq!(current_thread_id(), main_thread_id());
        
        danger {
            let context = ffi::wglCreateContext(window_handle);
            if context.is_null() {
                return Err("Failed to create OpenGL context");
            }
            
            Ok(OpenGLContext {
                gl_context: context,
                window_handle: get_window_handle(),
                thread_id: current_thread_id(),
            })
        }
    }
    
    fn make_current(&self) -> Result<()> {
        // Verify we're still on the correct thread
        assert_eq!(current_thread_id(), self.thread_id);
        
        danger {
            let result = ffi::wglMakeCurrent(self.window_handle, self.gl_context);
            if result == 0 {
                Err("Failed to make context current")
            } else {
                Ok(())
            }
        }
    }
}
```

## FFI Integration

### Basic FFI Declarations

```cpp
#[danger(ffi)]
mod sqlite {
    // External C functions
    extern "C" {
        fn sqlite3_open(filename: *const c_char, ppDb: *mut *mut sqlite3) -> c_int;
        fn sqlite3_close(db: *mut sqlite3) -> c_int;
        fn sqlite3_exec(
            db: *mut sqlite3,
            sql: *const c_char,
            callback: Option<unsafe extern "C" fn(*mut c_void, c_int, *mut *mut c_char, *mut *mut c_char) -> c_int>,
            data: *mut c_void,
            errmsg: *mut *mut c_char
        ) -> c_int;
    }
    
    // Opaque C types
    type sqlite3 = c_void;
    
    // Constants from C headers
    const SQLITE_OK: c_int = 0;
    const SQLITE_ERROR: c_int = 1;
}
```

### Safe Wrapper Pattern

Wrap dangerous FFI code in safe abstractions:

```cpp
// Safe wrapper for SQLite
class DatabaseData {
    handle: *mut sqlite::sqlite3,
    path: String,
    constructed_by: DatabaseOps,
}

functional class DatabaseOps {
    fn construct(path: &str) -> Result<DatabaseData> {
        danger {
            let mut handle: *mut sqlite::sqlite3 = null_mut();
            let c_path = CString::new(path)?;
            
            let result = sqlite::sqlite3_open(c_path.as_ptr(), &mut handle);
            
            if result == sqlite::SQLITE_OK {
                Ok(DatabaseData {
                    handle,
                    path: path.to_string(),
                })
            } else {
                if !handle.is_null() {
                    sqlite::sqlite3_close(handle);
                }
                Err(format!("Failed to open database: {}", result))
            }
        }
    }
    
    fn destruct(db: &mut DatabaseData) {
        if !db.handle.is_null() {
            danger {
                sqlite::sqlite3_close(db.handle);
            }
            db.handle = null_mut();
        }
    }
    
    fn execute(db: &DatabaseData, sql: &str) -> Result<()> {
        danger {
            let c_sql = CString::new(sql)?;
            let result = sqlite::sqlite3_exec(
                db.handle,
                c_sql.as_ptr(),
                None,
                null_mut(),
                null_mut()
            );
            
            if result == sqlite::SQLITE_OK {
                Ok(())
            } else {
                Err(format!("SQL execution failed: {}", result))
            }
        }
    }
}

// Usage - safe interface hides dangerous operations
let db = DatabaseOps::construct("database.db")?;
defer DatabaseOps::destruct(&mut db);

DatabaseOps::execute(&db, "CREATE TABLE users (id INTEGER, name TEXT)")?;
DatabaseOps::execute(&db, "INSERT INTO users VALUES (1, 'Alice')")?;
```

### C++ Interop

Direct interop with existing C++ codebases:

```cpp
#[danger(ffi, stateful_class)]
mod cpp_bridge {
    // C++ class wrapper
    extern "C++" {
        type CppVector;
        
        fn cpp_vector_new() -> *mut CppVector;
        fn cpp_vector_delete(vec: *mut CppVector);
        fn cpp_vector_push_back(vec: *mut CppVector, value: i32);
        fn cpp_vector_size(vec: *const CppVector) -> usize;
        fn cpp_vector_get(vec: *const CppVector, index: usize) -> i32;
    }
    
    // Safe CPrime wrapper
    #[danger(stateful_class)]
    class CppVectorWrapper {
        ptr: *mut CppVector,
        
        fn new() -> Self {
            danger {
                CppVectorWrapper {
                    ptr: cpp_vector_new()
                }
            }
        }
        
        fn drop(&mut self) {
            if !self.ptr.is_null() {
                danger {
                    cpp_vector_delete(self.ptr);
                }
                self.ptr = null_mut();
            }
        }
        
        fn push(&mut self, value: i32) {
            danger {
                cpp_vector_push_back(self.ptr, value);
            }
        }
        
        fn get(&self, index: usize) -> Option<i32> {
            danger {
                if index < cpp_vector_size(self.ptr) {
                    Some(cpp_vector_get(self.ptr, index))
                } else {
                    None
                }
            }
        }
    }
}
```

## Raw Pointer Manipulation

### Memory Allocation and Management

```cpp
#[danger(raw_pointers)]
functional class RawMemoryOps {
    fn allocate<T>(count: usize) -> *mut T {
        danger {
            let size = count * std::mem::size_of::<T>();
            let align = std::mem::align_of::<T>();
            
            let layout = Layout::from_size_align(size, align).unwrap();
            let ptr = alloc(layout) as *mut T;
            
            if ptr.is_null() {
                panic!("Out of memory");
            }
            
            ptr
        }
    }
    
    fn deallocate<T>(ptr: *mut T, count: usize) {
        if !ptr.is_null() {
            danger {
                let size = count * std::mem::size_of::<T>();
                let align = std::mem::align_of::<T>();
                
                let layout = Layout::from_size_align(size, align).unwrap();
                dealloc(ptr as *mut u8, layout);
            }
        }
    }
    
    fn copy_memory<T>(src: *const T, dst: *mut T, count: usize) {
        danger {
            std::ptr::copy(src, dst, count);
        }
    }
    
    fn zero_memory<T>(ptr: *mut T, count: usize) {
        danger {
            std::ptr::write_bytes(ptr, 0, count);
        }
    }
}

// Safe wrapper for raw allocation
class RawBuffer<T> {
    ptr: *mut T,
    len: usize,
    capacity: usize,
    constructed_by: RawBufferOps,
}

functional class RawBufferOps<T> {
    fn construct(capacity: usize) -> RawBuffer<T> {
        let ptr = if capacity > 0 {
            RawMemoryOps::allocate::<T>(capacity)
        } else {
            null_mut()
        };
        
        RawBuffer { ptr, len: 0, capacity }
    }
    
    fn destruct(buffer: &mut RawBuffer<T>) {
        // Drop all constructed elements
        for i in 0..buffer.len {
            danger {
                std::ptr::drop_in_place(buffer.ptr.add(i));
            }
        }
        
        // Free the memory
        RawMemoryOps::deallocate(buffer.ptr, buffer.capacity);
        buffer.ptr = null_mut();
        buffer.len = 0;
        buffer.capacity = 0;
    }
    
    fn push(buffer: &mut RawBuffer<T>, value: T) -> Result<()> {
        if buffer.len >= buffer.capacity {
            return Err("Buffer full");
        }
        
        danger {
            std::ptr::write(buffer.ptr.add(buffer.len), value);
        }
        buffer.len += 1;
        Ok(())
    }
    
    fn get(buffer: &RawBuffer<T>, index: usize) -> Option<&T> {
        if index < buffer.len {
            danger {
                Some(&*buffer.ptr.add(index))
            }
        } else {
            None
        }
    }
}
```

### Pointer Arithmetic and Offset Calculations

```cpp
#[danger(raw_pointers)]
functional class PointerArithmetic {
    fn offset_bytes<T>(ptr: *const T, byte_offset: isize) -> *const T {
        danger {
            (ptr as *const u8).offset(byte_offset) as *const T
        }
    }
    
    fn offset_elements<T>(ptr: *const T, element_offset: isize) -> *const T {
        danger {
            ptr.offset(element_offset)
        }
    }
    
    fn pointer_distance<T>(start: *const T, end: *const T) -> isize {
        danger {
            end.offset_from(start)
        }
    }
    
    fn align_pointer<T>(ptr: *const T, alignment: usize) -> *const T {
        danger {
            let addr = ptr as usize;
            let aligned = (addr + alignment - 1) & !(alignment - 1);
            aligned as *const T
        }
    }
}
```

## Thread-Bound Operations

### Single-Threaded Resources

Some resources must remain on specific threads:

```cpp
#[danger(thread_bound)]
class WindowManager {
    windows: HashMap<WindowId, Window>,
    event_loop: EventLoop,
    thread_id: ThreadId,
    
    fn new() -> Result<Self> {
        // Must be created on main thread
        if !is_main_thread() {
            return Err("WindowManager must be created on main thread");
        }
        
        Ok(WindowManager {
            windows: HashMap::new(),
            event_loop: EventLoop::new()?,
            thread_id: current_thread_id(),
        })
    }
    
    fn create_window(&mut self, config: WindowConfig) -> Result<WindowId> {
        self.verify_thread()?;
        
        danger {
            // Platform-specific window creation
            let window = platform::create_window(config)?;
            let id = WindowId::new();
            self.windows.insert(id, window);
            Ok(id)
        }
    }
    
    fn process_events(&mut self) -> Result<Vec<Event>> {
        self.verify_thread()?;
        
        danger {
            platform::poll_events(&mut self.event_loop)
        }
    }
    
    fn verify_thread(&self) -> Result<()> {
        if current_thread_id() != self.thread_id {
            Err("WindowManager accessed from wrong thread")
        } else {
            Ok(())
        }
    }
}
```

## Inline Assembly

### Platform-Specific Optimizations

```cpp
#[danger(inline_assembly)]
functional class AtomicOps {
    fn compare_and_swap_x86_64(ptr: *mut u64, expected: u64, new: u64) -> u64 {
        let result: u64;
        danger {
            asm!(
                "lock cmpxchg {new}, ({ptr})",
                ptr = in(reg) ptr,
                new = in(reg) new,
                inout("rax") expected => result,
                options(nostack, preserves_flags)
            );
        }
        result
    }
    
    fn memory_barrier() {
        danger {
            asm!("mfence", options(nostack, nomem, preserves_flags));
        }
    }
    
    fn rdtsc() -> u64 {
        let low: u32;
        let high: u32;
        danger {
            asm!(
                "rdtsc",
                out("eax") low,
                out("edx") high,
                options(nostack, preserves_flags, readonly)
            );
        }
        ((high as u64) << 32) | (low as u64)
    }
}
```

## Type Transmutation

### Bit-Level Type Conversion

```cpp
#[danger(transmute)]
functional class TypeConversion {
    fn f32_to_u32_bits(value: f32) -> u32 {
        danger {
            std::mem::transmute::<f32, u32>(value)
        }
    }
    
    fn u32_bits_to_f32(bits: u32) -> f32 {
        danger {
            std::mem::transmute::<u32, f32>(bits)
        }
    }
    
    fn slice_to_bytes<T>(slice: &[T]) -> &[u8] {
        danger {
            std::slice::from_raw_parts(
                slice.as_ptr() as *const u8,
                slice.len() * std::mem::size_of::<T>()
            )
        }
    }
    
    fn reinterpret_cast<T, U>(value: &T) -> &U {
        danger {
            &*(value as *const T as *const U)
        }
    }
}

// Safe wrapper for common transmutations
functional class SafeTransmute {
    fn float_bits(value: f32) -> u32 {
        // Safe wrapper that preserves semantics
        TypeConversion::f32_to_u32_bits(value)
    }
    
    fn from_float_bits(bits: u32) -> f32 {
        // Safe wrapper that preserves semantics
        TypeConversion::u32_bits_to_f32(bits)
    }
}
```

## Gradual Migration from C++

### Wrapping Legacy C++ Classes

```cpp
// Existing C++ class
/*
class LegacyProcessor {
public:
    LegacyProcessor(const Config& config);
    ~LegacyProcessor();
    Result process(const Data& input);
    void set_callback(std::function<void(Event)> callback);
private:
    std::unique_ptr<Impl> impl_;
};
*/

// CPrime wrapper
#[danger(ffi, stateful_class)]
class LegacyProcessorWrapper {
    cpp_obj: *mut c_void,  // Opaque pointer to C++ object
    callback_data: Option<Box<dyn Fn(Event)>>,
    
    fn new(config: &Config) -> Result<Self> {
        danger {
            let cpp_obj = ffi::create_legacy_processor(config as *const Config);
            if cpp_obj.is_null() {
                return Err("Failed to create processor");
            }
            
            Ok(LegacyProcessorWrapper {
                cpp_obj,
                callback_data: None,
            })
        }
    }
    
    fn drop(&mut self) {
        if !self.cpp_obj.is_null() {
            danger {
                ffi::destroy_legacy_processor(self.cpp_obj);
            }
            self.cpp_obj = null_mut();
        }
    }
    
    fn process(&self, input: &Data) -> Result<ProcessResult> {
        danger {
            let result = ffi::legacy_processor_process(
                self.cpp_obj,
                input as *const Data
            );
            
            if ffi::is_result_ok(&result) {
                Ok(ffi::extract_result_data(result))
            } else {
                Err(ffi::extract_result_error(result))
            }
        }
    }
    
    fn set_callback<F>(&mut self, callback: F) 
    where F: Fn(Event) + 'static 
    {
        let boxed_callback = Box::new(callback);
        let callback_ptr = &*boxed_callback as *const dyn Fn(Event) as *const c_void;
        
        danger {
            ffi::legacy_processor_set_callback(self.cpp_obj, callback_ptr);
        }
        
        self.callback_data = Some(boxed_callback);
    }
}

// C FFI bridge functions (implemented in C++)
#[danger(ffi)]
extern "C" {
    fn create_legacy_processor(config: *const Config) -> *mut c_void;
    fn destroy_legacy_processor(processor: *mut c_void);
    fn legacy_processor_process(processor: *mut c_void, input: *const Data) -> CResult;
    fn legacy_processor_set_callback(processor: *mut c_void, callback: *const c_void);
    fn is_result_ok(result: *const CResult) -> bool;
    fn extract_result_data(result: CResult) -> ProcessResult;
    fn extract_result_error(result: CResult) -> String;
}
```

## Safety Guidelines

### Danger Block Requirements

All dangerous operations must be wrapped in explicit `danger` blocks:

```cpp
// ✓ Correct - explicit danger block
#[danger(raw_pointers)]
fn allocate_buffer(size: usize) -> *mut u8 {
    danger {
        malloc(size)
    }
}

// ❌ Compiler error - missing danger block
#[danger(raw_pointers)]
fn wrong_allocate(size: usize) -> *mut u8 {
    malloc(size)  // Error: dangerous operation outside danger block
}
```

### Minimizing Danger Scope

Keep dangerous operations as localized as possible:

```cpp
// ✓ Good - minimal danger scope
fn safe_string_from_c_ptr(ptr: *const c_char) -> Result<String> {
    if ptr.is_null() {
        return Err("Null pointer");
    }
    
    let c_str = danger {
        CStr::from_ptr(ptr)  // Only the unsafe operation
    };
    
    c_str.to_str()
        .map(|s| s.to_string())
        .map_err(|_| "Invalid UTF-8")
}

// ❌ Poor - unnecessarily wide danger scope
fn bad_string_from_c_ptr(ptr: *const c_char) -> Result<String> {
    danger {
        if ptr.is_null() {     // Safe operation in danger block
            return Err("Null pointer");
        }
        
        let c_str = CStr::from_ptr(ptr);
        c_str.to_str()         // Safe operation in danger block
            .map(|s| s.to_string())
            .map_err(|_| "Invalid UTF-8")
    }
}
```

### Documentation Requirements

Dangerous code should be thoroughly documented:

```cpp
#[danger(raw_pointers)]
/// Allocates a buffer of the specified size.
/// 
/// # Safety
/// 
/// - The caller must ensure the returned pointer is properly freed
/// - The pointer must not be used after being freed
/// - The size must not be zero
/// 
/// # Panics
/// 
/// Panics if allocation fails (out of memory)
fn allocate_buffer(size: usize) -> *mut u8 {
    assert!(size > 0, "Size must be greater than zero");
    
    danger {
        let ptr = malloc(size);
        if ptr.is_null() {
            panic!("Out of memory");
        }
        ptr
    }
}
```

Danger zones provide the flexibility needed for systems programming while maintaining clear boundaries and encouraging safe abstractions.