# CPrime Compiler Project - AI Assistant Reference

## Project Overview

CPrime is a systems programming language designed to achieve everything that C++ tries to achieve, making close or identical decisions where C++ excels, while completely replacing C++-style inheritance with a differently structured polymorphism system based on access rights and capability security.

### Core Philosophy
- **C++ Memory Model**: Manual memory management, references can dangle, RAII patterns
- **Go-Style Concurrency**: Coroutines, channels, structured concurrency
- **Capability Security**: Access rights as compile-time security boundaries
- **Zero-Cost Abstractions**: Pay only for what you use, explicit performance costs
- **C++ Interop**: Direct compatibility and gradual migration path

### Three-Class System
1. **Data Classes**: Pure state and memory operations only
2. **Functional Classes**: Stateless operations with memoize-only optimization storage
3. **Danger Classes**: Traditional OOP for C++ interop and unsafe operations

## Directory Structure

```
/home/jirka/repositories/cprime/
├── compiler/              # Main compiler implementation
│   ├── src/              # N-layer architecture source code
│   │   ├── layerN/       # Core layer implementations (1, 2, 3, 4, ...)
│   │   ├── layerNvalidation/  # Validation counterparts for each layer
│   │   ├── common/       # Shared components (logging, types, buffers)
│   │   └── cli/          # cprime_cli binary for analysis/debugging
│   └── tests/            # Google Test framework with layer-specific tests
│       └── unit/LayerN/  # Layer-organized test structure
├── docs/                 # Language specification and layer documentation
│   ├── layers/          # Individual layer documentation (see references below)
│   └── commons/         # Shared component documentation
├── examples/            # Sample .cp and .cprime files for testing
├── scripts/             # BUILD/TEST/DEBUG/COMPILE AUTOMATION (MANDATORY INTERFACE)
│   ├── build.sh         # ALL BUILDING MUST USE THIS
│   ├── test.sh          # ALL TESTING MUST USE THIS  
│   ├── cprime_analyze.sh # ALL DEBUGGING MUST USE THIS
│   └── compile.sh       # ALL EXAMPLE COMPILATION MUST USE THIS
└── logs/               # Compiler and test execution logs
```

## **MANDATORY WORKFLOW SCRIPTS**

### ⚠️ CRITICAL RULE: Use Scripts ONLY
**ALL build, test, debug, and compilation operations MUST go through the official scripts. Never bypass these scripts - if they can't do something needed, enhance the scripts instead.**

### Build Script: `./scripts/build.sh`
**The ONLY way to build the project**

```bash
# Basic usage
./scripts/build.sh                    # Debug build
./scripts/build.sh -c                 # Clean build
./scripts/build.sh -t                 # Build with tests
./scripts/build.sh -r                 # Release build

# Combined options (using getopts)
./scripts/build.sh -ct                # Clean build with tests
./scripts/build.sh -ctr               # Clean release build with tests
./scripts/build.sh -ctv               # Clean build with tests, verbose
```

**Options:**
- `-c`: Clean build (removes build directory first)
- `-t`: Build with tests (enables Google Test suite)
- `-r`: Release mode (optimized build)
- `-v`: Verbose output
- `-h`: Help

**Dependencies:**
- CMake 3.16+, C++17
- vcpkg with spdlog (required)
- GTest (required only with `-t` flag)

### Test Runner: `./scripts/test.sh`
**The ONLY way to run tests**

```bash
# Layer selection with integers
./scripts/test.sh 1,3,5        # Run layers 1, 3, 5 with validation
./scripts/test.sh 1-4          # Run layers 1 through 4 with validation
./scripts/test.sh -l 1,3,5     # Run layers 1, 3, 5 WITHOUT validation

# Explicit layer selection
./scripts/test.sh --l1 --l3v   # Layer 1 core + Layer 3 validation
./scripts/test.sh --l2v        # Only Layer 2 validation

# Legacy backward compatibility
./scripts/test.sh -l1 -l2v     # Old syntax still supported
```

**Key Features:**
- **Integer Parsing**: `1,3,5` (list) or `1-4` (range)
- **Validation Control**: `-l` suppresses validation tests
- **Explicit Overrides**: `--l1v`, `--l2v`, etc. for specific validation layers
- **Backward Compatible**: Supports old `-l1`, `-l2v` syntax

### Debug/Analysis Script: `./scripts/analyze.sh`
**The ONLY way to debug and analyze code**

