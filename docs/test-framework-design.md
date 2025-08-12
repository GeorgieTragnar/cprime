# CPrime Compiler Test Framework Design

## Overview

This document describes the comprehensive CMake-generated TEST_F framework for automated testing of the CPrime compiler's layered architecture. The system automatically discovers layer functions, mirrors their implementation with intermediate state logging, and generates Google Test test cases.

## Core Concept

The framework operates by:
1. **CMake discovery** finds all layer functions and their sublayer call patterns
2. **Function content mirroring** copies production logic exactly with logging inserted
3. **TEST_F generation** creates individual Google Test cases for each test scenario
4. **String-based comparison** uses serialization for simple and reliable test validation
5. **Intermediate state capture** provides comprehensive debugging information on test failure

## Architecture & Naming Conventions

### Directory Structure
```
compiler/
├── src/
│   ├── commons/                    # All shared data structures
│   │   ├── rawToken.h
│   │   ├── processing_chunk.h
│   │   └── ...
│   ├── layerN/                     # Layer implementation
│   │   ├── tokenizer.h/.cpp        # Contains sublayerN functions
│   │   └── ...
│   └── layerNvalidation/           # Serialization support
│       ├── auto_serializers.h/.cpp
│       └── ...
├── test_cases/
│   ├── layerN/
│   │   ├── test_case_name/
│   │   │   ├── layerN              # Input data
│   │   │   └── layerN+1            # Expected output (serialized)
└── tests/
    ├── integration/
    │   ├── generated_layer_tests.cpp
    │   ├── layer_test_fixture.h/.cpp
    │   └── generated/               # CMake-generated files
    └── CMakeLists.txt
```

### Naming Convention Standards
```cpp
// Data structures (existing)
namespace commons {
    struct RawToken { ... };
    struct ProcessingChunk { ... };
    // ... all shared types
}

// Layer implementation (refactor existing to match)
namespace layerN {
    ReturnType sublayerNa(InputType input);
    ReturnType sublayerNb(const ReturnType& input, AdditionalParams...);
    // ... all sublayers as functions within layerN namespace
}

// Public API (global functions - what compiler actually calls)
FinalReturnType layerN(InputType input, AdditionalParams...) {
    auto resultNa = layerN::sublayerNa(input);
    auto resultNb = layerN::sublayerNb(resultNa, params...);
    // ... sequential sublayer calls
    return layerN::sublayerN_final(previous_result);
}
```

### Example: Layer1 Structure
```cpp
namespace commons {
    struct ProcessingChunk { ... };
    struct RawToken { ... };
}

namespace layer1 {
    std::vector<commons::ProcessingChunk> sublayer1a(std::stringstream& input);
    std::vector<commons::ProcessingChunk> sublayer1b(const std::vector<commons::ProcessingChunk>& input, StringTable& table);
    std::vector<commons::ProcessingChunk> sublayer1c(const std::vector<commons::ProcessingChunk>& input);
    std::vector<commons::ProcessingChunk> sublayer1d(const std::vector<commons::ProcessingChunk>& input);
    std::vector<commons::RawToken> sublayer1e(const std::vector<commons::ProcessingChunk>& input, StringTable& table);
}

// Global API function
std::vector<commons::RawToken> layer1(std::stringstream& input, StringTable& table) {
    auto result1a = layer1::sublayer1a(input);
    auto result1b = layer1::sublayer1b(result1a, table);
    auto result1c = layer1::sublayer1c(result1b);
    auto result1d = layer1::sublayer1d(result1c);
    return layer1::sublayer1e(result1d, table);
}
```

## Phase 1: CMake Function Discovery & Parsing

### 1.1 Function Discovery Module
**File**: `cmake/FunctionMirroringGenerator.cmake`

