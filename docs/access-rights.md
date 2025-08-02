# CPrime Access Rights System

## Overview

CPrime's access rights system provides capability-based security through compile-time access control. Instead of traditional inheritance-based polymorphism, CPrime uses access rights to control what operations are possible on data, providing fine-grained security with zero runtime overhead.

## Core Concepts

### Access Rights Are Not Types

In CPrime, access rights are separate from types. A single data type can have different access rights in different contexts:

```cpp
// One type, different friend access
class SecureData {
    key: [u8; 32],
    value: String,
    timestamp: u64,
    
    // Declares what functional classes can see what fields
    exposes UserOps { value, timestamp }
    exposes AdminOps { key, value, timestamp }
    exposes AuditOps { timestamp }
}
```

### Friend Modules Pattern

Access rights work like C++ friend declarations, but at the module level:

```cpp
// Functional classes with different friend access
functional class UserOps {
    fn read_value(data: &SecureData) -> &str {
        &data.value      // ✓ Can access value
        // data.key      // ❌ Cannot access key
    }
    
    fn get_timestamp(data: &SecureData) -> u64 {
        data.timestamp   // ✓ Can access timestamp
    }
}

functional class AdminOps {
    fn read_key(data: &SecureData) -> &[u8] {
        &data.key        // ✓ Can access key
    }
    
    fn read_value(data: &SecureData) -> &str {
        &data.value      // ✓ Can access value
    }
    
    fn update_key(data: &mut SecureData, new_key: [u8; 32]) {
        data.key = new_key;  // ✓ Can modify key
    }
}

functional class AuditOps {
    fn get_access_time(data: &SecureData) -> u64 {
        data.timestamp   // ✓ Can access timestamp
        // data.value    // ❌ Cannot access value
        // data.key      // ❌ Cannot access key
    }
}
```

## Module Boundaries and Construction

### Constructor Access Control

Modules control who can construct data types and with what access rights:

```cpp
mod database {
    class Connection {
        handle: DbHandle,
        permissions: PermissionSet,
        constructed_by: DatabaseOps,
    }
    
    functional class DatabaseOps {
        fn construct_user_connection(credentials: &UserCreds) -> Result<Connection> {
            let handle = db::connect_user(credentials)?;
            Ok(Connection {
                handle,
                permissions: PermissionSet::USER,
            })
        }
        
        fn construct_admin_connection(admin_key: &AdminKey) -> Result<Connection> {
            let handle = db::connect_admin(admin_key)?;
            Ok(Connection {
                handle,
                permissions: PermissionSet::ADMIN,
            })
        }
    }
    
    // Public factory functions
    pub fn create_user_connection(creds: &UserCreds) -> Result<Connection> {
        DatabaseOps::construct_user_connection(creds)
    }
    
    pub fn create_admin_connection(key: &AdminKey) -> Result<Connection> {
        DatabaseOps::construct_admin_connection(key)
    }
}

// Different access rights based on construction
let user_conn = database::create_user_connection(&user_creds)?;
let admin_conn = database::create_admin_connection(&admin_key)?;

// Same type, different capabilities
UserQueries::execute(&user_conn, "SELECT * FROM public_data");
AdminQueries::execute(&admin_conn, "DELETE FROM sensitive_table");
```

### Module-Level Security

Modules define security boundaries and control access patterns:

```cpp
mod secure_storage {
    // Private implementation details
    class StorageData {
        encrypted_data: Vec<u8>,
        metadata: StorageMetadata,
        constructed_by: SecureOps,
    }
    
    // Internal operations - not exposed
    functional class SecureOps {
        fn construct(data: &[u8], key: &EncryptionKey) -> StorageData {
            let encrypted = encrypt(data, key);
            StorageData {
                encrypted_data: encrypted,
                metadata: StorageMetadata::new(),
            }
        }
        
        fn decrypt_internal(storage: &StorageData, key: &EncryptionKey) -> Vec<u8> {
            decrypt(&storage.encrypted_data, key)
        }
    }
    
    // Public interface - limited access
    pub functional class PublicOps {
        pub fn store(data: &[u8], key: &EncryptionKey) -> StorageData {
            SecureOps::construct(data, key)
        }
        
        pub fn retrieve(storage: &StorageData, key: &EncryptionKey) -> Result<Vec<u8>> {
            verify_access(key)?;
            Ok(SecureOps::decrypt_internal(storage, key))
        }
        
        pub fn get_metadata(storage: &StorageData) -> &StorageMetadata {
            &storage.metadata  // Only metadata accessible publicly
        }
    }
}
```

