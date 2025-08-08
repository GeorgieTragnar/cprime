# CPrime Versioning System

## Overview

CPrime uses a comprehensive versioning system designed to provide strong compatibility guarantees while enabling independent evolution of compiler, standard library, and user libraries. The system balances stability for production use with flexibility for rapid development and innovation.

Key principles:
- **Component independence**: Compiler, std library, and libraries can evolve separately
- **ABI stability**: Strong binary compatibility within major versions  
- **Clear migration paths**: Predictable upgrade strategies
- **Developer control**: Flexible version specification and resolution

## Version Number Structure

### Format: X.Y.Z (Semantic Versioning)

```
X = Major version (breaking changes, ABI incompatible)
Y = Minor version (backward compatible features)
Z = Patch version (backward compatible fixes only)
```

### Version Progression Examples

```
Development Phase:
0.0.1 → 0.0.2 → ... → 0.0.99  (Early development)
0.1.0                           (First major milestone)
0.1.1 → 0.1.2 → ... → 0.9.99   (Pre-production)

Production Phase:
1.0.0                           (First production release)
1.0.1 → 1.0.2 → ... → 1.99.99  (Stable production)
2.0.0                           (Next major release)
```

### Component Independence

CPrime components can version independently while maintaining compatibility:

```toml
# Example system state
compiler: 1.3.5    # Compiler version
std:      1.2.8    # Standard library version  
library:  1.7.2    # User library version

# All are compatible because major version = 1
```

## Compatibility Rules

### Major Version Coupling

Components must match on major version for ABI compatibility:

```
Compiler 1.x.x ↔ Std 1.x.x ↔ Libraries 1.x.x
         ↕ Binary compatible ↕
Compiler 2.x.x ↔ Std 2.x.x ↔ Libraries 2.x.x
```

**Valid combinations:**
```toml
# ✓ Valid - same major version
compiler = "1.6.4"
std = "1.2.0"      # Older minor version OK
std = "1.8.3"      # Newer minor version OK
std = "1.99.99"    # Any 1.x.x compatible

# ✓ Valid - all components match major
database_lib = "1.4.2"  # Built with compiler 1.x
network_lib = "1.0.5"   # Also built with compiler 1.x
```

**Invalid combinations:**
```toml
# ✗ Invalid - major version mismatch
compiler = "1.6.4"
std = "2.0.0"      # Different major version

# ✗ Invalid - library/compiler mismatch  
compiler = "1.6.4"
database_lib = "2.1.0"  # Built with compiler 2.x
```

### Minor and Patch Version Freedom

Within a major version, minor and patch versions provide flexibility:

```cpp
// Minor version additions (backward compatible)
// Version 1.2.0 adds:
- New standard library functions
- Additional compiler optimizations  
- New language features (contextual keywords)
- Enhanced error messages
- Performance improvements

// Patch version fixes (fully compatible)
// Version 1.2.1 includes:
- Bug fixes
- Security patches  
- Documentation updates
- Performance improvements (same ABI)
```

### Library Compatibility Matrix

```
Library built with compiler: 1.1.3
Can link with any compiler:  1.x.x series
Cannot link with compiler:   2.x.x series

Example:
- Library "database-1.4.2" built with compiler 1.1.3
- Can be used with compiler 1.0.0, 1.5.7, 1.99.99
- Cannot be used with compiler 2.0.0
```

## ABI Stability Guarantees

### Frozen Within Major Version

These elements **CANNOT** change within a major version (1.x.x):

```cpp
// Memory layout - FROZEN
class DataStructure {
    field1: i32,    // Offset 0
    field2: f64,    // Offset 8  
    field3: String, // Offset 16
    // Field order and padding CANNOT change
}

// Function signatures - FROZEN
fn public_api_function(param: i32) -> String;
// Calling convention, parameter order CANNOT change

// Name mangling - FROZEN
// Symbol names in compiled libraries CANNOT change

// Exception/signal ABI - FROZEN
// Signal propagation mechanism CANNOT change

// Coroutine frame layout - FROZEN
// Stack frame structure CANNOT change
```

### Allowed Minor Version Changes

These elements **CAN** be added in minor versions:

