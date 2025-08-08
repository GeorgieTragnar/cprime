# CPrime Library Linking & Extensibility

## Overview

CPrime's N:M composition system creates a unique challenge for library linking: the exponential growth of type combinations between data classes and functional classes. Unlike traditional languages with linear composition patterns, CPrime's revolutionary N:M composition requires sophisticated mechanisms for controlling which combinations are available across library boundaries.

This document outlines CPrime's solution: a principled approach that balances **library control**, **extensibility**, and **compilation efficiency** while acknowledging fundamental physical constraints.

## The N:M Composition Challenge

### Exponential Type Growth

CPrime's composition system naturally creates exponential combinations:

```cpp
// Database library provides
class Connection { /* ... */ }
class QueryBuilder { /* ... */ }
class Transaction { /* ... */ }

// With functional classes
functional class ReadOps { /* ... */ }
functional class WriteOps { /* ... */ }
functional class AdminOps { /* ... */ }
functional class CacheOps { /* ... */ }

// Potential combinations:
Connection<ReadOps>
Connection<WriteOps>
Connection<AdminOps>
Connection<ReadOps, CacheOps>
Connection<WriteOps, CacheOps>
Connection<AdminOps, CacheOps>
QueryBuilder<ReadOps>
QueryBuilder<WriteOps>
// ... and many more
```

### Library Boundary Questions

1. **Which combinations should libraries expose?**
2. **What gets compiled vs instantiated at use-site?**
3. **How do users discover available combinations?**
4. **What about ABI stability across versions?**

## Forward Declaration Solution

### Explicit Library Contracts

Libraries explicitly declare which combinations they support:

```cpp
// database.cprime - Library interface
module Database {
    // Data classes
    class Connection { /* implementation */ }
    class QueryBuilder { /* implementation */ }
    
    // Functional classes
    functional class ReadOps { /* implementation */ }
    functional class WriteOps { /* implementation */ }
    functional class AdminOps { /* implementation */ }
    
    // Explicit combination contracts
    extern template Connection<ReadOps>;
    extern template Connection<WriteOps>;
    extern template Connection<AdminOps>;
    extern template Connection<ReadOps, WriteOps>;
    extern template Connection<AdminOps, WriteOps>;
    
    extern template QueryBuilder<ReadOps>;
    extern template QueryBuilder<WriteOps>;
    // No QueryBuilder<AdminOps> - intentionally unsupported
}
```

### Benefits of Explicit Contracts

1. **Clear API surface**: Exactly which combinations are supported
2. **ABI stability**: Fixed set of symbols in library binary
3. **Discovery mechanism**: Users know what's available
4. **Security boundaries**: Library controls dangerous combinations

## Library Ownership Philosophy

### Core Principle

**"If you own the object definitions, you control combinatorial possibilities"**

### Why Library Control Matters

```cpp
// Database library controls security
module Database {
    class SecureConnection {
        encrypted_handle: EncryptedHandle,
        audit_log: AuditLog,
        constructed_by: ConnectionManager,
    }
    
    // Library chooses which combinations are safe
    extern template SecureConnection<UserOps>;        // ✓ Safe
    extern template SecureConnection<AdminOps>;       // ✓ Safe  
    extern template SecureConnection<AuditBypassOps>; // ✗ Dangerous - not exposed
    extern template SecureConnection<DebugOps>;       // ✗ Internal only
}
```

### Security and Testing Boundaries

1. **Security**: Some combinations might bypass security models
2. **Testing scope**: Libraries only test explicitly supported combinations
3. **Internal patterns**: Some combinations are implementation details
4. **Evolution**: Can add combinations without breaking existing code

## Comptime Metaprogramming

### Problem: Manual Enumeration

Writing all combinations manually becomes unwieldy:

