# CPrime Compiler - Multi-Layer Architecture
cmake_minimum_required(VERSION 3.16)
project(cprime_compiler VERSION 1.0.0 LANGUAGES CXX)

# Create common library first (contains logger and shared types)
add_library(cprime_common
    common/logger.h
    common/validation_types.h
    common/common_types.h
    common/component_buffer_manager.h
    common/component_buffer_manager.cpp
    common/selective_buffer_sink.h
    common/logger_components.h
    common/string_table.h
    common/string_table.cpp
    common/token_types.h
    common/parse_context.h
    common/tokens.h
    common/token_streams.h
    common/token_utils.h
    common/token_utils.cpp
)

# Common library is header-only, but we need to link spdlog
target_include_directories(cprime_common PUBLIC .)
target_link_libraries(cprime_common PUBLIC spdlog::spdlog)

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

# Set include directories and link common library for all layers
target_include_directories(cprime_layer1 PUBLIC .)
target_link_libraries(cprime_layer1 PUBLIC cprime_common)

target_include_directories(cprime_layer1validation PUBLIC .)
target_link_libraries(cprime_layer1validation PUBLIC cprime_common cprime_layer1)

target_include_directories(cprime_layer2 PUBLIC .)
target_link_libraries(cprime_layer2 PUBLIC cprime_common)

target_include_directories(cprime_layer2validation PUBLIC .)
target_link_libraries(cprime_layer2validation PUBLIC cprime_common cprime_layer2)

target_include_directories(cprime_layer3 PUBLIC .)
target_link_libraries(cprime_layer3 PUBLIC cprime_common)

target_include_directories(cprime_layer3validation PUBLIC .)
target_link_libraries(cprime_layer3validation PUBLIC cprime_common cprime_layer3)

target_include_directories(cprime_layer4 PUBLIC .)
target_link_libraries(cprime_layer4 PUBLIC cprime_common)

target_include_directories(cprime_layer4validation PUBLIC .)
target_link_libraries(cprime_layer4validation PUBLIC cprime_common cprime_layer4)

# Layer 2 depends on Layer 1 for ContextStack and ContextResolver
target_link_libraries(cprime_layer2 PUBLIC cprime_layer1)
# Layer 3 depends on Layer 2 for semantic tokens
target_link_libraries(cprime_layer3 PUBLIC cprime_layer2)
# Layer 4 depends on Layer 3 for AST
target_link_libraries(cprime_layer4 PUBLIC cprime_layer3)

# Link all layers and validation libraries
target_link_libraries(cprime_compiler
    cprime_common
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

# Apply flags to all layer libraries (including common)
foreach(lib IN ITEMS cprime_common cprime_layer1 cprime_layer1validation cprime_layer2 cprime_layer2validation 
                     cprime_layer3 cprime_layer3validation cprime_layer4 cprime_layer4validation cprime_compiler)
    target_compile_features(${lib} PUBLIC cxx_std_17)
    target_compile_options(${lib} PRIVATE ${COMPILER_FLAGS})
endforeach()

# Main compiler executable
add_executable(cprime main.cpp)
target_link_libraries(cprime cprime_compiler)
set_target_properties(cprime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# CLI executable for layer testing
add_executable(cprime_cli cli/cprime_cli.cpp)
target_link_libraries(cprime_cli cprime_compiler)

# NOTE: Individual test executables have been migrated to Google Test
# Tests are now located in ../tests/ and built as cprime_tests
# 
# To run tests:
#   make cprime_tests && ./cprime_tests
# Or:
#   make run_tests

# Optional: Add to parent project if called from main CMakeLists.txt
if(NOT CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # We're being included from the main project
    set_target_properties(cprime_compiler PROPERTIES 
        FOLDER "compiler"
    )
    set_target_properties(cprime_cli PROPERTIES 
        FOLDER "compiler"
    )
    # Note: Individual test executables removed - now using Google Test
endif()