```cmake
# Function to discover all layer functions and their structure
function(discover_layer_functions SOURCE_DIR OUTPUT_REGISTRY)
    message(STATUS "Discovering layer functions in: ${SOURCE_DIR}")
    
    # Find all global layerN() function definitions
    file(GLOB_RECURSE SOURCE_FILES "${SOURCE_DIR}/*.cpp" "${SOURCE_DIR}/*.h")
    
    set(DISCOVERED_FUNCTIONS "")
    
    foreach(SOURCE_FILE ${SOURCE_FILES})
        file(READ ${SOURCE_FILE} FILE_CONTENT)
        
        # Regex to match global layerN() function signatures
        string(REGEX MATCHALL "([A-Za-z0-9_:<>,\\s]+)\\s+layer([0-9]+)\\s*\\([^{]*\\)\\s*\\{([^}]*)\\}" 
               FUNCTION_MATCHES "${FILE_CONTENT}")
        
        foreach(FUNCTION_MATCH ${FUNCTION_MATCHES})
            # Extract function details
            string(REGEX MATCH "([A-Za-z0-9_:<>,\\s]+)\\s+layer([0-9]+)\\s*\\(([^{]*)\\)" 
                   SIGNATURE_MATCH "${FUNCTION_MATCH}")
            
            if(SIGNATURE_MATCH)
                set(RETURN_TYPE "${CMAKE_MATCH_1}")
                set(LAYER_NUMBER "${CMAKE_MATCH_2}")
                set(PARAMETERS "${CMAKE_MATCH_3}")
                
                # Extract function body for sublayer call analysis
                string(REGEX MATCH "layer${LAYER_NUMBER}\\s*\\([^{]*\\)\\s*\\{([^}]*)\\}" 
                       BODY_MATCH "${FUNCTION_MATCH}")
                set(FUNCTION_BODY "${CMAKE_MATCH_1}")
                
                # Parse sublayer calls from function body
                string(REGEX MATCHALL "layer${LAYER_NUMBER}::sublayer${LAYER_NUMBER}[a-z]\\s*\\([^;]*\\)" 
                       SUBLAYER_CALLS "${FUNCTION_BODY}")
                
                # Store discovered function information
                list(APPEND DISCOVERED_FUNCTIONS 
                     "LAYER:${LAYER_NUMBER};RETURN:${RETURN_TYPE};PARAMS:${PARAMETERS};BODY:${FUNCTION_BODY};SUBLAYERS:${SUBLAYER_CALLS}")
            endif()
        endforeach()
    endforeach()
    
    # Write discovery results to output file
    string(REPLACE ";" "\n" FUNCTIONS_OUTPUT "${DISCOVERED_FUNCTIONS}")
    file(WRITE ${OUTPUT_REGISTRY} "${FUNCTIONS_OUTPUT}")
    
    message(STATUS "Discovered ${list(LENGTH DISCOVERED_FUNCTIONS)} layer functions")
endfunction()

# Function to extract complete function body with preserved formatting
function(extract_function_body FUNCTION_NAME SOURCE_FILE OUTPUT_CONTENT)
    file(READ ${SOURCE_FILE} FILE_CONTENT)
    
    # Find function definition with proper brace matching
    string(REGEX MATCH "${FUNCTION_NAME}\\s*\\([^{]*\\)\\s*\\{" START_MATCH "${FILE_CONTENT}")
    
    if(START_MATCH)
        string(FIND "${FILE_CONTENT}" "${START_MATCH}" START_POS)
        
        # Find matching closing brace (simplified - assumes proper nesting)
        set(BRACE_COUNT 0)
        set(CURRENT_POS ${START_POS})
        string(LENGTH "${FILE_CONTENT}" CONTENT_LENGTH)
        
        while(CURRENT_POS LESS ${CONTENT_LENGTH})
            string(SUBSTRING "${FILE_CONTENT}" ${CURRENT_POS} 1 CHAR)
            if(CHAR STREQUAL "{")
                math(EXPR BRACE_COUNT "${BRACE_COUNT} + 1")
            elseif(CHAR STREQUAL "}")
                math(EXPR BRACE_COUNT "${BRACE_COUNT} - 1")
                if(BRACE_COUNT EQUAL 0)
                    break()
                endif()
            endif()
            math(EXPR CURRENT_POS "${CURRENT_POS} + 1")
        endwhile()
        
        # Extract function body
        math(EXPR BODY_START "${START_POS} + string(LENGTH START_MATCH)")
        math(EXPR BODY_LENGTH "${CURRENT_POS} - ${BODY_START}")
        string(SUBSTRING "${FILE_CONTENT}" ${BODY_START} ${BODY_LENGTH} FUNCTION_BODY)
        
        file(WRITE ${OUTPUT_CONTENT} "${FUNCTION_BODY}")
    else()
        message(WARNING "Could not find function ${FUNCTION_NAME} in ${SOURCE_FILE}")
    endif()
endfunction()
```