```cpp
// New functions (backward compatible)
// Version 1.3.0 can add:
impl DataStructure {
    // ✓ New methods OK
    fn new_functionality(&self) -> Result<()> { ... }
}

// New language features (contextual)
// Version 1.4.0 can add:
defer cleanup();  // New contextual keyword

// Additional optimizations
// Version 1.2.0 can add:
- Improved coroutine performance
- Better memory layout optimization
- Enhanced compile-time analysis

// New standard library modules
mod new_utilities {  // Added in 1.5.0
    pub fn helper_function() { ... }
}
```

### Patch Version Limitations

Patch versions have the most restrictions:

```cpp
// ONLY allowed in patch versions:
// 1. Bug fixes that don't change behavior
fn fixed_function(input: i32) -> i32 {
    // Before (1.2.0): Wrong calculation
    // input * 2 + 1  // Bug!
    
    // After (1.2.1): Correct calculation  
    input * 3 + 1    // Fix maintains compatibility
}

// 2. Security patches
fn secure_operation(data: &[u8]) -> Result<()> {
    // Added bounds checking (1.2.1)
    // Maintains same API signature
}

// 3. Performance improvements (same ABI)
fn optimized_function(input: &Data) -> Output {
    // Better algorithm, same interface (1.2.1)
}

// NOT allowed in patch versions:
// - New functions
// - Changed signatures  
// - Different behavior
// - ABI changes
```

## Pre-Production Versioning (0.x.x)

### Development Phases

CPrime development follows structured phases during 0.x.x:

```
Phase 1: Core Language (0.0.x)
├── 0.0.1: Lexer and parser basics
├── 0.0.2: Data and functional classes
├── 0.0.3: Access rights system
├── 0.0.4: Basic code generation
└── 0.0.5: Three-class system integration

Phase 2: First Milestone (0.1.x) 
├── 0.1.0: Coroutines implementation (BREAKING)
├── 0.1.1: Channel system
├── 0.1.2: Runtime trait architecture
└── 0.1.3: Memory management refinements

Phase 3: Second Milestone (0.2.x)
├── 0.2.0: Signal handling system (BREAKING)
├── 0.2.1: Defer mechanism
├── 0.2.2: Union types
└── 0.2.3: Interface memory contracts

Phase 4: Advanced Features (0.3.x)
├── 0.3.0: N:M composition (BREAKING)
├── 0.3.1: Library linking system
├── 0.3.2: Compilation optimization
└── 0.3.3: C++ compatibility layer

Phase 5: Release Candidates (0.9.x)
├── 0.9.0: Feature freeze, stabilization focus
├── 0.9.1: RC1 - API stability testing
├── 0.9.2: RC2 - Performance optimization
└── 0.9.9: Final RC - production readiness
```

### Breaking Change Freedom

During 0.x.x development, minor versions **CAN** introduce breaking changes:

```toml
# Breaking changes allowed between these
0.1.x → 0.2.0  # Signal handling redesign
0.2.x → 0.3.0  # N:M composition changes
0.3.x → 0.4.0  # ABI modifications allowed

# Patch versions remain compatible
0.1.1 → 0.1.2  # Must be compatible
0.2.1 → 0.2.2  # Must be compatible
```

**Example breaking change progression:**
```cpp
// Version 0.1.x
fn process_data(data: &Data) -> Result<Output> {
    // Original implementation
}

// Version 0.2.0 (breaking change allowed)
fn process_data(data: &Data, options: ProcessOptions) -> Result<Output> {
    // Added required parameter - breaking change
    // This is OK in 0.x.x minor version
}

// Version 0.2.1 (must be compatible with 0.2.0)
fn process_data(data: &Data, options: ProcessOptions) -> Result<Output> {
    // Bug fix only - same signature required
}
```

## Production Versioning (1.0.0+)

### Release Cadence

```
Major versions: Every 3-5 years
├── Significant language evolution
├── ABI breaking changes
├── Major architectural improvements
└── Ecosystem-wide coordination

Minor versions: Every 3-6 months  
├── New backward-compatible features
├── Standard library additions
├── Performance improvements
└── Tool enhancements

Patch versions: As needed
├── Critical bug fixes
├── Security patches
├── Documentation updates
└── Emergency fixes
```

### Support Lifecycle