```bash
# Layer 0 analysis (CURRENTLY AVAILABLE)
./scripts/analyze.sh input hello                    # Debug input processing for hello.cprime
./scripts/analyze.sh streams simple                 # Analyze stringstreams for simple.cp
./scripts/analyze.sh validation class_test          # Show file validation
./scripts/analyze.sh layer0 hello                   # Complete Layer 0 analysis

# With options
./scripts/analyze.sh -v streams hello               # Verbose output
./scripts/analyze.sh --quiet layer0 simple          # Minimal headers
./scripts/analyze.sh layer0 hello -- --extra-flag   # Pass extra CLI flags

# Future layers (not yet implemented)
./scripts/analyze.sh tokens hello                   # Layer 1: Raw token dump
./scripts/analyze.sh context hello                  # Layer 2: Context resolution  
./scripts/analyze.sh ast hello                      # Layer 3: AST analysis
./scripts/analyze.sh full hello                     # Complete pipeline

# Show available examples
./scripts/analyze.sh --help                         # List all available examples
```

**Key Features:**
- **Smart File Resolution**: Looks for `.cprime` first, then `.cp` extension
- **Built-in Example Discovery**: Automatically lists all available examples  
- **Layer 0 Focus**: Currently supports input processing, stream analysis, and validation
- **Verbose/Quiet Modes**: Control output detail level
- **CLI Integration**: Uses `cprime_cli` binary for actual analysis

**Purpose:** 
- **Primary Interface**: Main way to debug compiler layers with example files
- **Development Testing**: Essential for Layer 0 debugging and validation
- **Future Integration**: Will expand as more layers are implemented
- **Layer-by-Layer**: Focused debugging capabilities for each compiler layer

### Compilation Script: `./scripts/compile.sh`
**The ONLY way to compile example CPrime files**

```bash
# Basic compilation
./scripts/compile.sh hello                    # Compile examples/hello.cprime
./scripts/compile.sh simple                   # Compile examples/simple.cp
./scripts/compile.sh class_test               # Compile examples/class_test.cprime

# With options
./scripts/compile.sh --debug hello            # Enable debug mode
./scripts/compile.sh --quiet simple           # Minimal output headers
./scripts/compile.sh hello -- --dump-ast      # Pass extra flags to compiler

# Show available examples
./scripts/compile.sh --help                   # List all available examples
```

**Key Features:**
- **Smart File Resolution**: Looks for `.cprime` first, then `.cp` extension
- **Built-in Example Discovery**: Automatically lists all available examples
- **Clean Output Display**: Headers, compiler output, and success/failure status
- **Flag Passing**: Use `--` to pass additional flags directly to compiler
- **Error Handling**: Clear messages for missing files or build issues

**Options:**
- `--debug`: Enable debug mode in cprime compiler
- `--quiet`: Suppress verbose headers and file information
- `--help`: Show usage and list all available examples
- `-- <flags>`: Pass remaining arguments directly to cprime compiler

**Purpose:**
- **Primary Interface**: Main way to test compiler with example files
- **Development Testing**: Easy way to test Layer 0 input processing
- **Future Integration**: Will expand as more layers are implemented
- **User-Friendly**: Simple interface for exploring CPrime language examples

## N-Layer Architecture

### Current Layer Structure (Expandable)
- **Layer 1**: Raw Tokenization (`layer1/` + `layer1validation/`)
- **Layer 2**: Context-Sensitive Semantic Analysis (`layer2/` + `layer2validation/`)
- **Layer 3**: AST Building (`layer3/` + `layer3validation/`)
- **Layer 4**: RAII Injection (`layer4/` + `layer4validation/`)
- **LayerN**: Architecture designed for future expansion

### Layer Documentation References
- [Layer 1 - Raw Tokenization](docs/layers/layer1.md) - Pure syntactic tokenization
- [Layer 2 - Semantic Analysis](docs/layers/layer2.md) - Context-sensitive keyword resolution
- [Layer 3 - AST Building](docs/layers/layer3.md) - Abstract syntax tree construction
- [Layer 4 - RAII Injection](docs/layers/layer4.md) - Resource management injection
- [Commons Documentation](docs/commons/) - Shared components, logging, validation

### Key Technical Features
- **Context-Sensitive Keywords**: `runtime`, `defer`, `exposes` have different meanings by context
- **Selective Buffer Management**: Component-based logging with spdlog
- **Validation Pipeline**: Each layer has comprehensive validation counterpart
- **Google Test Integration**: Layer-organized test structure with `gtest_discover_tests`

## Development Rules & Policies

### Problem Resolution Policy
**For any problems with uncertainty of resolution:**
1. **DO NOT** automate problem-solving without explicit user permission
2. **DO** analyze the problem thoroughly and present options
3. **DO** ask for explicit permission before proceeding with uncertain fixes
4. **DO** proceed systematically through problems one by one
5. **ONLY** automate when user explicitly requests it for a range of known problems