### 1.2 Discovery Process
The CMake system will:
1. **Scan all source files** for global `layerN()` function patterns
2. **Extract function signatures** including return types and parameters
3. **Parse function bodies** to identify sublayer call sequences
4. **Extract variable names** and intermediate types used
5. **Build complete registry** of layer structure and data flow

## Phase 2: TEST_F Code Generation

### 2.1 Test Generation Module
**File**: `cmake/TestHarnessGenerator.cmake`

```cmake
# Generate TEST_F cases for all discovered layer functions and test cases
function(generate_layer_test_cases DISCOVERY_REGISTRY TEST_CASES_DIR OUTPUT_FILE)
    message(STATUS "Generating layer test cases")
    
    # Read discovery registry
    file(READ ${DISCOVERY_REGISTRY} DISCOVERY_CONTENT)
    string(REPLACE "\n" ";" DISCOVERED_FUNCTIONS "${DISCOVERY_CONTENT}")
    
    # Find all available test cases
    file(GLOB TEST_CASE_DIRS "${TEST_CASES_DIR}/layer*")
    
    set(GENERATED_TESTS "")
    string(APPEND GENERATED_TESTS "// Auto-generated by CMake - do not edit manually\n\n")
    string(APPEND GENERATED_TESTS "#include \"layer_test_fixture.h\"\n")
    string(APPEND GENERATED_TESTS "#include \"../layerNvalidation/auto_serializers.h\"\n\n")
    
    foreach(FUNCTION_INFO ${DISCOVERED_FUNCTIONS})
        # Parse function information
        string(REGEX MATCH "LAYER:([0-9]+)" LAYER_MATCH "${FUNCTION_INFO}")
        set(LAYER_NUM "${CMAKE_MATCH_1}")
        
        string(REGEX MATCH "RETURN:([^;]*)" RETURN_MATCH "${FUNCTION_INFO}")
        set(RETURN_TYPE "${CMAKE_MATCH_1}")
        
        string(REGEX MATCH "PARAMS:([^;]*)" PARAMS_MATCH "${FUNCTION_INFO}")
        set(PARAMETERS "${CMAKE_MATCH_1}")
        
        string(REGEX MATCH "BODY:([^;]*)" BODY_MATCH "${FUNCTION_INFO}")
        set(FUNCTION_BODY "${CMAKE_MATCH_1}")
        
        # Find test cases for this layer
        file(GLOB LAYER_TEST_CASES "${TEST_CASES_DIR}/layer${LAYER_NUM}/*")
        
        foreach(TEST_CASE_DIR ${LAYER_TEST_CASES})
            get_filename_component(TEST_CASE_NAME ${TEST_CASE_DIR} NAME)
            
            # Generate TEST_F for this combination
            string(APPEND GENERATED_TESTS "TEST_F(LayerTestFixture, Layer${LAYER_NUM}_${TEST_CASE_NAME}) {\n")
            string(APPEND GENERATED_TESTS "    // Setup\n")
            string(APPEND GENERATED_TESTS "    load_test_case(\"layer${LAYER_NUM}/${TEST_CASE_NAME}\");\n")
            string(APPEND GENERATED_TESTS "    StringTable string_table;\n")
            string(APPEND GENERATED_TESTS "    std::stringstream input = load_layer_input(\"layer${LAYER_NUM}\");\n\n")
            
            string(APPEND GENERATED_TESTS "    // === MIRROR layer${LAYER_NUM}() FUNCTION WITH LOGGING ===\n")
            
            # Transform function body to add logging
            generate_instrumented_function_body("${FUNCTION_BODY}" "${LAYER_NUM}" INSTRUMENTED_BODY)
            string(APPEND GENERATED_TESTS "${INSTRUMENTED_BODY}\n")
            
            string(APPEND GENERATED_TESTS "    // === TEST VALIDATION ===\n")
            string(APPEND GENERATED_TESTS "    std::string actual_serialized = auto_serialize(final_result);\n")
            string(APPEND GENERATED_TESTS "    std::string expected_content = load_expected_output(\"layer$((LAYER_NUM + 1))\");\n\n")
            
            string(APPEND GENERATED_TESTS "    if (actual_serialized == expected_content) {\n")
            string(APPEND GENERATED_TESTS "        clear_intermediate_buffers();\n")
            string(APPEND GENERATED_TESTS "        SUCCEED();\n")
            string(APPEND GENERATED_TESTS "    } else {\n")
            string(APPEND GENERATED_TESTS "        dump_failure_log(\"Layer${LAYER_NUM}_${TEST_CASE_NAME}\", get_all_intermediate_states(), actual_serialized, expected_content);\n")
            string(APPEND GENERATED_TESTS "        FAIL() << \"Layer${LAYER_NUM} output mismatch for ${TEST_CASE_NAME} test case\";\n")
            string(APPEND GENERATED_TESTS "    }\n")
            string(APPEND GENERATED_TESTS "}\n\n")
        endforeach()
    endforeach()
    
    # Write generated test file
    file(WRITE ${OUTPUT_FILE} "${GENERATED_TESTS}")
    message(STATUS "Generated test cases written to: ${OUTPUT_FILE}")
endfunction()

# Transform function body to add intermediate state logging
function(generate_instrumented_function_body ORIGINAL_BODY LAYER_NUM OUTPUT_VAR)
    set(INSTRUMENTED "")
    
    # Find sublayer call patterns and add logging after each
    string(REGEX MATCHALL "auto\\s+([a-zA-Z0-9_]+)\\s*=\\s*layer${LAYER_NUM}::sublayer${LAYER_NUM}([a-z]+)\\s*\\([^;]*\\);" 
           SUBLAYER_CALLS "${ORIGINAL_BODY}")
    
    set(REMAINING_BODY "${ORIGINAL_BODY}")
    
    foreach(SUBLAYER_CALL ${SUBLAYER_CALLS})
        string(REGEX MATCH "auto\\s+([a-zA-Z0-9_]+)\\s*=\\s*layer${LAYER_NUM}::sublayer${LAYER_NUM}([a-z]+)" 
               CALL_MATCH "${SUBLAYER_CALL}")
        set(RESULT_VAR "${CMAKE_MATCH_1}")
        set(SUBLAYER_NAME "${CMAKE_MATCH_2}")
        
        # Add the original call
        string(APPEND INSTRUMENTED "    ${SUBLAYER_CALL}\n")
        
        # Add intermediate state logging
        string(APPEND INSTRUMENTED "    log_intermediate_state(\"sublayer${LAYER_NUM}${SUBLAYER_NAME}\", auto_serialize(${RESULT_VAR}));\n\n")
    endforeach()
    
    # Handle final return statement
    string(REGEX REPLACE "return\\s+layer${LAYER_NUM}::" "auto final_result = layer${LAYER_NUM}::" INSTRUMENTED "${INSTRUMENTED}")
    
    set(${OUTPUT_VAR} "${INSTRUMENTED}" PARENT_SCOPE)
endfunction()
```