```
Current:  1.3.x (Latest features, active development)
├── New features and optimizations
├── Latest language improvements
├── Cutting-edge tooling
└── Community focus

LTS:      1.0.x (Long-term support, 5 years)
├── Security patches only
├── Critical bug fixes
├── Stability guarantee
└── Enterprise focus

Legacy:   0.9.x (Security patches, 2 years)
├── Security vulnerabilities only
├── No feature additions
├── Minimal maintenance
└── Migration assistance

Preview:  2.0-beta (Next major preview)
├── Breaking changes preview
├── Community testing
├── Migration preparation
└── Feedback collection
```

### Version Channels

```toml
[channels]
stable = "1.3.5"        # Production ready
lts = "1.0.23"          # Long-term support
beta = "1.4.0-beta2"    # Next minor preview  
nightly = "2.0-dev"     # Next major development
experimental = "2.1-alpha"  # Experimental features
```

**Channel selection:**
```bash
# Install specific channel
cprime install --channel stable
cprime install --channel lts
cprime install --channel beta

# Project channel specification
echo 'channel = "lts"' >> cprime.toml
```

## Dependency Resolution

### Project Version Specification

```toml
[project]
name = "web_service"
version = "1.2.3"
compiler = "1.3"        # Minimum 1.3.0, maximum < 2.0.0

[dependencies]
std = "~1.3"            # 1.3.x series only
database = "^1.2"       # >= 1.2.0, < 2.0.0
network = "=1.4.2"      # Exactly version 1.4.2
logging = ">=1.1.0"     # At least 1.1.0
experimental = "2.0-beta"  # Pre-release versions
```

**Version specifier meanings:**
```toml
"1.2"       # >= 1.2.0, < 2.0.0
"~1.2"      # >= 1.2.0, < 1.3.0 (compatible within minor)
"^1.2"      # >= 1.2.0, < 2.0.0 (compatible within major)  
"=1.2.3"    # Exactly 1.2.3
">=1.2.0"   # At least 1.2.0
"<2.0.0"    # Less than 2.0.0
```

### Resolution Algorithm

The dependency resolver follows this process:

```
1. Parse version constraints for all dependencies
   ├── Direct project dependencies
   ├── Transitive dependencies  
   └── Compiler/std requirements

2. Find highest compatible std library version
   ├── Must match compiler major version
   ├── Satisfy all library requirements
   └── Prefer latest compatible version

3. Resolve library dependency graph
   ├── Topological sort of dependencies
   ├── Version constraint satisfaction
   └── Conflict detection and resolution

4. Verify ABI compatibility
   ├── Check major version alignment
   ├── Validate symbol compatibility
   └── Confirm linking requirements

5. Download and cache resolved versions
   ├── Fetch missing packages
   ├── Verify checksums and signatures
   └── Prepare for compilation
```

### Conflict Resolution

When version conflicts arise:

```toml
# Conflict scenario
app_dependency = { database = ">=1.3.0" }
lib_dependency = { database = "<1.3.0" }  

# Resolution strategies
[resolver]
strategy = "newest"     # Prefer newer versions (default)
strategy = "minimal"    # Prefer older versions  
strategy = "exact"      # Require exact matches
strategy = "interactive" # Prompt user for choices

# Override specific conflicts
[resolver.overrides]
database = "1.3.1"     # Force specific version
network = "=1.2.0"     # Override all constraints
```

**Resolution example:**
```
Resolving dependencies for web_service 1.2.3:

├── std ~1.3 
│   └── Resolved: 1.3.8 (latest in 1.3.x)
├── database ^1.2
│   ├── Requires: std >=1.1.0  
│   ├── Transitive: crypto ^1.0
│   └── Resolved: 1.4.2 (latest compatible)
├── network =1.4.2
│   ├── Requires: std >=1.2.0
│   └── Resolved: 1.4.2 (exact match)
└── logging >=1.1.0
    └── Resolved: 1.3.1 (latest)

All dependencies resolved successfully!
ABI compatibility verified ✓
```

## Distribution and Multi-Version Support

### Multi-Version Package Structure

Libraries can support multiple compiler major versions:

```
database-1.4.2/
├── manifest.toml
├── cprime1/                    # For compiler 1.x
│   ├── database.so            # Compiled library
│   ├── database.bc            # LLVM IR  
│   ├── database.rlib          # Static library
│   └── src/                   # Source headers
├── cprime2/                    # For compiler 2.x
│   ├── database.so            # Compiled for 2.x ABI
│   ├── database.bc            # LLVM IR for 2.x
│   └── src/                   # Updated source
├── docs/                      # Documentation
└── tests/                     # Test suite
```

