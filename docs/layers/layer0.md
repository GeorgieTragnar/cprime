# Layer 0 - Input Processing

## Responsibilities

- **Source stream creation**: Convert file paths, URLs, and direct input into named stringstreams
- **Encoding normalization**: Handle UTF-8, UTF-16, ASCII and other text encodings consistently  
- **Source identification**: Assign unique stream identifiers for error correlation and module organization
- **Content validation**: Basic input validation and character encoding verification
- **Stream organization**: Prepare input data structure for efficient Layer 1 processing

## Input/Output

- **Input**: Various source formats (file paths, URLs, direct text, stdin)
- **Output**: `std::map<std::string, std::stringstream>` - Named input streams ready for tokenization
- **Side Effects**: File system access, network requests (for URL inputs), encoding conversions
- **Dependencies**: STL file/stream handling, optional networking for remote sources

## Core Data Structure - Input Streams

```cpp
// Layer 0 Output
std::map<std::string, std::stringstream> input_streams;
```

### **Stream Identification Strategy**
- **File inputs**: Use canonical file path as stream ID (`/path/to/source.cp`)
- **URL inputs**: Use full URL as stream ID (`https://example.com/code.cprime`) 
- **Direct text**: Use descriptive identifier (`<inline>`, `<stdin>`, `<generated>`)
- **Module inputs**: Use module path as stream ID (`module::submodule`)

## Input Source Types

### **File System Sources**
```cpp
// Single file
input_processor.add_file("/path/to/main.cprime");

// Directory scanning
input_processor.add_directory("/src", "*.cp", "*.cprime");

// Glob patterns  
input_processor.add_pattern("/src/**/*.cprime");
```

### **Direct Text Sources**
```cpp
// Inline code
input_processor.add_text("main_inline", "class Test { int x; };");

// Standard input
input_processor.add_stdin("<stdin>");

// Generated code
input_processor.add_generated("template_expansion", generated_code);
```

### **Network Sources** (Optional)
```cpp
// Remote modules
input_processor.add_url("https://cprime.org/std/vector.cprime");

// Package management integration
input_processor.add_package("std::vector", "1.2.3");
```

### **Module System Integration**
```cpp
// Module dependencies
input_processor.add_module("graphics::renderer");
input_processor.add_module_tree("std", include_tests = false);
```

## Processing Pipeline

### **Input Validation Phase**
1. **Source accessibility**: Verify files exist, URLs are reachable
2. **Permission validation**: Check read permissions on file sources
3. **Encoding detection**: Identify character encoding (UTF-8, UTF-16, ASCII)
4. **Size validation**: Ensure sources are within reasonable size limits
5. **Content preview**: Basic validation that content appears to be source code

### **Stream Creation Phase**  
1. **Content reading**: Load source content into memory
2. **Encoding normalization**: Convert all inputs to consistent UTF-8
3. **Stream construction**: Create stringstream with processed content
4. **Stream identification**: Assign unique, meaningful stream IDs
5. **Metadata attachment**: Store source metadata (file paths, timestamps, etc.)

### **Organization Phase**
1. **Dependency ordering**: Organize streams based on import/include relationships
2. **Module grouping**: Group related streams by module boundaries
3. **Duplicate detection**: Identify and handle duplicate source inclusions
4. **Stream validation**: Final validation of created stream collection

## Input Stream Metadata

### **Source Correlation Data**
Layer 0 prepares metadata for error correlation in later layers:

```cpp
struct SourceMetadata {
    std::string original_path;        // Original file path or URL
    std::string canonical_id;         // Normalized stream identifier  
    std::time_t last_modified;        // File modification time
    std::string encoding;             // Original character encoding
    size_t content_size;              // Byte size of content
    std::string checksum;             // Content hash for change detection
};
```

### **Module Relationship Data**
```cpp
struct ModuleRelationship {
    std::string module_name;          // Module identifier
    std::vector<std::string> dependencies; // Required modules
    std::vector<std::string> exports;      // Exported symbols
    ModuleType type;                  // Library, application, test, etc.
};
```

## Error Handling and Validation