### 2.2 Generated Test Structure Example
**File**: `generated/layer_tests_generated.inc`
```cpp
// Auto-generated by CMake - do not edit manually

#include "layer_test_fixture.h"
#include "../layerNvalidation/auto_serializers.h"

TEST_F(LayerTestFixture, Layer1_HelloWorld) {
    // Setup
    load_test_case("layer1/hello_world");
    StringTable string_table;
    std::stringstream input = load_layer_input("layer1");
    
    // === MIRROR layer1() FUNCTION WITH LOGGING ===
    auto result1a = layer1::sublayer1a(input);
    log_intermediate_state("sublayer1a", auto_serialize(result1a));
    
    auto result1b = layer1::sublayer1b(result1a, string_table);
    log_intermediate_state("sublayer1b", auto_serialize(result1b));
    
    auto result1c = layer1::sublayer1c(result1b);
    log_intermediate_state("sublayer1c", auto_serialize(result1c));
    
    auto result1d = layer1::sublayer1d(result1c);
    log_intermediate_state("sublayer1d", auto_serialize(result1d));
    
    auto final_result = layer1::sublayer1e(result1d, string_table);
    
    // === TEST VALIDATION ===
    std::string actual_serialized = auto_serialize(final_result);
    std::string expected_content = load_expected_output("layer2");
    
    if (actual_serialized == expected_content) {
        clear_intermediate_buffers();
        SUCCEED();
    } else {
        dump_failure_log("Layer1_HelloWorld", get_all_intermediate_states(), actual_serialized, expected_content);
        FAIL() << "Layer1 output mismatch for HelloWorld test case";
    }
}

TEST_F(LayerTestFixture, Layer1_StringLiterals) {
    // Same pattern for string literals test case...
}

TEST_F(LayerTestFixture, Layer2_HelloWorld) {
    // Generated for layer2 using same pattern...
}

// ... more generated test cases
```

