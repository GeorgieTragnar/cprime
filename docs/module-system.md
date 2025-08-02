# CPrime Module System

## Overview

CPrime's module system provides namespace organization, access control, and code reuse mechanisms. Modules serve as security boundaries, compilation units, and the foundation for the access rights system.

## Module Declaration and Structure

### Basic Module Declaration

```cpp
// Simple module declaration
mod network {
    // Module-private items
    fn internal_connect(addr: &str) -> Result<Socket> {
        // Implementation details hidden
    }
    
    // Public interface
    pub fn connect(addr: &str) -> Result<Connection> {
        let socket = internal_connect(addr)?;
        Ok(Connection::from_socket(socket))
    }
}

// Usage
let conn = network::connect("127.0.0.1:8080")?;
```

### Module Attributes

Modules can be annotated with attributes that control their behavior:

```cpp
// Sealed module - cannot be extended
#[module(sealed)]
mod crypto {
    // Implementation is final, no external extensions allowed
    pub fn hash(data: &[u8]) -> Hash { ... }
    pub fn encrypt(data: &[u8], key: &Key) -> Vec<u8> { ... }
}

// Open module - can be extended with additional functionality
#[module(open)]
mod serializers {
    pub fn to_json<T: Serialize>(value: &T) -> String { ... }
    pub fn from_json<T: Deserialize>(json: &str) -> Result<T> { ... }
}

// Version information
#[module(version = "1.2.3")]
mod stable_api {
    // API with versioning guarantees
}

// Platform-specific modules
#[module(target = "linux")]
mod linux_specific {
    pub fn get_pid() -> u32 { ... }
}

#[module(target = "windows")]  
mod windows_specific {
    pub fn get_pid() -> u32 { ... }
}
```

## Module Equivalence with Functional Classes

### Syntactic Sugar

Modules without constructors are equivalent to functional classes:

```cpp
// These declarations are equivalent:

// Module syntax
mod StringOps {
    fn split(s: &str, delimiter: char) -> Vec<&str> { ... }
    fn join(parts: &[&str], separator: &str) -> String { ... }
    fn trim(s: &str) -> &str { ... }
}

// Functional class syntax  
functional class StringOps {
    fn split(s: &str, delimiter: char) -> Vec<&str> { ... }
    fn join(parts: &[&str], separator: &str) -> String { ... }
    fn trim(s: &str) -> &str { ... }
}

// Usage is identical
let words = StringOps::split("hello,world", ',');
let joined = StringOps::join(&words, " ");
```

### When to Use Which Syntax

| Use Module Syntax | Use Functional Class Syntax |
|------------------|----------------------------|
| Utility functions | Operations on specific data types |
| Library APIs | Data manipulation methods |
| Mathematical operations | Business logic operations |
| System interfaces | Type-specific algorithms |

```cpp
// Good use of module syntax - utility functions
mod math {
    pub fn gcd(a: i32, b: i32) -> i32 { ... }
    pub fn lcm(a: i32, b: i32) -> i32 { ... }
    pub fn factorial(n: u32) -> u64 { ... }
}

// Good use of functional class - operations on specific type
class FileData {
    path: PathBuf,
    handle: FileHandle,
    constructed_by: FileOps,
}

functional class FileOps {
    fn construct(path: &Path) -> Result<FileData> { ... }
    fn read_line(file: &mut FileData) -> Result<String> { ... }
    fn write_line(file: &mut FileData, line: &str) -> Result<()> { ... }
}
```

## Module Hierarchy and Organization

### Nested Modules

```cpp
mod network {
    // Nested module for HTTP-specific functionality
    pub mod http {
        class RequestData {
            url: String,
            headers: HashMap<String, String>,
            body: Vec<u8>,
            constructed_by: RequestOps,
        }
        
        pub functional class RequestOps {
            pub fn construct(url: &str) -> RequestData { ... }
            pub fn add_header(req: &mut RequestData, key: &str, value: &str) { ... }
            pub fn set_body(req: &mut RequestData, body: Vec<u8>) { ... }
        }
    }
    
    // Nested module for WebSocket functionality
    pub mod websocket {
        pub functional class WebSocketOps {
            pub fn connect(url: &str) -> Result<WebSocketData> { ... }
            pub fn send_message(ws: &mut WebSocketData, msg: &str) -> Result<()> { ... }
        }
    }
    
    // Module-level functionality
    pub fn resolve_hostname(hostname: &str) -> Result<IpAddr> { ... }
}

// Usage with full paths
let request = network::http::RequestOps::construct("https://api.example.com");
let ws = network::websocket::WebSocketOps::connect("ws://localhost:8080")?;
let ip = network::resolve_hostname("example.com")?;
```