```cpp
// Nobody wants to maintain this by hand:
extern template Connection<ReadOps>;
extern template Connection<WriteOps>;
extern template Connection<AdminOps>;
extern template Connection<ReadOps, CacheOps>;
extern template Connection<WriteOps, CacheOps>;
extern template Connection<AdminOps, CacheOps>;
extern template Connection<ReadOps, WriteOps>;
extern template Connection<ReadOps, AdminOps>;
extern template Connection<WriteOps, AdminOps>;
extern template Connection<ReadOps, WriteOps, CacheOps>;
extern template Connection<ReadOps, AdminOps, CacheOps>;
extern template Connection<WriteOps, AdminOps, CacheOps>;
extern template Connection<ReadOps, WriteOps, AdminOps>;
extern template Connection<ReadOps, WriteOps, AdminOps, CacheOps>;
// ... exponential explosion
```

### Solution: Programmatic Generation

```cpp
module Database {
    comptime {
        // Define categories of operations
        const base_ops = [ReadOps, WriteOps, AdminOps];
        const cache_ops = [CacheOps, NoCache];
        const logging_ops = [AuditLog, QuietMode];
        
        // Generate valid combinations programmatically
        for (base in base_ops) {
            // Single operation combinations
            extern template Connection<{base}>;
            
            for (cache in cache_ops if cache != NoCache) {
                // Base + cache combinations
                extern template Connection<{base}, {cache}>;
                
                for (log in logging_ops if base == AdminOps || log != AuditLog) {
                    // Base + cache + logging (audit only for admin)
                    extern template Connection<{base}, {cache}, {log}>;
                }
            }
        }
        
        // Custom rules for special cases
        if (FEATURE_DEBUG_BUILD) {
            extern template Connection<DebugOps>;
        }
    }
}
```

### Advanced Generation Patterns

```cpp
comptime {
    // Rule-based generation
    for (ops_combo in generate_combinations(base_ops)) {
        if (is_valid_security_combination(ops_combo)) {
            extern template Connection<{...ops_combo}>;
        }
    }
    
    // Hierarchical patterns
    const access_levels = [User, PowerUser, Admin, SuperAdmin];
    for (level in access_levels) {
        for (ops in get_allowed_operations(level)) {
            extern template Connection<{level}, {ops}>;
        }
    }
    
    // Conditional compilation
    #[cfg(feature = "advanced_analytics")]
    for (base in base_ops) {
        extern template Connection<{base}, AnalyticsOps>;
    }
}
```

## The Extension Problem & 2×2 Matrix

### Core Tension

Users want to add their own functional classes to library data classes:

```cpp
// User-defined functionality
functional class CustomAnalyticsOps {
    fn analyze_query_patterns(conn: &Connection, timeframe: Duration) -> Report;
}

// User wants this to work:
let conn: Connection<CustomAnalyticsOps> = ConnectionManager::construct(config);
// But library didn't precompile this combination!
```

### The 2×2 Extension Matrix

Each interface can declare its extensibility with a 2×2 matrix:

```cpp
interface DatabaseOps {
    #[extension(data_closed, access_closed)]  // Fully sealed
    fn execute_query(&self, sql: &str) -> Result<QueryResult>;
}

interface PluggableAnalytics {
    #[extension(data_open, access_closed)]    // Classic interface pattern
    fn generate_report(&self, data: &AnalysisData) -> Report;
}

interface ExtensibleConnection {
    #[extension(data_closed, access_open)]    // New operations on fixed data
    fn get_connection_info(&self) -> ConnectionInfo;
}

interface FullProtocol {
    #[extension(data_open, access_open)]      // Complete extensibility
    fn process_request(&mut self, request: &Request) -> Response;
}
```

### Four Extension Modes

| Mode | Data | Access | Description | User Can |
|------|------|--------|-------------|----------|
| `[closed, closed]` | Fixed | Fixed | Sealed library | Use only |
| `[data_open, closed]` | Extensible | Fixed | Classic interface | Implement interface |
| `[closed, access_open]` | Fixed | Extensible | New operations | Add operations |
| `[open, open]` | Extensible | Extensible | Full protocol | Extend everything |

## The Impossible Triangle

### Fundamental Constraint

