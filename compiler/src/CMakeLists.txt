# CPrime Compiler - Multi-Layer Architecture
cmake_minimum_required(VERSION 3.16)
project(cprime_compiler VERSION 1.0.0 LANGUAGES CXX)

# Create libraries for each layer with validation
add_library(cprime_layer1
    layer1/raw_token.cpp
    layer1/context_stack.cpp
)

add_library(cprime_layer1validation
    layer1validation/token_sequence_validator.cpp
)

add_library(cprime_layer2
    layer2/semantic_token.cpp
    layer2/semantic_translator.cpp
    layer2/contextual_token.cpp
)

add_library(cprime_layer2validation
    layer2validation/context_validator.cpp
)

add_library(cprime_layer3
    layer3/ast.cpp
    layer3/symbol_table.cpp
    layer3/ast_builder.cpp
)

add_library(cprime_layer3validation
    layer3validation/ast_structure_validator.cpp
)

add_library(cprime_layer4
    layer4/raii_injector.cpp
)

add_library(cprime_layer4validation
    layer4validation/raii_constraint_validator.cpp
)

# Main compiler library combining all layers
add_library(cprime_compiler
    validation_common.h
    validation_pipeline.cpp
)

# Set include directories for all libraries
target_include_directories(cprime_layer1 PUBLIC .)
target_include_directories(cprime_layer1validation PUBLIC .)
target_include_directories(cprime_layer2 PUBLIC .)
target_include_directories(cprime_layer2validation PUBLIC .)
target_include_directories(cprime_layer3 PUBLIC .)
target_include_directories(cprime_layer3validation PUBLIC .)
target_include_directories(cprime_layer4 PUBLIC .)
target_include_directories(cprime_layer4validation PUBLIC .)

# Link validation libraries to their respective layers
target_link_libraries(cprime_layer1validation cprime_layer1)
target_link_libraries(cprime_layer2validation cprime_layer2)
target_link_libraries(cprime_layer3validation cprime_layer3)
target_link_libraries(cprime_layer4validation cprime_layer4)

# Link all layers and validation libraries
target_link_libraries(cprime_compiler
    cprime_layer1 cprime_layer1validation
    cprime_layer2 cprime_layer2validation  
    cprime_layer3 cprime_layer3validation
    cprime_layer4 cprime_layer4validation
)

target_include_directories(cprime_compiler PUBLIC .)

# Compiler flags for all libraries
set(COMPILER_FLAGS 
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# Apply flags to all layer libraries
foreach(lib IN ITEMS cprime_layer1 cprime_layer1validation cprime_layer2 cprime_layer2validation 
                     cprime_layer3 cprime_layer3validation cprime_layer4 cprime_layer4validation cprime_compiler)
    target_compile_features(${lib} PUBLIC cxx_std_17)
    target_compile_options(${lib} PRIVATE ${COMPILER_FLAGS})
endforeach()

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

# RAII injector test
add_executable(test_raii_injector tests/test_raii_injector.cpp)
target_link_libraries(test_raii_injector cprime_compiler)

# Validation layers test
add_executable(test_validation_layers tests/test_validation_layers.cpp)
target_link_libraries(test_validation_layers cprime_compiler)

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