## Compile-Time vs Runtime Access Control

### Compile-Time (Default)

By default, access rights are checked at compile time with zero runtime cost:

```cpp
// Compile-time access control
let conn: Connection = create_connection();
UserOps::query(&conn, "SELECT ...");     // ✓ Compile-time verified
// AdminOps::delete(&conn);              // ❌ Compile error

// Access rights are compile-time constants
fn process_user_data(conn: &Connection) {
    UserOps::read(&conn);                 // ✓ Always allowed
    // AdminOps::write(&conn);            // ❌ Compile error
}
```

### Runtime (Opt-In)

When needed, access rights can be checked at runtime with dynamic dispatch:

```cpp
// Runtime access control - explicit opt-in
let conn: runtime Connection = create_plugin_connection();

// Dynamic capability checking
if let Some(admin_access) = conn.cast::<AdminOps>() {
    AdminOps::dangerous_operation(&admin_access);
} else {
    println("Insufficient privileges");
}

// Pattern matching on capabilities
match conn.capabilities() {
    Capabilities::User(user_conn) => {
        UserOps::read_data(&user_conn);
    }
    Capabilities::Admin(admin_conn) => {
        AdminOps::modify_system(&admin_conn);
    }
    Capabilities::Audit(audit_conn) => {
        AuditOps::log_access(&audit_conn);
    }
}
```

### Performance Trade-offs

| Access Type | Performance | Flexibility | Use Case |
|-------------|-------------|-------------|----------|
| Compile-time | Zero cost | Static only | Most operations |
| Runtime | Small overhead | Dynamic capabilities | Plugin systems, configuration |

## Advanced Access Patterns

### Access Escalation

Request higher privileges at runtime when needed:

```cpp
functional class EscalationOps {
    fn request_admin_access(user_conn: &Connection, justification: &str) -> Result<AdminConnection> {
        // Check if escalation is allowed
        if can_escalate(user_conn, justification) {
            // Audit the escalation
            audit_log("Access escalation granted", user_conn, justification);
            
            // Return upgraded connection
            Ok(upgrade_to_admin(user_conn))
        } else {
            Err("Escalation denied")
        }
    }
}

// Usage
let user_conn = create_user_connection(&creds)?;
let admin_conn = EscalationOps::request_admin_access(&user_conn, "Emergency fix")?;
AdminOps::emergency_repair(&admin_conn);
```

### Capability Composition

Combine multiple access interfaces for complex operations:

```cpp
// Multiple capabilities required
functional class ComplexOps {
    fn complex_operation(
        db_conn: &Connection,        // Database access
        file_handle: &FileData,      // File system access  
        network: &NetworkData        // Network access
    ) -> Result<()> {
        // Each parameter provides different capabilities
        DatabaseOps::begin_transaction(&db_conn)?;
        FileOps::create_backup(&file_handle)?;
        NetworkOps::notify_completion(&network)?;
        DatabaseOps::commit_transaction(&db_conn)
    }
}

// All capabilities must be provided
let result = ComplexOps::complex_operation(&db_conn, &file, &network);
```

### Time-Limited Access

Capabilities can have temporal constraints:

```cpp
class TemporaryAccess {
    token: AccessToken,
    expires_at: Timestamp,
    constructed_by: TemporaryOps,
}

functional class TemporaryOps {
    fn construct(duration: Duration) -> TemporaryAccess {
        TemporaryAccess {
            token: generate_token(),
            expires_at: now() + duration,
        }
    }
    
    fn execute_with_timeout<T>(
        access: &TemporaryAccess, 
        operation: impl FnOnce() -> T
    ) -> Result<T> {
        if now() > access.expires_at {
            return Err("Access expired");
        }
        
        Ok(operation())
    }
}
```

## Security Model

### Capability Theory Foundation