### Script Enhancement Policy
**If the build/test/debug/compile scripts cannot do something you need:**
1. **DO NOT** bypass the scripts
2. **DO** enhance the appropriate script to support the new functionality
3. **DO** maintain backward compatibility
4. **DO** update this documentation after script enhancements

### Script Maintenance
- All four scripts (`build.sh`, `test.sh`, `cprime_analyze.sh`, `compile.sh`) must be maintained throughout the project lifecycle
- Scripts are the official interface - they will evolve into sophisticated tooling
- Consistency is critical for development workflow

### Testing Standards
- **Layer-Based Organization**: Tests organized by layer (Layer1/, Layer2/, etc.)
- **Validation Testing**: Each layer has validation tests that can be run independently
- **Google Test Framework**: Modern C++ testing with comprehensive assertions
- **Buffered Logging**: Component-specific log buffering for clean test output

## Common Development Workflows

### Setting Up Development Environment
```bash
# 1. Build with tests
./scripts/build.sh -ct

# 2. Run comprehensive tests
./scripts/test.sh 1-4

# 3. Test compiler with examples
./scripts/compile.sh hello

# 4. Debug specific functionality
./scripts/cprime_analyze.sh full examples/hello.cprime
```

### Layer Development Workflow
```bash
# 1. Build and test specific layer
./scripts/build.sh -ct
./scripts/test.sh 2  # Layer 2 with validation

# 2. Test with examples
./scripts/compile.sh class_test --debug

# 3. Debug layer functionality (when available)
./scripts/analyze.sh context test_sample

# 4. Run validation only
./scripts/test.sh --l2v
```

### Testing Current Compiler (Layer 0 Only)
```bash
# 1. Build compiler
./scripts/build.sh

# 2. Test with simple examples
./scripts/compile.sh hello
./scripts/compile.sh simple
./scripts/compile.sh class_test

# 3. Test with debug output
./scripts/compile.sh --debug test_orchestrator

# 4. Test quiet mode for minimal output
./scripts/compile.sh --quiet variables_test

# 5. See all available examples
./scripts/compile.sh --help
```

### Layer 0 Debugging
```bash
# 1. Debug input processing
./scripts/analyze.sh input hello

# 2. Analyze stream creation
./scripts/analyze.sh streams simple

# 3. Check file validation
./scripts/analyze.sh validation class_test

# 4. Complete Layer 0 analysis
./scripts/analyze.sh layer0 hello

# 5. Verbose debugging
./scripts/analyze.sh -v layer0 simple

# 6. See all available examples
./scripts/analyze.sh --help
```

### Debugging Failed Tests
```bash
# 1. Run specific failing layer
./scripts/test.sh -l 1  # Layer 1 without validation

# 2. Debug Layer 0 issues
./scripts/analyze.sh layer0 problematic_example

# 3. Check logs
tail -f logs/cprime.log
```

## Git Repository Status
- **Clean Repository**: Simplified .gitignore (only ignores `build/`, `logs/`, temp files)
- **Version Control**: All legitimate files under version control
- **Branch**: Currently on `develop` branch

## Quick Reference

### Most Common Commands
```bash
# Clean build compiler
./scripts/build.sh -c

# Test with example files
./scripts/compile.sh hello
./scripts/compile.sh class_test --debug

# Debug Layer 0 (input processing)
./scripts/analyze.sh layer0 hello
./scripts/analyze.sh -v streams simple

# Run all layers with validation (when available)
./scripts/test.sh 1-4

# Future debugging (when layers implemented)
./scripts/analyze.sh tokens hello
./scripts/analyze.sh context simple
```

### When Things Go Wrong
1. **Build Fails**: Check vcpkg dependencies, ensure CMake 3.16+
2. **Tests Fail**: Use layer-specific testing with `./scripts/test.sh [layer]`
3. **Analysis Fails**: Ensure `cprime_cli` binary exists in `build/bin/`
4. **Layer 0 Issues**: Use `./scripts/analyze.sh layer0 [example]` for debugging
5. **Scripts Need Enhancement**: Follow Script Enhancement Policy above

---

## Layer-Agnostic Test Case Creation Methodology

### Core Principle
**layerN file serves dual purpose:**
- **Input** for Layer N processing
- **Expected output validation** for Layer N-1 processing

### Test Discovery Algorithm
Tests are automatically discovered where both `layerN` and `layerN+1` files exist:
- If files exist: `layer1`, `layer2`, `layer4`, `layer5`, `layer6`
- Then tests run for: **Layer 1** (1→2), **Layer 4** (4→5), **Layer 5** (5→6)
- Layer 2 and 3 are skipped (no layer3 file to validate layer2 output)

