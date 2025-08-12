# CPrime Function Discovery Module
# Discovers layer functions and their sublayer call structure for automatic test generation
# Based on the design documented in docs/test-framework-design.md

cmake_minimum_required(VERSION 3.16)

# Function to discover all layer functions in the codebase
function(discover_layer_functions SOURCE_DIR OUTPUT_REGISTRY)
    message(STATUS "Discovering layer functions in: ${SOURCE_DIR}")
    
    # Find all source files to scan
    file(GLOB_RECURSE SOURCE_FILES 
         "${SOURCE_DIR}/*.cpp" 
         "${SOURCE_DIR}/*.h" 
         "${SOURCE_DIR}/*.hpp")
    
    set(DISCOVERED_FUNCTIONS "")
    set(TOTAL_FUNCTIONS_FOUND 0)
    
    foreach(SOURCE_FILE ${SOURCE_FILES})
        # Skip files that are not in layer directories for now
        get_filename_component(FILE_DIR ${SOURCE_FILE} DIRECTORY)
        get_filename_component(DIR_NAME ${FILE_DIR} NAME)
        
        # Only scan layer directories (layer1, layer2, etc.)
        if(NOT DIR_NAME MATCHES "^layer[0-9]+$")
            continue()
        endif()
        
        message(STATUS "  Scanning: ${SOURCE_FILE}")
        
        file(READ ${SOURCE_FILE} FILE_CONTENT)
        
        # Look for layer function patterns - start simple
        if(FILE_CONTENT MATCHES "layer([0-9]+)\\s*\\(")
            message(STATUS "    Found potential layer function in file")
            
            # Extract layer numbers found
            string(REGEX MATCHALL "layer([0-9]+)\\s*\\(" LAYER_MATCHES "${FILE_CONTENT}")
            
            foreach(LAYER_MATCH ${LAYER_MATCHES})
                string(REGEX MATCH "layer([0-9]+)" LAYER_NUM_MATCH "${LAYER_MATCH}")
                if(LAYER_NUM_MATCH)
                    set(LAYER_NUMBER "${CMAKE_MATCH_1}")
                    message(STATUS "      Found layer${LAYER_NUMBER} function")
                    
                    # For now, create a simple entry - we'll enhance this later
                    set(FUNCTION_BODY "// Function body placeholder")
                    set(SUBLAYER_CALLS "")
                    
                    # Try to extract sublayer calls
                    string(REGEX MATCHALL "layer${LAYER_NUMBER}_sublayers::sublayer${LAYER_NUMBER}[a-z]+" SUBLAYER_MATCHES "${FILE_CONTENT}")
                    foreach(SUBLAYER_MATCH ${SUBLAYER_MATCHES})
                        list(APPEND SUBLAYER_CALLS "${SUBLAYER_MATCH}")
                    endforeach()
                    
                    # Store basic function info for now
                    string(REPLACE ";" "," SUBLAYER_CALLS_STR "${SUBLAYER_CALLS}")
                    list(APPEND DISCOVERED_FUNCTIONS 
                         "LAYER:${LAYER_NUMBER};RETURN:std::vector<RawToken>;PARAMS:std::stringstream& stream, StringTable& string_table;BODY:${FUNCTION_BODY};SUBLAYERS:${SUBLAYER_CALLS_STR};FILE:${SOURCE_FILE}")
                    
                    math(EXPR TOTAL_FUNCTIONS_FOUND "${TOTAL_FUNCTIONS_FOUND} + 1")
                endif()
            endforeach()
        endif()
        
    endforeach()
    
    # Write discovery results to output file
    if(DISCOVERED_FUNCTIONS)
        string(REPLACE ";" "\n" FUNCTIONS_OUTPUT "${DISCOVERED_FUNCTIONS}")
        file(WRITE ${OUTPUT_REGISTRY} "${FUNCTIONS_OUTPUT}")
        message(STATUS "Discovery complete: Found ${TOTAL_FUNCTIONS_FOUND} layer functions")
        message(STATUS "Results written to: ${OUTPUT_REGISTRY}")
    else()
        file(WRITE ${OUTPUT_REGISTRY} "# No layer functions discovered")
        message(WARNING "No layer functions found in ${SOURCE_DIR}")
    endif()
endfunction()