CPrime's access rights are based on capability security principles:

1. **Unforgeable**: Access rights cannot be created arbitrarily
2. **Transferable**: Access rights can be passed to other functions
3. **Revocable**: Access can be withdrawn (through module control)
4. **Attenuated**: Access rights can be reduced but not increased

### Security Properties

```cpp
// Demonstration of security properties
mod secure_module {
    class ProtectedData {
        secret: Secret,
        constructed_by: ProtectedOps,
    }
    
    functional class ProtectedOps {
        // Only this module can create instances
        fn construct(secret: Secret) -> ProtectedData {
            ProtectedData { secret }
        }
    }
    
    // Public interface cannot access secret directly
    pub functional class PublicInterface {
        // Can only perform authorized operations
        pub fn authorized_operation(data: &ProtectedData) -> PublicResult {
            // Cannot access data.secret directly
            ProtectedOps::internal_operation(data)
        }
    }
}

// External code cannot forge access
// let fake_data = ProtectedData { secret: hacked_secret }; // ❌ Compile error
// let data = secure_module::create(secret);                // ✓ Must use authorized constructor
```

### Audit and Monitoring

Access rights enable comprehensive security auditing:

```cpp
functional class AuditOps {
    fn log_access(operation: &str, data_type: &str, user: &UserContext) {
        audit_log::record(AuditEvent {
            timestamp: now(),
            operation,
            data_type,
            user_id: user.id,
            access_level: user.access_level,
        });
    }
}

// Automatic audit logging
functional class SecureFileOps {
    fn read(file: &FileData, user: &UserContext) -> Result<Vec<u8>> {
        AuditOps::log_access("read", "FileData", user);
        // ... implementation
    }
    
    fn write(file: &mut FileData, data: &[u8], user: &UserContext) -> Result<()> {
        AuditOps::log_access("write", "FileData", user);
        // ... implementation
    }
}
```

## Implementation Strategies

### Compile-Time Implementation

Access rights compile to template parameters or namespace selection:

```cpp
// CPrime code
UserOps::read(&data);

// Generated C++ (conceptual)
namespace UserOps {
    template<typename T>
    requires HasUserAccess<T>
    auto read(const T& data) -> decltype(data.value) {
        return data.value;  // Only accessible fields
    }
}
```

### Runtime Implementation

Runtime access uses vtables or similar dispatch mechanisms:

```cpp
// CPrime code
let conn: runtime Connection = get_connection();
if let Some(admin) = conn.cast::<AdminOps>() {
    AdminOps::operation(&admin);
}

// Generated C++ (conceptual)
class RuntimeConnection {
    std::unique_ptr<ConnectionImpl> impl;
    CapabilitySet capabilities;
    
    template<typename Ops>
    std::optional<typename Ops::AccessType> cast() const {
        if (capabilities.contains(Ops::required_capability())) {
            return Ops::AccessType{impl.get()};
        }
        return std::nullopt;
    }
};
```

## Design Patterns with Access Rights

### Decorator Pattern Replacement

```cpp
// Traditional decorator
class DecoratedStream : public Stream {
    Stream* base;
    // ... decoration logic
};

// CPrime access rights approach
class StreamData {
    handle: FileHandle,
    exposes BasicOps { handle }
    exposes EncryptedOps { handle }
}

functional class BasicOps {
    fn read(data: &StreamData) -> Vec<u8> { /* basic read */ }
}

functional class EncryptedOps {
    fn read(data: &StreamData) -> Vec<u8> { 
        let encrypted = BasicOps::read(data);
        decrypt(encrypted)
    }
}
```

### Strategy Pattern Replacement

```cpp
// Different strategies through different access rights
functional class FastProcessor {
    fn process(data: &ProcessingData) -> Result { /* fast algorithm */ }
}

functional class SecureProcessor {  
    fn process(data: &ProcessingData) -> Result { /* secure algorithm */ }
}

// Strategy selection through module construction
let data = if need_security {
    create_secure_data()   // Returns data with SecureProcessor access
} else {
    create_fast_data()     // Returns data with FastProcessor access
};
```

The access rights system provides powerful security and abstraction capabilities while maintaining zero-cost performance for the common case.