### Version Metadata

Each package includes comprehensive metadata:

```toml
[package]
name = "database"
version = "3.2.1"
description = "High-performance database library"
license = "MIT"
authors = ["CPrime Team <team@cprime.org>"]

[compatibility]
compiler_min = "1.2.0"         # Minimum compiler version
compiler_max = "1.99.99"       # Maximum compiler version
std_min = "1.1.0"              # Minimum std library
std_max = "1.99.99"            # Maximum std library

[built_with]
compiler = "1.3.5"             # Built with this compiler
std = "1.2.8"                  # Built with this std
llvm = "17.0"                  # LLVM version used
build_date = "2024-03-15"      # Build timestamp

[abi]
version = "1.0"                # ABI version
symbols = "database-1.0.symbols" # Symbol export list
compatibility_hash = "abc123def456" # ABI compatibility hash

[dependencies]
std = "^1.2"
network = "~1.4"
crypto = ">=1.0.0"
```

### Platform-Specific Versioning

```toml
[targets.x86_64-linux]
min_compiler = "1.0.0"
max_tested = "1.3.5"
features = ["simd", "avx2"]

[targets.aarch64-linux]
min_compiler = "1.1.0"         # ARM support added in 1.1
max_tested = "1.3.5"
features = ["neon"]

[targets.wasm32]
min_compiler = "1.2.0"         # WASM support added in 1.2
max_tested = "1.3.0"
features = ["web"]
restrictions = ["no_std_only"]  # Limited std library

[targets.x86_64-windows]
min_compiler = "1.0.0"
max_tested = "1.3.5"  
features = ["win32_api"]
```

## Migration Support

### Version Upgrade Tools

CPrime provides comprehensive migration assistance:

```bash
# Check current project compatibility
cprime compat-check
Output:
  Current configuration:
    Compiler: 1.3.5
    Std:      1.2.8  
    Dependencies: 12 libraries
  
  Status: ✓ All compatible
  
  Available upgrades:
    Compiler: 1.4.0 (minor - compatible)
    Std:      1.3.2 (minor - compatible)
    
  Major version available: 2.0.0-beta
  Run 'cprime migrate --preview 2.0' for details

# Preview major version migration
cprime migrate --to 2.0 --preview
Output:
  Migration to CPrime 2.0.0:
  
  Breaking changes detected:
  ├── semconst is now default field modifier
  ├── Signal syntax changed: except() → signals()
  ├── Channel API simplified
  └── Runtime selection syntax updated
  
  Dependency updates required:
  ├── database: 1.4.2 → 2.0.1 (available)
  ├── network: 1.8.1 → 2.0.0 (available)  
  └── logging: 1.3.1 → No 2.x version available
  
  Migration effort: Medium (3-5 days)
  Generate migration script? [Y/n]

# Generate migration script
cprime migrate --to 2.0 --generate
Output:
  Generated migration script: migrate-to-2.0.sh
  
  Script includes:
  ✓ Dependency updates
  ✓ Syntax transformations  
  ✓ Code modernization
  ✓ Test updates
  
  Review script before running!
```

### Multi-Version Project Support

Projects can maintain multiple versions simultaneously:

```toml
[workspace]
members = [
    { path = "core", compiler = "1.0", channel = "lts" },
    { path = "api", compiler = "1.3", channel = "stable" },
    { path = "experimental", compiler = "2.0-beta", channel = "nightly" }
]

[workspace.dependencies]
shared_utils = { path = "utils", version = "*" }

# Different build targets
[[workspace.build]]
name = "stable_release"
members = ["core", "api"]
compiler = "1.3"

[[workspace.build]]  
name = "experimental_build"
members = ["experimental"]
compiler = "2.0-beta"
```

### Compatibility Testing

```yaml
# CI configuration for version testing
name: Version Compatibility

jobs:
  test_matrix:
    strategy:
      matrix:
        compiler: [1.0-lts, 1.2, 1.3-stable, 2.0-beta]
        std: [minimal, latest, experimental]
        exclude:
          # Invalid combinations
          - compiler: 1.0-lts
            std: experimental
          - compiler: 2.0-beta
            std: minimal
    
    steps:
      - uses: cprime/setup-compiler@v1
        with:
          version: ${{ matrix.compiler }}
          std-version: ${{ matrix.std }}
      
      - run: cprime test --compatibility-mode
      - run: cprime build --warnings-as-errors
      - run: cprime compat-check --future-versions
```