## Phase 3: Test Fixture & Infrastructure

### 3.1 Base Test Fixture
**File**: `tests/integration/layer_test_fixture.h`
```cpp
#pragma once

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <filesystem>

class LayerTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test environment
        intermediate_states.clear();
        current_test_case_path.clear();
    }
    
    void TearDown() override {
        // Clean up after test
        clear_intermediate_buffers();
    }
    
    // Test case management
    void load_test_case(const std::string& test_path) {
        current_test_case_path = get_test_cases_root() + "/" + test_path;
        if (!std::filesystem::exists(current_test_case_path)) {
            throw std::runtime_error("Test case path does not exist: " + current_test_case_path);
        }
    }
    
    std::stringstream load_layer_input(const std::string& layer_name) {
        std::string input_file = current_test_case_path + "/" + layer_name;
        std::ifstream file(input_file);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open input file: " + input_file);
        }
        
        std::stringstream ss;
        ss << file.rdbuf();
        return ss;
    }
    
    std::string load_expected_output(const std::string& layer_name) {
        std::string output_file = current_test_case_path + "/" + layer_name;
        std::ifstream file(output_file);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open expected output file: " + output_file);
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return content;
    }
    
    // Intermediate state logging
    void log_intermediate_state(const std::string& sublayer_name, const std::string& serialized_data) {
        intermediate_states.emplace_back(sublayer_name, serialized_data);
    }
    
    void clear_intermediate_buffers() {
        intermediate_states.clear();
    }
    
    std::vector<std::pair<std::string, std::string>> get_all_intermediate_states() {
        return intermediate_states;
    }
    
    // Failure reporting
    void dump_failure_log(const std::string& test_name, 
                         const std::vector<std::pair<std::string, std::string>>& states,
                         const std::string& actual_output,
                         const std::string& expected_output) {
        
        std::string log_dir = "logs/temp";
        std::filesystem::create_directories(log_dir);
        
        std::string log_file = log_dir + "/" + test_name + ".log";
        std::ofstream log(log_file);
        
        log << "=== LAYER TEST FAILURE LOG ===\n";
        log << "Test: " << test_name << "\n";
        log << "Timestamp: " << get_timestamp() << "\n\n";
        
        log << "=== INTERMEDIATE STATES ===\n";
        for (const auto& [sublayer, data] : states) {
            log << "--- " << sublayer << " ---\n";
            log << data << "\n\n";
        }
        
        log << "=== FINAL COMPARISON ===\n";
        log << "--- ACTUAL OUTPUT ---\n";
        log << actual_output << "\n\n";
        log << "--- EXPECTED OUTPUT ---\n";
        log << expected_output << "\n\n";
        
        log << "=== DIFF ANALYSIS ===\n";
        log << generate_diff(actual_output, expected_output) << "\n";
        
        std::cout << "Test failure log written to: " << log_file << std::endl;
    }

private:
    std::string current_test_case_path;
    std::vector<std::pair<std::string, std::string>> intermediate_states;
    
    std::string get_test_cases_root() {
        // Return path to test_cases directory
        return std::filesystem::current_path().parent_path() / "test_cases";
    }
    
    std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    std::string generate_diff(const std::string& actual, const std::string& expected) {
        // Simple line-by-line diff (could be enhanced)
        std::stringstream actual_stream(actual);
        std::stringstream expected_stream(expected);
        std::string actual_line, expected_line;
        std::stringstream diff;
        
        int line_num = 1;
        while (std::getline(actual_stream, actual_line) || std::getline(expected_stream, expected_line)) {
            if (actual_line != expected_line) {
                diff << "Line " << line_num << ":\n";
                diff << "- Expected: " << expected_line << "\n";
                diff << "+ Actual:   " << actual_line << "\n";
            }
            line_num++;
        }
        
        return diff.str();
    }
};
```

