# CPrime Compiler - Multi-Layer Architecture
cmake_minimum_required(VERSION 3.16)
project(cprime_compiler VERSION 1.0.0 LANGUAGES CXX)

# Create a library for the compiler components
add_library(cprime_compiler
    layer1/raw_token.cpp
    layer1/context_stack.cpp
    layer2/semantic_token.cpp
    layer2/semantic_translator.cpp
    layer2/contextual_token.cpp
    layer3/ast.cpp
    layer3/symbol_table.cpp
    layer3/ast_builder.cpp
)

target_include_directories(cprime_compiler PUBLIC .)

# Compiler flags
target_compile_features(cprime_compiler PUBLIC cxx_std_17)
target_compile_options(cprime_compiler PRIVATE 
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# CLI executable
add_executable(cprime_cli cli/cprime_cli.cpp)
target_link_libraries(cprime_cli cprime_compiler)

# Test executable
add_executable(test_three_layer tests/test_three_layer.cpp)
target_link_libraries(test_three_layer cprime_compiler)

# Contextual tokens demo
add_executable(test_contextual_tokens tests/test_contextual_tokens.cpp)
target_link_libraries(test_contextual_tokens cprime_compiler)

# AST builder test
add_executable(test_ast_builder tests/test_ast_builder.cpp)
target_link_libraries(test_ast_builder cprime_compiler)

# Optional: Add to parent project if called from main CMakeLists.txt
if(NOT CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # We're being included from the main project
    set_target_properties(cprime_compiler PROPERTIES 
        FOLDER "compiler"
    )
    set_target_properties(cprime_cli PROPERTIES 
        FOLDER "compiler"
    )
    set_target_properties(test_three_layer PROPERTIES 
        FOLDER "compiler/tests"
    )
endif()