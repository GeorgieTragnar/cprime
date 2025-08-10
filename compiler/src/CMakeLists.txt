# CPrime Compiler - Orchestrator-Based Architecture
cmake_minimum_required(VERSION 3.16)
project(cprime_compiler VERSION 3.0.0 LANGUAGES CXX)

# Set C++17 as the standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Create orchestrator library (main coordination)
add_library(cprime_orchestrator
    orchestrator.h
    orchestrator.cpp
)

target_include_directories(cprime_orchestrator PUBLIC .)
target_link_libraries(cprime_orchestrator PUBLIC cprime_commons cprime_layer0)
target_compile_features(cprime_orchestrator PUBLIC cxx_std_17)

# Compiler flags for all libraries
set(COMPILER_FLAGS 
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# Apply flags to all libraries
foreach(lib IN ITEMS cprime_commons cprime_layer0 cprime_orchestrator)
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