There's an **Impossible Triangle** in library design - you can only have 2 of 3:

```
       Zero-Cost Composition
              /\
             /  \
            /    \
           /      \
          /________\
    Compiled       Open
    Libraries    Extensibility
```

### Why It's Impossible

1. **New compositions need access to private fields** for zero-cost composition
2. **Compiled libraries have fixed symbols** - can't generate new instantiations
3. **Can't magically create template instantiations from binary code**

This is a fundamental **physical constraint**, not a design limitation.

### Resolution Options

| Option | Trade-off | When to Use |
|--------|-----------|-------------|
| **Ship headers** | Compilation time | Maximum flexibility |
| **Runtime dispatch** | Performance cost | Dynamic composition needs |
| **Restrict to public API** | Composition limitations | Simple extensions |
| **Whole-program compilation** | Build complexity | Performance-critical |

## Distribution Strategies

### Extension Mode → Distribution Mapping

```cpp
libdatabase/
  bin/
    database.so          // Pre-compiled common combinations
  include/
    database.h           // Public API always included
    extensible_ops.h     // Only if access_open
    connection_data.h    // Only if data_open
    // internal.h        // Never shipped if closed
```

| Extension Mode | Binary | Headers Shipped | User Capability |
|----------------|--------|-----------------|-----------------|
| `[closed, closed]` | ✓ | Public API only | Use pre-compiled combinations |
| `[data_open, closed]` | ✓ | Public API + data class headers | Implement interfaces |
| `[closed, access_open]` | ✓ | Public API + operation headers | Add new operations |
| `[open, open]` | ✓ | Full header set | Complete extensibility |

### Compilation Models

#### Fast Path: Pre-compiled Combinations
```cpp
// Uses library's compiled binary
let conn: Connection<ReadOps> = Database::connect(config);  // ✓ Fast link
```

#### Extension Path: Header-Only Compilation
```cpp
// Requires headers and compilation
let conn: Connection<CustomOps> = Database::connect(config);  // ✓ Slower compile
```

#### Mixed Usage
```cpp
// Both in same program
let standard_conn: Connection<ReadOps> = Database::connect(config);      // Fast
let custom_conn: Connection<MyCustomOps> = Database::connect(config);    // Compiled
```

## Implementation Examples

### Closed Library Pattern

```cpp
// Fully controlled library
module SecureDatabase {
    #[extension(data_closed, access_closed)]
    interface SecureOps {
        fn authenticate(&self, credentials: &Credentials) -> AuthResult;
        fn execute_secure_query(&self, query: &str) -> Result<QueryResult>;
    }
    
    // Only these combinations exist
    comptime {
        extern template SecureConnection<UserSecureOps>;
        extern template SecureConnection<AdminSecureOps>;
        // No custom security operations allowed
    }
}
```

### Extensible Operations Pattern

```cpp
// Fixed data, extensible operations
module MonitoringDatabase {
    class MonitoredConnection {
        handle: DbHandle,
        metrics: MetricsCollector,
        constructed_by: MonitoringManager,
    }
    
    #[extension(data_closed, access_open)]
    interface MonitoringOps {
        fn collect_metrics(&self) -> Metrics;
    }
    
    // Users can add new monitoring operations
    // Ships: MonitoredConnection definition + operation interface headers
}

// User extension
functional class CustomMetricsOps implements MonitoringOps {
    fn collect_metrics(conn: &MonitoredConnection) -> Metrics {
        // Custom metrics collection
    }
    
    fn generate_custom_report(conn: &MonitoredConnection) -> CustomReport {
        // New operation not in original interface
    }
}
```

### Full Protocol Pattern