### 3.2 Auto-Serialization Support
**File**: `src/layerNvalidation/auto_serializers.h`
```cpp
#pragma once

#include "../commons/rawToken.h"
#include "../commons/processing_chunk.h"
#include "../commons/dirty/string_table.h"
#include <string>
#include <vector>
#include <type_traits>

namespace layerNvalidation {

// Automatic serialization for commons types
std::string auto_serialize(const std::vector<commons::ProcessingChunk>& chunks);
std::string auto_serialize(const std::vector<commons::RawToken>& tokens);
std::string auto_serialize(const commons::RawToken& token);
std::string auto_serialize(const commons::ProcessingChunk& chunk);

// Template for automatic serialization of any type
template<typename T>
std::string auto_serialize(const T& data) {
    static_assert(std::is_same_v<T, void>, "No serialization available for this type");
    return "";
}

// Specializations for known types will be generated by CMake discovery

}
```

**File**: `src/layerNvalidation/auto_serializers.cpp`
```cpp
#include "auto_serializers.h"
#include "token_serializer.h" // Existing serialization infrastructure
#include <sstream>

namespace layerNvalidation {

std::string auto_serialize(const std::vector<commons::ProcessingChunk>& chunks) {
    std::stringstream ss;
    for (size_t i = 0; i < chunks.size(); ++i) {
        ss << "CHUNK[" << i << "]: " << auto_serialize(chunks[i]);
        if (i < chunks.size() - 1) ss << "\n";
    }
    return ss.str();
}

std::string auto_serialize(const std::vector<commons::RawToken>& tokens) {
    // Use existing token serialization infrastructure
    return TokenSerializer::serialize_tokens(tokens, get_default_string_table());
}

std::string auto_serialize(const commons::RawToken& token) {
    return TokenSerializer::serialize(token, get_default_string_table());
}

std::string auto_serialize(const commons::ProcessingChunk& chunk) {
    std::stringstream ss;
    ss << "ProcessingChunk{";
    if (chunk.is_processed()) {
        ss << "token=" << auto_serialize(chunk.get_token());
    } else {
        ss << "string=\"" << escape_string(chunk.get_string()) << "\"";
    }
    ss << ", pos=" << chunk.start_pos << "-" << chunk.end_pos;
    ss << ", line=" << chunk.line << ", col=" << chunk.column;
    ss << "}";
    return ss.str();
}

// Helper functions
StringTable& get_default_string_table() {
    static StringTable table;
    return table;
}

std::string escape_string(const std::string& str) {
    std::stringstream ss;
    for (char c : str) {
        switch (c) {
            case '\n': ss << "\\n"; break;
            case '\t': ss << "\\t"; break;
            case '\r': ss << "\\r"; break;
            case '\\': ss << "\\\\"; break;
            case '"': ss << "\\\""; break;
            default: ss << c; break;
        }
    }
    return ss.str();
}

}
```

## Phase 4: CMake Integration

### 4.1 Main Build Integration
**File**: `tests/CMakeLists.txt` (additions)
```cmake
# Include generation modules
include(${CMAKE_SOURCE_DIR}/../cmake/FunctionMirroringGenerator.cmake)
include(${CMAKE_SOURCE_DIR}/../cmake/TestHarnessGenerator.cmake)

# Set paths
set(COMPILER_SRC_DIR "${CMAKE_SOURCE_DIR}/../compiler/src")
set(TEST_CASES_DIR "${CMAKE_SOURCE_DIR}/../test_cases")
set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")

# Create generated directory
file(MAKE_DIRECTORY ${GENERATED_DIR})

# Phase 1: Discover layer functions
set(DISCOVERY_REGISTRY "${CMAKE_BINARY_DIR}/layer_discovery.txt")
discover_layer_functions(${COMPILER_SRC_DIR} ${DISCOVERY_REGISTRY})

# Phase 2: Generate test cases
set(GENERATED_TESTS_FILE "${GENERATED_DIR}/layer_tests_generated.inc")
generate_layer_test_cases(${DISCOVERY_REGISTRY} ${TEST_CASES_DIR} ${GENERATED_TESTS_FILE})

# Build test executable with generated tests
add_executable(generated_layer_tests 
    integration/generated_layer_tests.cpp
    integration/layer_test_fixture.cpp
    ../compiler/src/layerNvalidation/auto_serializers.cpp
    ../compiler/src/layerNvalidation/token_serializer.cpp
    # Add other required source files
)

# Include paths
target_include_directories(generated_layer_tests PRIVATE 
    ${GENERATED_DIR}
    ${CMAKE_SOURCE_DIR}/../compiler/src
    ${CMAKE_SOURCE_DIR}/integration
)

# Link libraries
target_link_libraries(generated_layer_tests PRIVATE 
    GTest::gtest 
    GTest::gtest_main
    spdlog::spdlog
)

# Add to CTest
add_test(NAME GeneratedLayerTests COMMAND generated_layer_tests)
```