### Module Re-exports

```cpp
mod internal {
    pub fn implementation_detail() -> i32 { 42 }
    
    pub mod deep {
        pub fn nested_function() -> String { "hidden".to_string() }
    }
}

// Re-export for cleaner public API
pub use internal::implementation_detail as public_function;
pub use internal::deep::nested_function;

// Glob re-exports
pub use internal::*;

// Renamed re-exports
pub use internal::deep::nested_function as utility_fn;
```

## Module Access Control and Security

### Access Control Levels

```cpp
mod secure_storage {
    // Private - only accessible within this module
    fn encrypt_internal(data: &[u8], key: &[u8]) -> Vec<u8> { ... }
    
    // Module-public - accessible to parent module
    pub(super) fn debug_info() -> String { ... }
    
    // Crate-public - accessible throughout the crate
    pub(crate) fn internal_api() -> InternalData { ... }
    
    // Public - accessible to external crates
    pub fn public_encrypt(data: &[u8]) -> Vec<u8> {
        let key = get_encryption_key();
        encrypt_internal(data, &key)
    }
}
```

### Friend Relationships

Modules can declare friendship for privileged access:

```cpp
mod database {
    class ConnectionData {
        handle: DbHandle,
        credentials: Credentials,
        
        // Grant access to specific modules
        exposes ConnectionPool { handle }
        exposes QueryEngine { handle, credentials }
    }
    
    // Only friends can access private data
    friend mod connection_pool;
    friend mod query_engine;
    
    pub fn create_connection(creds: Credentials) -> ConnectionData {
        ConnectionData {
            handle: db::connect(&creds),
            credentials: creds,
        }
    }
}

// Friend module with special access
friend mod connection_pool {
    use super::database::ConnectionData;
    
    fn recycle_connection(conn: &ConnectionData) {
        // Can access handle through friendship
        db::reset_connection(conn.handle);
    }
}
```

## Module Extension and Composition

### Extension for Open Modules

```cpp
#[module(open)]
mod base_serializers {
    pub fn to_json<T: Serialize>(value: &T) -> String { ... }
    pub fn from_json<T: Deserialize>(json: &str) -> Result<T> { ... }
}

// Extend the open module
extend mod base_serializers {
    // Add new functionality
    pub fn to_yaml<T: Serialize>(value: &T) -> String { ... }
    pub fn from_yaml<T: Deserialize>(yaml: &str) -> Result<T> { ... }
    
    // Add specialized implementations
    pub fn to_compact_json<T: Serialize>(value: &T) -> String {
        to_json(value).replace(" ", "").replace("\n", "")
    }
}

// Usage includes extended functionality
let json = base_serializers::to_json(&data);
let yaml = base_serializers::to_yaml(&data);  // Extended function
let compact = base_serializers::to_compact_json(&data);  // Extended function
```

### Module Composition

```cpp
// Compose functionality from multiple modules
mod http_client {
    use network::tcp;
    use serializers::json;
    use auth::oauth;
    
    class HttpClientData {
        tcp_conn: tcp::ConnectionData,
        auth_token: oauth::TokenData,
        constructed_by: HttpClientOps,
    }
    
    pub functional class HttpClientOps {
        pub fn construct(url: &str, token: oauth::TokenData) -> Result<HttpClientData> {
            let conn = tcp::TcpOps::connect(url)?;
            Ok(HttpClientData {
                tcp_conn: conn,
                auth_token: token,
            })
        }
        
        pub fn get<T: Deserialize>(
            client: &HttpClientData, 
            path: &str
        ) -> Result<T> {
            let request = build_request(path, &client.auth_token);
            let response = tcp::TcpOps::send_request(&client.tcp_conn, request)?;
            json::from_json(&response)
        }
    }
}
```

## Module Compilation and Linking

### Compilation Units

Each module can be compiled separately:

```cpp
// File: src/network/mod.cp
mod network {
    pub mod tcp;     // Refers to src/network/tcp.cp
    pub mod http;    // Refers to src/network/http.cp
    pub mod tls;     // Refers to src/network/tls.cp
    
    // Module-level exports
    pub use tcp::TcpConnection;
    pub use http::HttpClient;
    pub use tls::TlsConfig;
}

// File: src/network/tcp.cp
// Automatically part of parent network module
class TcpConnectionData {
    socket: Socket,
    constructed_by: TcpOps,
}

pub functional class TcpOps {
    pub fn connect(addr: &str) -> Result<TcpConnectionData> { ... }
    pub fn send(conn: &TcpConnectionData, data: &[u8]) -> Result<()> { ... }
    pub fn recv(conn: &TcpConnectionData, buf: &mut [u8]) -> Result<usize> { ... }
}
```