```cpp
// Complete extensibility
module PluggableDatabase {
    #[extension(data_open, access_open)]
    interface DatabaseProtocol {
        fn connect(&self, config: &Config) -> Result<Self::Connection>;
        fn query(&self, conn: &mut Self::Connection, sql: &str) -> Result<QueryResult>;
    }
    
    // Ships: Full headers, pre-compiled common cases for performance
}

// User can implement completely custom database backends
class MyCustomConnection {
    custom_handle: MyHandle,
    constructed_by: MyConnectionManager,
}

functional class MyConnectionOps implements DatabaseProtocol {
    type Connection = MyCustomConnection;
    
    fn connect(config: &Config) -> Result<MyCustomConnection> { /* ... */ }
    fn query(conn: &mut MyCustomConnection, sql: &str) -> Result<QueryResult> { /* ... */ }
}
```

## Best Practices

### For Library Authors

1. **Start closed, open gradually**: Begin with `[closed, closed]`, open based on user needs
2. **Use comptime for common patterns**: Avoid manual combination enumeration
3. **Document extension points clearly**: Make extension modes explicit
4. **Provide migration paths**: Newer versions can be more open, never more closed
5. **Test what you ship**: Only expose thoroughly tested combinations

### For Library Users

1. **Prefer pre-compiled combinations**: Use library-provided combinations when possible
2. **Understand compilation costs**: Custom combinations require header compilation
3. **Check extension modes**: Verify what kind of extensions are supported
4. **Follow security boundaries**: Respect library's access control decisions

### For Ecosystem

1. **Embrace the C++ model**: Header distribution for extensibility is proven
2. **Make trade-offs explicit**: Document performance vs flexibility choices
3. **Provide good tooling**: Help with discovery and compilation
4. **Don't fight physics**: Accept the Impossible Triangle constraint

## Integration with CPrime Features

### With Interface Memory Contracts

```cpp
interface Cacheable {
    #[extension(data_open, access_closed)]
    memory_contract {
        id: u64,
        timestamp: u64,
        hash: u32,
    }
}

// Library ships: interface definition
// Users can: implement on their data classes
// Users cannot: add new contract fields
```

### With Access Rights

```cpp
class ExtensibleData {
    core_data: CoreFields,
    
    #[extension(data_closed, access_open)]
    exposes CoreOps { core_data }
    
    constructed_by: DataManager,
}

// Users can add new functional classes that work with CoreOps
// But cannot modify CoreFields
```

### With Semconst Fields

```cpp
class ConfigurableService {
    semconst config: ServiceConfig,
    mutable runtime_state: RuntimeState,
    
    #[extension(data_closed, access_open)]
    exposes ServiceOps { config as const, runtime_state as mutable }
}

// Extensions can read config, modify runtime_state
// But cannot replace config (semconst + const exposure)
```

## Migration and Evolution

### Version Evolution Rules

```cpp
// v1.0 - Closed
interface DatabaseOps {
    #[extension(data_closed, access_closed)]
    fn basic_query(&self, sql: &str) -> QueryResult;
}

// v2.0 - Can open up (backward compatible)
interface DatabaseOps {
    #[extension(data_closed, access_open)]  // More permissive
    fn basic_query(&self, sql: &str) -> QueryResult;
}

// v3.0 - Cannot close down (breaking change)
// #[extension(data_closed, access_closed)]  // ❌ Breaking change
```

### Migration Patterns

1. **Adding combinations**: Always safe (more pre-compiled options)
2. **Opening extension modes**: Safe (enables new use cases)
3. **Closing extension modes**: Breaking change (removes capabilities)
4. **Changing data classes**: Version as new types

## Conclusion

CPrime's library linking solution acknowledges that N:M composition creates genuine complexity that cannot be magically resolved. Instead, it provides:

1. **Explicit control mechanisms**: Libraries control combinatorial explosion
2. **Principled extensibility**: 2×2 matrix makes trade-offs clear
3. **Familiar patterns**: Builds on proven C++ template model
4. **Physical reality acceptance**: Embraces the Impossible Triangle

The result is a system that makes complexity explicit, gives fine-grained control to library authors, and provides clear guidance for users while maintaining CPrime's zero-cost abstraction principles where possible.

By not trying to solve unsolvable problems and instead making trade-offs transparent, CPrime enables powerful composition patterns while maintaining the clarity and control that systems programming requires.