### 4.2 Custom Target for Regeneration
```cmake
# Custom target to regenerate tests when source changes
add_custom_target(regenerate_layer_tests
    COMMAND ${CMAKE_COMMAND} -E remove ${GENERATED_TESTS_FILE}
    COMMAND ${CMAKE_COMMAND} 
        -DCOMPILER_SRC_DIR=${COMPILER_SRC_DIR}
        -DTEST_CASES_DIR=${TEST_CASES_DIR}
        -DDISCOVERY_REGISTRY=${DISCOVERY_REGISTRY}
        -DGENERATED_TESTS_FILE=${GENERATED_TESTS_FILE}
        -P ${CMAKE_SOURCE_DIR}/../cmake/RegenerateTests.cmake
    COMMENT "Regenerating layer tests from current source"
)

# Make test executable depend on regeneration
add_dependencies(generated_layer_tests regenerate_layer_tests)
```

## Phase 5: Integration with Existing Workflow

### 5.1 Script Integration
**File**: `scripts/test_runner.sh` (enhancements)
```bash
# Enhanced test runner with generated test support

run_generated_layer_tests() {
    local layers="$1"
    
    echo "Running generated layer tests for layers: $layers"
    
    # Build generated tests
    if ! ./scripts/build.sh -ct; then
        echo "Failed to build generated layer tests"
        return 1
    fi
    
    # Run specific layer tests based on layer selection
    case "$layers" in
        "1"|"layer1")
            ./build/tests/generated_layer_tests --gtest_filter="Layer1_*"
            ;;
        "2"|"layer2") 
            ./build/tests/generated_layer_tests --gtest_filter="Layer2_*"
            ;;
        "1,2"|"1-2")
            ./build/tests/generated_layer_tests --gtest_filter="Layer[12]_*"
            ;;
        *)
            ./build/tests/generated_layer_tests
            ;;
    esac
}

# Add to main test runner logic
if [[ "$GENERATED_TESTS" == "true" ]]; then
    run_generated_layer_tests "$LAYER_SELECTION"
else
    # Fall back to existing manual test approach
    run_manual_layer_tests "$LAYER_SELECTION"
fi
```

### 5.2 Test Case Management
**File**: `scripts/validate_test_cases.py`
```python
#!/usr/bin/env python3
"""
Validate test case directory structure and file formats
"""
import os
import sys
from pathlib import Path

def validate_test_case_structure(test_cases_root):
    """Validate that test cases follow expected structure"""
    issues = []
    
    for layer_dir in Path(test_cases_root).glob("layer*"):
        if not layer_dir.is_dir():
            continue
            
        layer_num = layer_dir.name[5:]  # Extract number from "layerN"
        
        for test_case_dir in layer_dir.iterdir():
            if not test_case_dir.is_dir():
                continue
                
            # Check required files exist
            layer_input = test_case_dir / f"layer{layer_num}"
            expected_output = test_case_dir / f"layer{int(layer_num) + 1}"
            
            if not layer_input.exists():
                issues.append(f"Missing input file: {layer_input}")
                
            if not expected_output.exists():
                issues.append(f"Missing expected output file: {expected_output}")
                
            # Validate file formats (if serialization format is known)
            if expected_output.exists():
                try:
                    validate_serialization_format(expected_output)
                except Exception as e:
                    issues.append(f"Invalid serialization format in {expected_output}: {e}")
    
    return issues

def validate_serialization_format(file_path):
    """Validate that file contains proper serialization format"""
    with open(file_path, 'r') as f:
        content = f.read().strip()
        
    # Check for expected serialization patterns
    if content and not content.startswith('TOKEN:'):
        # Could be other valid format, add more validation as needed
        pass

if __name__ == "__main__":
    test_cases_root = sys.argv[1] if len(sys.argv) > 1 else "../test_cases"
    
    issues = validate_test_case_structure(test_cases_root)
    
    if issues:
        print("Test case validation issues found:")
        for issue in issues:
            print(f"  - {issue}")
        sys.exit(1)
    else:
        print("All test cases validated successfully")
        sys.exit(0)
```