### Test Case Structure
```
test_cases/{test_name}/
├── layer1          # CPrime source input
├── layer2          # Layer 1 expected output / Layer 2 input
├── layer3          # Layer 2 expected output / Layer 3 input (if exists)
├── layer4          # Layer 3 expected output / Layer 4 input (if exists)
└── layerN          # Layer N-1 expected output / Layer N input
```

### Creation Workflow

**1. Create Base Input (layer1)**
- Write focused CPrime code for specific language feature
- Use clear, testable syntax patterns

**2. Generate Layer Outputs Sequentially**
```bash
# Generate layer2 (Layer 1 tokenization output)
./scripts/analyze.sh tokens {test_name} -o test_cases/{test_name}/layer2

# Generate layer3 (Layer 2 structure output) - when implemented
./scripts/analyze.sh context {test_name} -o test_cases/{test_name}/layer3

# Generate layer4 (Layer 3 AST output) - when implemented  
./scripts/analyze.sh ast {test_name} -o test_cases/{test_name}/layer4
```

**3. Validation Process (MANDATORY)**

**Step 3a: Binary Content Analysis**
```bash
# 1. Show exact binary content and positions
xxd -c 16 test_cases/{test_name}/layer1

# 2. Review token output for validation
head -20 test_cases/{test_name}/layer2
```

**Step 3b: Character-by-Character Validation**
- Map each byte position to corresponding token
- Verify multi-character tokens (identifiers, literals) span correct ranges
- Ensure no gaps or overlaps in position coverage
- Validate line/column tracking across newlines

**Step 3c: Token Accuracy Checks**
- **Position Accuracy**: Every token's pos/line/col must match actual source locations
- **Character Coverage**: Every input byte must appear in exactly one token
- **Token Classification**: Verify token types match Layer 1 design decisions
- **Boundary Handling**: Check proper handling of whitespace, newlines, EOF

**Step 3d: Complete Coverage Validation**
```bash
# Count total characters in input
wc -c test_cases/{test_name}/layer1

# Count total tokens generated  
wc -l test_cases/{test_name}/layer2

# Verify last token position + length equals file size
```

**Step 3e: Pipeline Validation** (for future layers)
- Ensure Layer N-1 output exactly matches layerN input when both exist

**Step 4: Run Tests (MANDATORY)**
```bash
# Build with tests enabled
./scripts/build.sh -ct

# Run Layer 1 tests to validate new test cases
./scripts/test.sh 1

# Check for any test failures
# If tests fail, examine failure logs in tmp/ directory
# Fix either the test case or the expected output as needed
```

**Step 5: Test Result Validation**
- Verify all test cases pass
- Check failure logs if tests fail - may indicate:
  - Incorrect expected output in layer2 file
  - Actual tokenization bug in Layer 1
  - Test infrastructure issues
- Re-run validation if test cases are updated

**4. Test Execution Logic**
```python
# Pseudo-code for test discovery
for test_case in test_cases:
    layers = discover_layer_files(test_case)  # [1,2,4,5,6]
    testable_layers = []
    
    for i in range(len(layers)-1):
        if layers[i+1] == layers[i] + 1:  # consecutive layers
            testable_layers.append(layers[i])
    
    # Result: [1,4,5] - test Layer 1→2, Layer 4→5, Layer 5→6
```

**5. Quality Criteria**
- **Completeness**: Every layer input fully processed to expected output
- **Accuracy**: Byte-perfect position tracking through all layers
- **Consistency**: Layer N-1 output must exactly match Layer N input
- **Coverage**: Test cases cover comprehensive language feature matrix

**6. Example Test Cases**
- **arithmetic_operators**: Basic arithmetic (`+`, `-`, `*`, `/`, `%`)
- **arithmetic_compound**: Compound assignment (`+=`, `-=`, `*=`, `/=`, `%=`)
- **arithmetic_unary**: Unary operators (`++`, `--`, unary `+`, `-`)
- **arithmetic_precedence**: Operator precedence testing
- **arithmetic_complex**: Multi-level expressions with parentheses

**7. Benefits**
- **Scalable**: Easy to add new layers without changing methodology
- **Flexible**: Can test incomplete pipelines (missing middle layers)
- **Maintainable**: Single source of truth for each layer transition
- **Comprehensive**: Full pipeline validation when all layers present

### Current Implementation Status
- **Layer 1 Testing**: ✅ Fully implemented with token dumping
- **Layer 2+ Testing**: 🔄 Infrastructure ready, awaiting layer implementation

---

**Remember: This project uses MANDATORY SCRIPT WORKFLOW. Always enhance scripts rather than bypassing them.**