## Special Version Scenarios

### Security Updates

Security patches follow strict compatibility requirements:

```cpp
// Version 1.3.5 → 1.3.6 (security patch)

// BEFORE (vulnerable):
fn parse_input(data: &str) -> ParsedData {
    // Buffer overflow vulnerability
    let mut buffer = [0u8; 256];
    unsafe {
        std::ptr::copy_nonoverlapping(
            data.as_ptr(),
            buffer.as_mut_ptr(), 
            data.len()  // No bounds check!
        );
    }
    ParsedData::from_buffer(buffer)
}

// AFTER (secure):
fn parse_input(data: &str) -> ParsedData {
    // Fixed: proper bounds checking
    let mut buffer = [0u8; 256];
    let copy_len = std::cmp::min(data.len(), 256);
    buffer[..copy_len].copy_from_slice(&data.as_bytes()[..copy_len]);
    ParsedData::from_buffer(buffer)
}

// Security patch requirements:
// ✓ Same function signature
// ✓ Same ABI  
// ✓ Compatible behavior
// ✓ Can be backported to older minors
```

**Security update propagation:**
```
Security fix in: 1.3.6
Also backported to:
├── 1.2.15 (LTS maintenance)
├── 1.1.28 (Extended support)
└── 2.0.1 (Current development)
```

### Experimental Features

Features progress through stability levels:

```toml
[features]
# Feature progression example
async_generators = {
    introduced = "1.4.0-beta",
    stabilized = "1.5.0",
    description = "Async generator functions"
}

experimental_syntax = {
    introduced = "1.4.0-nightly",
    stabilized = "never",  # May be removed
    warning = "Highly experimental - API will change"
}
```

**Feature usage:**
```cpp
// Experimental feature usage
#[feature(async_generators)]  // Requires nightly
async gen fn number_stream() -> i32 {
    for i in 0..10 {
        yield i;
    }
}

// Compiler warnings:
// Warning: async_generators is experimental (1.4.0-nightly)
// This feature may change or be removed in future versions
```

### Deprecation Cycle

Features are removed through a structured process:

```cpp
// Deprecation timeline example

// Version 1.3.0: Deprecation warning
#[deprecated(since = "1.3.0", remove = "2.0.0", 
             reason = "Use new_function() instead")]
fn old_function() -> Result<()> {
    // Implementation still available
}

// Version 1.5.0: Stronger warning
#[deprecated(since = "1.3.0", remove = "2.0.0",
             reason = "Use new_function() instead")]
#[warn(deprecated)]  // Warning by default
fn old_function() -> Result<()> {
    // Still works but warns more prominently
}

// Version 1.9.0: Error with flag
#[deprecated(since = "1.3.0", remove = "2.0.0")]
#[error_on(deprecated)]  // Error with --strict flag
fn old_function() -> Result<()> {
    // Error in strict mode, warning otherwise
}

// Version 2.0.0: Removed
// fn old_function() - REMOVED
```

## Policy and Enforcement

### Version Policies

Projects can enforce version policies:

```toml
[project.policy]
# Security policies
allow_prerelease = false        # No beta/alpha in production
require_exact_std = true        # Lock std library version
security_updates = "automatic"  # Auto-accept security patches
outdated_warning_days = 90      # Warn about old versions

# Compatibility policies  
min_compiler_age_days = 30      # Don't use very new compilers
max_compiler_age_days = 365     # Don't use very old compilers
require_lts_for_production = true  # Production must use LTS

# Dependency policies
max_dependency_age_days = 180   # Warn about old dependencies
require_security_audit = true   # Audit deps for vulnerabilities
pin_transitive_deps = false     # Allow transitive updates
```

### CI/CD Integration