### **Input Errors**
- **File not found**: Missing source files with clear error messages
- **Permission denied**: Insufficient file system permissions  
- **Encoding errors**: Invalid or unsupported character encodings
- **Network failures**: Unreachable URLs or connection timeouts
- **Size limits**: Oversized input files beyond processing limits

### **Content Validation**
- **Binary detection**: Warn about binary files passed as source
- **Encoding corruption**: Detect and report character encoding issues
- **Suspicious content**: Identify potential security risks in remote sources
- **Format validation**: Basic checks that content appears to be CPrime source

### **Graceful Degradation**
- **Partial failure**: Continue processing available sources when some fail
- **Warning collection**: Accumulate non-fatal issues for user review
- **Recovery strategies**: Attempt alternative encodings, fallback sources
- **User feedback**: Clear progress indication for slow operations (large files, network)

## Performance Characteristics

### **Memory Management**
- **Streaming**: Large files processed in chunks to minimize memory usage
- **Lazy loading**: Sources loaded only when needed by Layer 1
- **Content caching**: Optional caching of frequently accessed sources
- **Memory limits**: Configurable limits on total content size

### **I/O Optimization**
- **Parallel loading**: Multiple sources can be loaded concurrently
- **Efficient reading**: Use platform-optimal file reading strategies
- **Network optimization**: Connection pooling and compression for URL sources
- **Caching strategies**: Local caching of remote sources with validation

### **Scalability**
- **Large projects**: Efficient handling of projects with thousands of source files
- **Network sources**: Robust handling of slow or unreliable network connections
- **Module systems**: Efficient dependency resolution and loading
- **Incremental processing**: Support for processing only changed sources

## Layer 1 Interface

Layer 0 provides optimized input for Layer 1 tokenization:

### **Stream Organization**
- **Named streams**: Unique identifiers enable source correlation throughout compilation
- **Content ready**: All encoding normalization and validation completed
- **Metadata available**: Source information attached for error reporting
- **Efficient access**: Stringstream format enables efficient character-by-character processing

### **Error Correlation Foundation**
- **Source mapping**: Stream IDs enable precise source location tracking
- **Original preservation**: Maintain links to original source locations
- **Module context**: Module relationship data supports semantic analysis
- **Change detection**: Checksums and timestamps support incremental compilation

## Integration Points

### **Build System Integration**
- **File watching**: Integration with file system change detection
- **Incremental compilation**: Only reprocess changed sources
- **Dependency tracking**: Understanding of source dependencies for minimal rebuilds
- **Output coordination**: Coordinate with build system for output management

### **IDE Integration**  
- **Real-time processing**: Support for processing unsaved editor buffers
- **Source correlation**: Enable jump-to-definition and error highlighting
- **Module navigation**: Support IDE module and dependency exploration
- **Live updates**: Process sources as user types for real-time error feedback

### **Package Management**
- **Remote modules**: Integration with CPrime package repositories
- **Version resolution**: Handle module version constraints and conflicts
- **Dependency installation**: Automatic download of missing dependencies
- **Security validation**: Verify checksums and signatures of remote sources

## Key Design Principles

### **Source Fidelity**
- **Lossless processing**: Maintain perfect source correlation throughout pipeline
- **Encoding preservation**: Handle all common text encodings without data loss
- **Original accessibility**: Always maintain access to original source content
- **Change detection**: Support efficient incremental compilation workflows

### **Robustness**
- **Error resilience**: Continue processing despite individual source failures
- **Input validation**: Comprehensive validation prevents downstream errors
- **Security awareness**: Careful handling of remote and untrusted sources
- **Resource limits**: Prevent resource exhaustion from malicious or corrupted inputs

### **Performance**
- **Efficient I/O**: Optimize file and network access patterns
- **Memory conscious**: Handle large projects without excessive memory usage
- **Parallel processing**: Leverage multiple cores for concurrent source loading
- **Caching strategies**: Intelligent caching reduces redundant I/O operations

### **Integration Friendly**
- **Tool compatibility**: Work well with existing build tools and IDEs
- **Standards compliance**: Follow platform conventions for file handling
- **Module system**: Foundation for sophisticated module and package management
- **Development workflow**: Support modern development practices and workflows