### Module Dependencies

```cpp
// Explicit dependency declaration
#[module(depends = ["network", "serializers", "crypto"])]
mod secure_client {
    use network::tls;
    use serializers::json;
    use crypto::aes;
    
    // Implementation using dependencies
}

// Version constraints
#[module(depends = [
    "network >= 1.0, < 2.0",
    "crypto ^1.2.0",
    "utils ~0.5.2"
])]
mod versioned_module {
    // Module with specific version requirements
}
```

## Standard Library Modules

### Core Modules

```cpp
// Memory management utilities
mod memory {
    pub fn allocate<T>(count: usize) -> *mut T { ... }
    pub fn deallocate<T>(ptr: *mut T, count: usize) { ... }
    pub fn align_to<T>(size: usize, align: usize) -> usize { ... }
}

// Collection types
mod collections {
    pub mod vector;
    pub mod hash_map;
    pub mod linked_list;
    pub mod binary_tree;
}

// I/O operations
mod io {
    pub mod file;
    pub mod network;
    pub mod console;
    
    pub trait Read {
        fn read(&mut self, buf: &mut [u8]) -> Result<usize>;
    }
    
    pub trait Write {
        fn write(&mut self, buf: &[u8]) -> Result<usize>;
    }
}

// Concurrency primitives
mod sync {
    pub mod channel;
    pub mod mutex;
    pub mod semaphore;
    pub mod atomic;
}
```

### Platform-Specific Modules

```cpp
// Conditional compilation based on target
#[cfg(target_os = "linux")]
mod os {
    pub fn get_process_id() -> u32 {
        unsafe { libc::getpid() as u32 }
    }
}

#[cfg(target_os = "windows")]
mod os {
    pub fn get_process_id() -> u32 {
        unsafe { winapi::um::processthreadsapi::GetCurrentProcessId() }
    }
}

// Architecture-specific optimizations
#[cfg(target_arch = "x86_64")]
mod simd {
    pub fn vectorized_add(a: &[f32], b: &[f32]) -> Vec<f32> {
        // Use AVX instructions
    }
}

#[cfg(not(target_arch = "x86_64"))]
mod simd {
    pub fn vectorized_add(a: &[f32], b: &[f32]) -> Vec<f32> {
        // Fallback implementation
    }
}
```

## Module Testing and Documentation

### Test Modules

```cpp
mod string_utils {
    pub fn reverse(s: &str) -> String {
        s.chars().rev().collect()
    }
    
    pub fn is_palindrome(s: &str) -> bool {
        let cleaned: String = s.chars()
            .filter(|c| c.is_alphanumeric())
            .map(|c| c.to_lowercase().next().unwrap())
            .collect();
        cleaned == reverse(&cleaned)
    }
    
    #[cfg(test)]
    mod tests {
        use super::*;
        
        #[test]
        fn test_reverse() {
            assert_eq!(reverse("hello"), "olleh");
            assert_eq!(reverse(""), "");
        }
        
        #[test]
        fn test_palindrome() {
            assert!(is_palindrome("A man, a plan, a canal: Panama"));
            assert!(!is_palindrome("not a palindrome"));
        }
    }
}
```

### Documentation

```cpp
/// Network communication utilities
/// 
/// This module provides high-level abstractions for network communication,
/// including TCP connections, HTTP clients, and WebSocket support.
/// 
/// # Examples
/// 
/// ```cpp
/// let client = network::http::Client::new();
/// let response = client.get("https://api.example.com/data").await?;
/// ```
mod network {
    /// Establishes a TCP connection to the specified address
    /// 
    /// # Arguments
    /// 
    /// * `addr` - The address to connect to (e.g., "127.0.0.1:8080")
    /// 
    /// # Returns
    /// 
    /// Returns a `Result` containing the connection data on success
    /// 
    /// # Errors
    /// 
    /// This function will return an error if:
    /// - The address is invalid
    /// - The connection cannot be established
    /// - Network permissions are insufficient
    pub fn connect(addr: &str) -> Result<ConnectionData> {
        // Implementation
    }
}
```

The module system provides clear organization, security boundaries, and code reuse mechanisms while maintaining the performance characteristics essential for systems programming.