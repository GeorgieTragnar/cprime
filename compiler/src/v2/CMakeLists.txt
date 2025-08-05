# CPrime Compiler V2 - Three-Layer Architecture
cmake_minimum_required(VERSION 3.16)
project(cprime_v2 VERSION 0.1.0 LANGUAGES CXX)

# Create a library for the V2 components
add_library(cprime_v2
    raw_token.cpp
    context_stack.cpp
    semantic_token.cpp
    semantic_translator.cpp
)

target_include_directories(cprime_v2 PUBLIC .)

# Compiler flags
target_compile_features(cprime_v2 PUBLIC cxx_std_17)
target_compile_options(cprime_v2 PRIVATE 
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# CLI executable
add_executable(cprime_v2_cli cprime_v2_cli.cpp)
target_link_libraries(cprime_v2_cli cprime_v2)

# Test executable
add_executable(test_three_layer test_three_layer.cpp)
target_link_libraries(test_three_layer cprime_v2)

# Optional: Add to parent project if called from main CMakeLists.txt
if(NOT CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # We're being included from the main project
    set_target_properties(cprime_v2 PROPERTIES 
        FOLDER "compiler/v2"
    )
    set_target_properties(cprime_v2_cli PROPERTIES 
        FOLDER "compiler/v2"
    )
    set_target_properties(test_three_layer PROPERTIES 
        FOLDER "compiler/v2/tests"
    )
endif()