# Helper function to extract sublayer calls from function body
function(extract_sublayer_calls FUNCTION_BODY LAYER_NUMBER OUTPUT_VAR)
    set(SUBLAYER_CALLS "")
    
    # Look for patterns like: var = layerN_sublayers::sublayerNx(...)
    # or: layerN_sublayers::sublayerNx(...)
    string(REGEX MATCHALL 
           "([a-zA-Z_][a-zA-Z0-9_]*\\s*=\\s*)?layer${LAYER_NUMBER}_sublayers::sublayer${LAYER_NUMBER}([a-z]+)\\s*\\([^;)]*\\)" 
           CALL_MATCHES "${FUNCTION_BODY}")
    
    foreach(CALL_MATCH ${CALL_MATCHES})
        # Extract variable name and sublayer name
        string(REGEX MATCH 
               "([a-zA-Z_][a-zA-Z0-9_]*)\\s*=\\s*layer${LAYER_NUMBER}_sublayers::sublayer${LAYER_NUMBER}([a-z]+)" 
               VAR_MATCH "${CALL_MATCH}")
        
        if(VAR_MATCH)
            set(VAR_NAME "${CMAKE_MATCH_1}")
            set(SUBLAYER_NAME "${CMAKE_MATCH_2}")
            list(APPEND SUBLAYER_CALLS "${VAR_NAME}=sublayer${LAYER_NUMBER}${SUBLAYER_NAME}")
        else()
            # Handle case without variable assignment
            string(REGEX MATCH 
                   "layer${LAYER_NUMBER}_sublayers::sublayer${LAYER_NUMBER}([a-z]+)" 
                   DIRECT_MATCH "${CALL_MATCH}")
            if(DIRECT_MATCH)
                set(SUBLAYER_NAME "${CMAKE_MATCH_1}")
                list(APPEND SUBLAYER_CALLS "sublayer${LAYER_NUMBER}${SUBLAYER_NAME}")
            endif()
        endif()
    endforeach()
    
    set(${OUTPUT_VAR} "${SUBLAYER_CALLS}" PARENT_SCOPE)
endfunction()

# Function to extract complete function body with preserved formatting
function(extract_function_body FUNCTION_NAME SOURCE_FILE OUTPUT_FILE)
    message(STATUS "Extracting function body for ${FUNCTION_NAME} from ${SOURCE_FILE}")
    
    file(READ ${SOURCE_FILE} FILE_CONTENT)
    
    # Find function definition
    string(REGEX MATCH "${FUNCTION_NAME}\\s*\\([^{]*\\)\\s*\\{" START_MATCH "${FILE_CONTENT}")
    
    if(START_MATCH)
        string(FIND "${FILE_CONTENT}" "${START_MATCH}" START_POS)
        
        # Find matching closing brace using simple counting
        string(LENGTH "${START_MATCH}" START_LENGTH)
        math(EXPR BODY_START "${START_POS} + ${START_LENGTH}")
        
        set(BRACE_COUNT 1)
        set(CURRENT_POS ${BODY_START})
        string(LENGTH "${FILE_CONTENT}" CONTENT_LENGTH)
        
        while(CURRENT_POS LESS ${CONTENT_LENGTH} AND BRACE_COUNT GREATER 0)
            string(SUBSTRING "${FILE_CONTENT}" ${CURRENT_POS} 1 CHAR)
            if(CHAR STREQUAL "{")
                math(EXPR BRACE_COUNT "${BRACE_COUNT} + 1")
            elseif(CHAR STREQUAL "}")
                math(EXPR BRACE_COUNT "${BRACE_COUNT} - 1")
            endif()
            math(EXPR CURRENT_POS "${CURRENT_POS} + 1")
        endwhile()
        
        if(BRACE_COUNT EQUAL 0)
            math(EXPR BODY_END "${CURRENT_POS} - 1")
            math(EXPR BODY_LENGTH "${BODY_END} - ${BODY_START}")
            string(SUBSTRING "${FILE_CONTENT}" ${BODY_START} ${BODY_LENGTH} FUNCTION_BODY)
            
            file(WRITE ${OUTPUT_FILE} "${FUNCTION_BODY}")
            message(STATUS "Function body extracted to: ${OUTPUT_FILE}")
        else()
            message(WARNING "Could not find matching closing brace for function ${FUNCTION_NAME}")
        endif()
    else()
        message(WARNING "Could not find function ${FUNCTION_NAME} in ${SOURCE_FILE}")
    endif()
endfunction()

# Function to validate discovery output
function(validate_discovery_output DISCOVERY_FILE)
    if(NOT EXISTS ${DISCOVERY_FILE})
        message(FATAL_ERROR "Discovery file not found: ${DISCOVERY_FILE}")
    endif()
    
    file(READ ${DISCOVERY_FILE} DISCOVERY_CONTENT)
    
    if(NOT DISCOVERY_CONTENT OR DISCOVERY_CONTENT MATCHES "^#.*No layer functions")
        message(WARNING "No valid layer functions found in discovery")
        return()
    endif()
    
    # Count discovered functions
    string(REGEX MATCHALL "LAYER:[0-9]+" LAYER_MATCHES "${DISCOVERY_CONTENT}")
    list(LENGTH LAYER_MATCHES FUNCTION_COUNT)
    
    message(STATUS "Discovery validation: ${FUNCTION_COUNT} functions found")
    
    # Validate each function entry
    string(REPLACE "\n" ";" FUNCTION_ENTRIES "${DISCOVERY_CONTENT}")
    foreach(ENTRY ${FUNCTION_ENTRIES})
        if(ENTRY MATCHES "^LAYER:")
            # Validate required fields
            if(NOT ENTRY MATCHES "LAYER:[0-9]+.*RETURN:.*PARAMS:.*BODY:.*SUBLAYERS:")
                message(WARNING "Invalid function entry format: ${ENTRY}")
            endif()
        endif()
    endforeach()
endfunction()