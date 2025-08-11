# CPrime Compiler - Orchestrator-Based Architecture
cmake_minimum_required(VERSION 3.16)
project(cprime_compiler VERSION 3.0.0 LANGUAGES CXX)

# Set C++17 as the standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find magic_enum for layer1validation
find_package(magic_enum CONFIG REQUIRED)

# Create commons library (shared infrastructure)
add_library(cprime_commons
    commons/context.h
    commons/contextualToken.h
    commons/instruction.h
    commons/scope.h
    commons/rawToken.h
    commons/token.h
    commons/enum/token.h
    commons/enum/rawToken.h
    commons/enum/contextualToken.h
    commons/logger.h
    commons/logger.cpp
    commons/dirty/selective_buffer_sink.h
    commons/dirty/component_buffer_manager.h
    commons/dirty/component_buffer_manager.cpp
    commons/dirty/string_table.h
    commons/dirty/string_table.cpp
)

target_include_directories(cprime_commons PUBLIC .)
target_compile_features(cprime_commons PUBLIC cxx_std_17)
target_link_libraries(cprime_commons PUBLIC spdlog::spdlog)

# Create Layer 0 library (input processing)
add_library(cprime_layer0
    layer0/compilation_parameters.h
    layer0/input_processor.h
    layer0/input_processor.cpp
)

target_include_directories(cprime_layer0 PUBLIC .)
target_link_libraries(cprime_layer0 PUBLIC cprime_commons)
target_compile_features(cprime_layer0 PUBLIC cxx_std_17)

# Create Layer 1 library (tokenization)
add_library(cprime_layer1
    layer1/tokenizer.h
    layer1/tokenizer.cpp
)

target_include_directories(cprime_layer1 PUBLIC .)
target_link_libraries(cprime_layer1 PUBLIC cprime_commons)
target_compile_features(cprime_layer1 PUBLIC cxx_std_17)

# Create Layer 1 Validation library (debugging and testing support)
add_library(cprime_layer1validation
    layer1validation/enum_stringifier.h
    layer1validation/enum_stringifier.cpp
    layer1validation/chunk_serializer.h
    layer1validation/chunk_serializer.cpp
    layer1validation/token_serializer.h
    layer1validation/token_serializer.cpp
    layer1validation/layer1_debug.h
    layer1validation/layer1_debug.cpp
)

target_include_directories(cprime_layer1validation PUBLIC .)
target_link_libraries(cprime_layer1validation PUBLIC cprime_commons cprime_layer1 magic_enum::magic_enum)
target_compile_features(cprime_layer1validation PUBLIC cxx_std_17)

# Create orchestrator library (main coordination)
add_library(cprime_orchestrator
    orchestrator.h
    orchestrator.cpp
)

target_include_directories(cprime_orchestrator PUBLIC .)
target_link_libraries(cprime_orchestrator PUBLIC cprime_commons cprime_layer0 cprime_layer1)
target_compile_features(cprime_orchestrator PUBLIC cxx_std_17)

# Create layer0validation library (debug support for Layer 0)
add_library(cprime_layer0validation
    layer0validation/input_debug.h
    layer0validation/input_debug.cpp
    layer0validation/stream_inspector.h
    layer0validation/stream_inspector.cpp
)

target_include_directories(cprime_layer0validation PUBLIC .)
target_link_libraries(cprime_layer0validation PUBLIC cprime_commons cprime_layer0)
target_compile_features(cprime_layer0validation PUBLIC cxx_std_17)

# Compiler flags for all libraries
set(COMPILER_FLAGS 
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# Apply flags to all libraries
foreach(lib IN ITEMS cprime_commons cprime_layer0 cprime_layer1 cprime_layer1validation cprime_orchestrator cprime_layer0validation)
    target_compile_options(${lib} PRIVATE ${COMPILER_FLAGS})
endforeach()

# Main compiler executable
add_executable(cprime main.cpp)
target_link_libraries(cprime cprime_orchestrator)
target_compile_features(cprime PUBLIC cxx_std_17)
target_compile_options(cprime PRIVATE ${COMPILER_FLAGS})

set_target_properties(cprime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# CLI development tool executable
add_executable(cprime_cli 
    cli/cprime_cli.cpp
    cli/cli_options.cpp
    cli/cli_options.h
)
target_link_libraries(cprime_cli cprime_layer0validation cprime_layer1validation cprime_orchestrator)
target_compile_features(cprime_cli PUBLIC cxx_std_17)
target_compile_options(cprime_cli PRIVATE ${COMPILER_FLAGS})

set_target_properties(cprime_cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)