## Benefits and Advantages

### Perfect Fidelity
- **Exact production logic**: Tests mirror actual compiler implementation perfectly
- **Automatic synchronization**: Changes to layer functions automatically update tests
- **No drift**: Tests can never become out of sync with implementation

### Comprehensive Coverage
- **Every test case automatically tested**: New test cases immediately get TEST_F implementations
- **Every intermediate state captured**: Complete visibility into data transformation pipeline  
- **Every layer follows same pattern**: Consistent testing approach across entire compiler

### Zero Maintenance
- **No manual test writing**: All tests generated from existing code structure
- **Self-updating**: Tests evolve with codebase changes
- **Automatic discovery**: New layers and sublayers automatically incorporated

### Superior Debugging
- **Intermediate state logging**: See exactly where processing fails
- **String-based comparison**: Simple, reliable test validation
- **Detailed failure logs**: Complete pipeline state available for analysis
- **Selective buffer integration**: Rich logging context for failures

### Google Test Integration
- **Standard TEST_F structure**: Familiar testing workflow
- **Individual test isolation**: Each test case runs independently
- **Parallel execution**: Tests can run concurrently
- **Rich reporting**: Standard GTest output with enhanced failure information

## Future Extensions

### Multi-Layer Testing
```cpp
// Generated tests for cross-layer validation
TEST_F(LayerTestFixture, Pipeline_Layer1_to_Layer3_HelloWorld) {
    // Run layer1 → layer2 → layer3 pipeline
    // Validate intermediate results and final output
}
```

### Performance Profiling
```cpp
// Generated performance tests
TEST_F(LayerPerformanceFixture, Layer1_Performance_Benchmark) {
    // Time each sublayer execution
    // Record performance metrics
    // Detect performance regressions
}
```

### Fuzzing Integration
```cpp
// Generated fuzz tests
TEST_F(LayerFuzzFixture, Layer1_Fuzz_RandomInput) {
    // Generate random valid input
    // Test layer robustness
    // Capture and classify failures
}
```

## Implementation Timeline

### Phase 1 (Week 1): Foundation
1. **Refactor existing code** to match naming conventions
2. **Implement basic CMake discovery** for function parsing  
3. **Create test fixture infrastructure**
4. **Build basic serialization support**

### Phase 2 (Week 2): Generation
1. **Implement test case generation** from discovered functions
2. **Create instrumented function body transformation**
3. **Integrate with Google Test framework**
4. **Test with existing layer1 structure**

### Phase 3 (Week 3): Integration
1. **Integrate with build system** and existing scripts
2. **Create test case validation tools**
3. **Enhance failure reporting and logging**
4. **Document usage patterns**

### Phase 4 (Week 4): Expansion
1. **Apply to additional layers** (layer2, layer3, etc.)
2. **Performance optimization** of generated tests
3. **Advanced features** (cross-layer testing, profiling)
4. **Complete documentation** and examples

## Conclusion

This test framework design provides a comprehensive, automated, and maintenance-free solution for testing the CPrime compiler's layered architecture. By mirroring production code exactly and inserting intermediate state logging, it ensures perfect fidelity while providing rich debugging information.

The CMake-based generation approach scales automatically with the codebase and requires zero maintenance once implemented. Each test case is isolated, comprehensive, and provides detailed failure analysis through selective buffer logging.

This design represents a significant advancement in compiler testing methodology, providing both immediate benefits for current development and a solid foundation for future expansion of the CPrime compiler architecture.