```yaml
# Comprehensive version testing
name: Version Matrix Testing

on:
  push:
    branches: [main, develop]
  pull_request:

jobs:
  version_compatibility:
    strategy:
      matrix:
        compiler_version: 
          - "1.0"      # LTS  
          - "1.3"      # Current stable
          - "1.4-beta" # Next version
        std_version:
          - "minimal"   # Oldest supported std
          - "latest"    # Latest compatible std
          - "preview"   # Preview next version
        exclude:
          # Don't test invalid combinations
          - compiler_version: "1.0"
            std_version: "preview"
    
    steps:
      - uses: actions/checkout@v3
      
      - name: Setup CPrime
        uses: cprime/setup@v1
        with:
          compiler-version: ${{ matrix.compiler_version }}
          std-version: ${{ matrix.std_version }}
      
      - name: Validate dependencies
        run: |
          cprime deps check --compatibility
          cprime deps audit --security
          
      - name: Build with version constraints
        run: |
          cprime build --warnings-as-errors
          cprime test --all
          
      - name: Future compatibility check
        run: cprime compat-check --future-versions

  minimum_supported_version:
    # Test minimum version requirements  
    runs-on: ubuntu-latest
    steps:
      - uses: cprime/setup@v1
        with:
          compiler-version: "1.0.0"  # Minimum supported
          std-version: "1.0.0"
      
      - run: cprime build
      - run: cprime test
```

### Strict Version Mode

For maximum reproducibility:

```toml
[project.policy.strict]
enabled = true

# Lock all versions exactly
[dependencies]
std = "=1.3.5"              # Exact version required
database = "=1.4.2"         # No flexibility
network = "=1.8.1"          
logging = "=1.2.7"

# Generate lockfile
[lock]
auto_generate = true         # cprime.lock file
include_transitive = true   # Lock all deps
verify_checksums = true     # Verify integrity
```

**Lockfile example (cprime.lock):**
```toml
# Generated lockfile - do not edit manually
[metadata]
generated_at = "2024-03-15T10:30:00Z"
cprime_version = "1.3.5"

[dependencies.std]
version = "1.3.5"
checksum = "sha256:abc123def456..."
source = "registry+https://packages.cprime.org"

[dependencies.database]
version = "1.4.2"  
checksum = "sha256:def456abc123..."
source = "registry+https://packages.cprime.org"
dependencies = [
    "crypto = 1.0.8",
    "network = 1.8.1"
]
```

## Best Practices

### For Library Authors

```toml
# Good versioning practices for libraries
[package]
name = "my_awesome_library"
version = "1.2.3"

# Specify wide compatibility ranges
[compatibility]
compiler_min = "1.0.0"     # Support LTS users
compiler_max = "1.99.99"   # Allow future 1.x compilers

# Conservative dependency ranges  
[dependencies]
std = ">=1.0.0"            # Allow old std versions
network = "^1.1"           # Allow minor updates
```

**Version bump guidelines:**
```cpp
// Patch version (1.2.3 → 1.2.4)
// ✓ Bug fixes
// ✓ Documentation improvements  
// ✓ Performance improvements (same API)
// ✗ New functions
// ✗ Changed behavior

// Minor version (1.2.3 → 1.3.0)
// ✓ New functions/methods
// ✓ New optional parameters (with defaults)
// ✓ Performance improvements
// ✓ Deprecation warnings
// ✗ Breaking API changes
// ✗ Removed functions

// Major version (1.2.3 → 2.0.0)
// ✓ Breaking API changes
// ✓ Removed deprecated functions
// ✓ Changed behavior
// ✓ New required parameters
```

### For Application Developers

```toml
# Application version management
[dependencies]
# Prefer caret ranges for flexibility
database = "^1.4"          # >= 1.4.0, < 2.0.0
network = "^2.1"           # >= 2.1.0, < 3.0.0

# Pin exact versions for stability-critical
crypto = "=1.8.2"          # Security-sensitive

# Use tilde for compatible updates
logging = "~1.3.1"         # >= 1.3.1, < 1.4.0
```

**Update strategies:**
```bash
# Conservative updates
cprime update --patch-only      # Only patch versions
cprime update --security-only   # Only security fixes

# Regular updates  
cprime update --minor          # Minor and patch versions
cprime update --compatible     # Stay within major versions

# Aggressive updates
cprime update --latest         # Latest available versions
cprime update --preview        # Include preview versions
```

## Cross-References

- **[Compilation](compilation.md)**: ABI details and binary compatibility
- **[Library Linking](library-linking.md)**: Template instantiation and library distribution
- **[Language Summary](language-summary.md)**: Overall language evolution and stability

CPrime's versioning system provides a robust foundation for both rapid development and long-term stability, ensuring that developers can confidently build applications while the language and ecosystem continue to evolve.