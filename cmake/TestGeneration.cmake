# CPrime Test Generation Module
# Generates 2 files: includes file + test cases file with copied function bodies
# Based on simplified approach focusing on exact function copying

cmake_minimum_required(VERSION 3.16)

# Generate includes file with correct layerN.h includes
function(generate_includes_file DISCOVERY_REGISTRY OUTPUT_FILE)
    message(STATUS "Generating includes file")
    
    file(READ ${DISCOVERY_REGISTRY} DISCOVERY_CONTENT)
    
    if(NOT DISCOVERY_CONTENT OR DISCOVERY_CONTENT MATCHES "^#.*No layer functions")
        file(WRITE ${OUTPUT_FILE} "// No layer functions discovered\n")
        return()
    endif()
    
    set(INCLUDES_CONTENT "")
    string(APPEND INCLUDES_CONTENT "// Auto-generated includes - DO NOT EDIT MANUALLY\n")
    string(APPEND INCLUDES_CONTENT "// Generated from: ${DISCOVERY_REGISTRY}\n\n")
    
    # Extract unique layers and generate includes
    string(REGEX MATCHALL "LAYER:([0-9]+)" LAYER_MATCHES "${DISCOVERY_CONTENT}")
    set(UNIQUE_LAYERS "")
    
    foreach(LAYER_MATCH ${LAYER_MATCHES})
        string(REGEX MATCH "LAYER:([0-9]+)" EXTRACTED_LAYER "${LAYER_MATCH}")
        if(EXTRACTED_LAYER)
            set(LAYER_NUM "${CMAKE_MATCH_1}")
            list(APPEND UNIQUE_LAYERS ${LAYER_NUM})
        endif()
    endforeach()
    
    # Remove duplicates
    if(UNIQUE_LAYERS)
        list(REMOVE_DUPLICATES UNIQUE_LAYERS)
        list(SORT UNIQUE_LAYERS)
    endif()
    
    # Generate include statements
    foreach(LAYER_NUM ${UNIQUE_LAYERS})
        string(APPEND INCLUDES_CONTENT "#include \"../src/layer${LAYER_NUM}/layer${LAYER_NUM}.h\"\n")
        string(APPEND INCLUDES_CONTENT "#include \"../src/layer${LAYER_NUM}validation/layer${LAYER_NUM}validation.h\"\n")
    endforeach()
    
    file(WRITE ${OUTPUT_FILE} "${INCLUDES_CONTENT}")
    message(STATUS "Generated includes file: ${OUTPUT_FILE}")
endfunction()

# Generate instrumented functions file with fully copied function bodies (no TEST_F blocks)
function(generate_instrumented_functions_file DISCOVERY_REGISTRY OUTPUT_FILE)
    message(STATUS "Generating instrumented functions file with copied function bodies")
    
    file(READ ${DISCOVERY_REGISTRY} DISCOVERY_CONTENT)
    
    if(NOT DISCOVERY_CONTENT OR DISCOVERY_CONTENT MATCHES "^#.*No layer functions")
        file(WRITE ${OUTPUT_FILE} "// No layer functions discovered for test generation\n")
        return()
    endif()
    
    set(INSTRUMENTED_FUNCTIONS_CONTENT "")
    string(APPEND INSTRUMENTED_FUNCTIONS_CONTENT "// Auto-generated instrumented functions - DO NOT EDIT MANUALLY\n")
    string(APPEND INSTRUMENTED_FUNCTIONS_CONTENT "// Generated from: ${DISCOVERY_REGISTRY}\n\n")
    string(APPEND INSTRUMENTED_FUNCTIONS_CONTENT "namespace instrumented_layers {\n\n")
    
    # Parse discovered functions - each function spans multiple lines
    string(REPLACE "\n" ";" DISCOVERY_LINES "${DISCOVERY_CONTENT}")
    
    set(CURRENT_LAYER "")
    set(CURRENT_RETURN "")
    set(CURRENT_PARAMS "")
    set(CURRENT_FILE "")
    
    foreach(LINE ${DISCOVERY_LINES})
        if(LINE MATCHES "^LAYER:([0-9]+)")
            # If we have a complete function, process it
            if(CURRENT_LAYER AND CURRENT_RETURN AND CURRENT_PARAMS AND CURRENT_FILE)
                process_discovered_function("${CURRENT_LAYER}" "${CURRENT_RETURN}" "${CURRENT_PARAMS}" "${CURRENT_FILE}" INSTRUMENTED_FUNCTIONS_CONTENT)
            endif()
            
            # Start new function
            set(CURRENT_LAYER "${CMAKE_MATCH_1}")
            set(CURRENT_RETURN "")
            set(CURRENT_PARAMS "")
            set(CURRENT_FILE "")
            
        elseif(LINE MATCHES "^RETURN:(.*)") 
            set(CURRENT_RETURN "${CMAKE_MATCH_1}")
            
        elseif(LINE MATCHES "^PARAMS:(.*)")
            set(CURRENT_PARAMS "${CMAKE_MATCH_1}")
            
        elseif(LINE MATCHES "^FILE:(.*)")
            set(CURRENT_FILE "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    
    # Process the last function
    if(CURRENT_LAYER AND CURRENT_RETURN AND CURRENT_PARAMS AND CURRENT_FILE)
        process_discovered_function("${CURRENT_LAYER}" "${CURRENT_RETURN}" "${CURRENT_PARAMS}" "${CURRENT_FILE}" INSTRUMENTED_FUNCTIONS_CONTENT)
    endif()
    
    # Close namespace
    string(APPEND INSTRUMENTED_FUNCTIONS_CONTENT "\n} // namespace instrumented_layers\n")
    
    file(WRITE ${OUTPUT_FILE} "${INSTRUMENTED_FUNCTIONS_CONTENT}")
    message(STATUS "Generated instrumented functions file: ${OUTPUT_FILE}")
endfunction()

# Helper function to process a discovered function
function(process_discovered_function LAYER_NUM RETURN_TYPE PARAMETERS SOURCE_FILE OUTPUT_VAR)
    set(CURRENT_CONTENT "${${OUTPUT_VAR}}")
    
    # Only process .cpp files for function body extraction (skip .h files)
    get_filename_component(FILE_EXT "${SOURCE_FILE}" EXT)
    if(NOT FILE_EXT STREQUAL ".cpp")
        message(STATUS "  Skipping ${SOURCE_FILE} (not a .cpp file)")
        set(${OUTPUT_VAR} "${CURRENT_CONTENT}" PARENT_SCOPE)
        return()
    endif()
    
    message(STATUS "  Processing layer${LAYER_NUM} from ${SOURCE_FILE}")
    message(STATUS "    LAYER_NUM: '${LAYER_NUM}'")
    message(STATUS "    RETURN_TYPE: '${RETURN_TYPE}'")
    message(STATUS "    PARAMETERS: '${PARAMETERS}'")
    message(STATUS "    SOURCE_FILE: '${SOURCE_FILE}'")
    
    # Extract full function body from source file
    extract_complete_function_body("layer${LAYER_NUM}" "${SOURCE_FILE}" FUNCTION_BODY)
    
    if(FUNCTION_BODY)
        # Split function body into code blocks separated by sublayer calls
        split_function_body_at_sublayers("${FUNCTION_BODY}" "${LAYER_NUM}" CODE_BLOCKS)
        
        # Generate instrumented function with separated code blocks
        string(APPEND CURRENT_CONTENT "${RETURN_TYPE} execute_layer${LAYER_NUM}_instrumented(${PARAMETERS}) {\n")
        string(APPEND CURRENT_CONTENT "    // === INSTRUMENTED layer${LAYER_NUM} FUNCTION ===\n")
        
        # Insert each code block with instrumentation
        list(LENGTH CODE_BLOCKS BLOCK_COUNT)
        math(EXPR LAST_BLOCK "${BLOCK_COUNT} - 1")
        
        foreach(BLOCK_INDEX RANGE ${LAST_BLOCK})
            list(GET CODE_BLOCKS ${BLOCK_INDEX} CODE_BLOCK)
            
            # Check if this block contains a return statement
            string(FIND "${CODE_BLOCK}" "return " HAS_RETURN)
            
            if(HAS_RETURN GREATER -1)
                # This block contains the return statement - replace it
                message(STATUS "      Found return statement in block ${BLOCK_INDEX}")
                
                # Split the block into lines and process each line
                string(REPLACE "\n" ";" BLOCK_LINES "${CODE_BLOCK}")
                set(PROCESSED_LINES "")
                
                foreach(LINE ${BLOCK_LINES})
                    if(LINE MATCHES ".*return\\s+.*")
                        # Manually replace "return layer" with "auto retVal = cprime::layer"
                        string(REPLACE "return layer" "auto retVal = cprime::layer" PROCESSED_LINE "${LINE}")
                        message(STATUS "        Replacing line: '${LINE}' -> '${PROCESSED_LINE}'")
                        list(APPEND PROCESSED_LINES "${PROCESSED_LINE};")
                    else()
                        list(APPEND PROCESSED_LINES "${LINE}")
                    endif()
                endforeach()
                
                string(REPLACE ";" "\n" PROCESSED_BLOCK "${PROCESSED_LINES}")
                
                string(APPEND CURRENT_CONTENT "\n    // --- Code Block ${BLOCK_INDEX} (Final) ---\n")
                string(APPEND CURRENT_CONTENT "${PROCESSED_BLOCK}")
                string(APPEND CURRENT_CONTENT "\n    log_intermediate_state(\"final_result\", layer${LAYER_NUM}_sublayers::validation::serialize(retVal));\n")
            else()
                # Regular block - extract variable name from the sublayer call line itself
                string(APPEND CURRENT_CONTENT "\n    // --- Code Block ${BLOCK_INDEX} ---\n")
                string(APPEND CURRENT_CONTENT "${CODE_BLOCK}")
                
                if(BLOCK_INDEX LESS ${LAST_BLOCK})
                    # Use standardized retVal1, retVal2, retVal3, etc. naming pattern
                    math(EXPR VAR_NUMBER "${BLOCK_INDEX} + 1")
                    set(VARIABLE_NAME "retVal${VAR_NUMBER}")
                    
                    message(STATUS "      Using variable name: '${VARIABLE_NAME}' for block ${BLOCK_INDEX}")
                    string(APPEND CURRENT_CONTENT "\n    log_intermediate_state(\"${VARIABLE_NAME}\", layer${LAYER_NUM}_sublayers::validation::serialize(${VARIABLE_NAME}));\n")
                endif()
            endif()
        endforeach()
        
        string(APPEND CURRENT_CONTENT "\n    // === END INSTRUMENTED FUNCTION ===\n")
        string(APPEND CURRENT_CONTENT "    return retVal;\n")
        string(APPEND CURRENT_CONTENT "}\n\n")
    else()
        message(WARNING "Could not extract function body for layer${LAYER_NUM}")
        string(APPEND CURRENT_CONTENT "// WARNING: Could not extract layer${LAYER_NUM} function body\n\n")
    endif()
    
    set(${OUTPUT_VAR} "${CURRENT_CONTENT}" PARENT_SCOPE)
endfunction()

# Function to split function body at sublayer calls and add cprime:: prefix
function(split_function_body_at_sublayers FUNCTION_BODY LAYER_NUM OUTPUT_VAR)
    message(STATUS "    Splitting function body at sublayer calls for layer${LAYER_NUM}")
    
    # Split the body by newlines to process line by line
    string(REPLACE "\n" ";" BODY_LINES "${FUNCTION_BODY}")
    
    set(CODE_BLOCKS "")
    set(CURRENT_BLOCK "")
    set(BLOCK_COUNT 0)
    
    foreach(LINE ${BODY_LINES})
        # Check if this line contains a sublayer call and add cprime:: prefix
        if(LINE MATCHES "layer${LAYER_NUM}_sublayers::sublayer${LAYER_NUM}[a-z]+")
            # Manually add cprime:: prefix to the line
            string(REPLACE "layer${LAYER_NUM}_sublayers::" "cprime::layer${LAYER_NUM}_sublayers::" PREFIXED_LINE "${LINE}")
            string(APPEND CURRENT_BLOCK "${PREFIXED_LINE}\n")
            
            # Close the current block and start a new one
            list(APPEND CODE_BLOCKS "${CURRENT_BLOCK}")
            set(CURRENT_BLOCK "")
            math(EXPR BLOCK_COUNT "${BLOCK_COUNT} + 1")
            
            message(STATUS "      Found sublayer call in line: ${LINE}")
            message(STATUS "      Added cprime:: prefix: ${PREFIXED_LINE}")
            message(STATUS "      Created block ${BLOCK_COUNT}")
        else()
            # Add line to current block (no modification needed for non-sublayer lines)
            string(APPEND CURRENT_BLOCK "${LINE}\n")
        endif()
    endforeach()
    
    # Add the final block if it has content
    if(CURRENT_BLOCK)
        list(APPEND CODE_BLOCKS "${CURRENT_BLOCK}")
        math(EXPR BLOCK_COUNT "${BLOCK_COUNT} + 1")
        message(STATUS "      Created final block ${BLOCK_COUNT}")
    endif()
    
    list(LENGTH CODE_BLOCKS FINAL_BLOCK_COUNT)
    message(STATUS "    Split into ${FINAL_BLOCK_COUNT} code blocks")
    
    set(${OUTPUT_VAR} "${CODE_BLOCKS}" PARENT_SCOPE)
endfunction()

# Extract complete function body with proper formatting
function(extract_complete_function_body FUNCTION_NAME SOURCE_FILE OUTPUT_VAR)
    if(NOT EXISTS ${SOURCE_FILE})
        message(WARNING "Source file does not exist: ${SOURCE_FILE}")
        set(${OUTPUT_VAR} "" PARENT_SCOPE)
        return()
    endif()
    
    file(READ ${SOURCE_FILE} FILE_CONTENT)
    
    # Find function definition - look for the specific function signature
    # Handle both namespace and non-namespace versions
    message(STATUS "    Looking for function: ${FUNCTION_NAME}")
    
    # Look for the function with a simpler pattern first
    string(FIND "${FILE_CONTENT}" "${FUNCTION_NAME}(" FUNCTION_POS)
    
    if(FUNCTION_POS GREATER -1)
        message(STATUS "    Found '${FUNCTION_NAME}(' at position ${FUNCTION_POS}")
        # Since we found the position, extract from the beginning of the function
        # Look backwards to find the start of the function declaration
        math(EXPR SEARCH_START "${FUNCTION_POS} - 50")
        if(SEARCH_START LESS 0)
            set(SEARCH_START 0)
        endif()
        
        # Extract a chunk around the function position
        string(SUBSTRING "${FILE_CONTENT}" ${SEARCH_START} 200 FUNCTION_CHUNK)
        message(STATUS "    Function chunk: ${FUNCTION_CHUNK}")
        
        # Look for the opening brace in this chunk
        string(FIND "${FUNCTION_CHUNK}" "{" BRACE_POS)
        if(BRACE_POS GREATER -1)
            message(STATUS "    Found opening brace at relative position ${BRACE_POS}")
            # Calculate absolute position of opening brace
            math(EXPR BRACE_ABS_POS "${SEARCH_START} + ${BRACE_POS}")
            # For now, just set a simple match to trigger the extraction
            set(START_MATCH "dummy_match")
            set(START_POS ${BRACE_ABS_POS})
        endif()
    else()
        message(STATUS "    Function name '${FUNCTION_NAME}(' not found in file")
    endif()
    
    if(START_MATCH)
        # We already have START_POS set from above
        if(NOT DEFINED START_POS)
            # Fallback: Find the position of the opening brace
            string(FIND "${FILE_CONTENT}" "${START_MATCH}" START_POS)
            string(LENGTH "${START_MATCH}" START_LENGTH)
        else()
            # We have the absolute position already
            set(START_LENGTH 1)  # Just the opening brace
        endif()
        
        # Find the position right after the opening brace
        math(EXPR BODY_START "${START_POS} + ${START_LENGTH} - 1")
        
        # Find matching closing brace using brace counting
        set(BRACE_COUNT 1)
        math(EXPR CURRENT_POS "${BODY_START} + 1")
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
            # Extract function body (excluding the outer braces)
            math(EXPR BODY_END "${CURRENT_POS} - 2")
            math(EXPR BODY_LENGTH "${BODY_END} - ${BODY_START}")
            string(SUBSTRING "${FILE_CONTENT}" ${BODY_START} ${BODY_LENGTH} EXTRACTED_BODY)
            
            # Clean up and format the body
            string(REGEX REPLACE "^\\s*\\{\\s*" "" EXTRACTED_BODY "${EXTRACTED_BODY}")
            string(REGEX REPLACE "\\s*\\}\\s*$" "" EXTRACTED_BODY "${EXTRACTED_BODY}")
            
            set(${OUTPUT_VAR} "${EXTRACTED_BODY}" PARENT_SCOPE)
            message(STATUS "    Successfully extracted function body (${BODY_LENGTH} chars)")
        else()
            message(WARNING "Could not find matching closing brace for function ${FUNCTION_NAME}")
            set(${OUTPUT_VAR} "" PARENT_SCOPE)
        endif()
    else()
        message(WARNING "Could not find function ${FUNCTION_NAME} in ${SOURCE_FILE}")
        set(${OUTPUT_VAR} "" PARENT_SCOPE)
    endif()
endfunction()

# Main function to generate three .inc files
function(generate_layer_test_files DISCOVERY_REGISTRY OUTPUT_DIR)
    message(STATUS "Generating layer test .inc files")
    
    # Generate includes file (.inc)
    set(INCLUDES_FILE "${OUTPUT_DIR}/layer_includes_generated.inc")
    generate_includes_file(${DISCOVERY_REGISTRY} ${INCLUDES_FILE})
    
    # Generate instrumented functions file (.inc)
    set(INSTRUMENTED_FUNCTIONS_FILE "${OUTPUT_DIR}/layer_instrumented_functions_generated.inc")
    generate_instrumented_functions_file(${DISCOVERY_REGISTRY} ${INSTRUMENTED_FUNCTIONS_FILE})
    
    # Generate dynamic TEST_F blocks file (.inc)
    set(DYNAMIC_TESTS_FILE "${OUTPUT_DIR}/layer_dynamic_tests_generated.inc")
    generate_dynamic_tests_file(${DISCOVERY_REGISTRY} ${DYNAMIC_TESTS_FILE})
    
    message(STATUS "Generated .inc files:")
    message(STATUS "  Includes: ${INCLUDES_FILE}")
    message(STATUS "  Instrumented functions: ${INSTRUMENTED_FUNCTIONS_FILE}")
    message(STATUS "  Dynamic tests: ${DYNAMIC_TESTS_FILE}")
endfunction()

# Generate dynamic TEST_F blocks file based on discovered layers
function(generate_dynamic_tests_file DISCOVERY_REGISTRY OUTPUT_FILE)
    message(STATUS "Generating dynamic TEST_F blocks file")
    
    file(READ ${DISCOVERY_REGISTRY} DISCOVERY_CONTENT)
    
    if(NOT DISCOVERY_CONTENT OR DISCOVERY_CONTENT MATCHES "^#.*No layer functions")
        file(WRITE ${OUTPUT_FILE} "// No layer functions discovered for dynamic test generation\n")
        return()
    endif()
    
    set(DYNAMIC_TESTS_CONTENT "")
    string(APPEND DYNAMIC_TESTS_CONTENT "// Auto-generated dynamic TEST_F blocks - DO NOT EDIT MANUALLY\n")
    string(APPEND DYNAMIC_TESTS_CONTENT "// Generated from: ${DISCOVERY_REGISTRY}\n\n")
    
    # Extract unique layer numbers from discovery
    string(REGEX MATCHALL "LAYER:([0-9]+)" LAYER_MATCHES "${DISCOVERY_CONTENT}")
    set(UNIQUE_LAYERS "")
    
    foreach(LAYER_MATCH ${LAYER_MATCHES})
        string(REGEX MATCH "LAYER:([0-9]+)" EXTRACTED_LAYER "${LAYER_MATCH}")
        if(EXTRACTED_LAYER)
            set(LAYER_NUM "${CMAKE_MATCH_1}")
            list(APPEND UNIQUE_LAYERS ${LAYER_NUM})
        endif()
    endforeach()
    
    # Remove duplicates
    if(UNIQUE_LAYERS)
        list(REMOVE_DUPLICATES UNIQUE_LAYERS)
        list(SORT UNIQUE_LAYERS)
    endif()
    
    # Generate TEST_F block for each layer
    foreach(LAYER_NUM ${UNIQUE_LAYERS})
        math(EXPR EXPECTED_LAYER "${LAYER_NUM} + 1")
        
        string(APPEND DYNAMIC_TESTS_CONTENT "TEST_F(LayerTestFixture, Layer${LAYER_NUM}_DynamicTests) {\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "    auto test_cases = discover_test_cases();\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "    \n")
        string(APPEND DYNAMIC_TESTS_CONTENT "    for (const auto& test_case : test_cases) {\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "        if (test_case.expected_outputs.count(${EXPECTED_LAYER})) {\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            SCOPED_TRACE(\"Test case: \" + test_case.name);\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            \n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            // Fresh input for this layer test\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            std::stringstream fresh_input(test_case.input_content);\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            StringTable string_table;\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            \n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            // Execute instrumented layer${LAYER_NUM} with deserialized input\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            auto actual_result = instrumented_layers::execute_layer${LAYER_NUM}_instrumented(cprime::layer${LAYER_NUM}_sublayers::validation::deserialize(fresh_input), string_table);\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            \n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            // Validate result\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            std::string actual_serialized = cprime::layer${LAYER_NUM}_sublayers::validation::serialize(actual_result);\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            std::string expected = test_case.expected_outputs.at(${EXPECTED_LAYER});\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            \n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            if (actual_serialized != expected) {\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "                log_test_failure(test_case.name, ${LAYER_NUM}, get_intermediate_states(), actual_serialized, expected);\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "                FAIL() << \"Layer${LAYER_NUM} output mismatch for \" << test_case.name;\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            }\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            \n")
        string(APPEND DYNAMIC_TESTS_CONTENT "            clear_intermediate_states();\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "        }\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "    }\n")
        string(APPEND DYNAMIC_TESTS_CONTENT "}\n\n")
    endforeach()
    
    file(WRITE ${OUTPUT_FILE} "${DYNAMIC_TESTS_CONTENT}")
    message(STATUS "Generated dynamic tests file: ${OUTPUT_FILE}")
endfunction()

# Function to display discovery summary
function(display_discovery_summary DISCOVERY_REGISTRY)
    if(NOT EXISTS ${DISCOVERY_REGISTRY})
        message(WARNING "Discovery registry not found: ${DISCOVERY_REGISTRY}")
        return()
    endif()
    
    file(READ ${DISCOVERY_REGISTRY} DISCOVERY_CONTENT)
    
    if(NOT DISCOVERY_CONTENT OR DISCOVERY_CONTENT MATCHES "^#.*No layer functions")
        message(STATUS "Discovery Summary: No layer functions found")
        return()
    endif()
    
    # Count functions
    string(REGEX MATCHALL "LAYER:[0-9]+" LAYER_MATCHES "${DISCOVERY_CONTENT}")
    list(LENGTH LAYER_MATCHES FUNCTION_COUNT)
    
    message(STATUS "=== DISCOVERY SUMMARY ===")
    message(STATUS "Total functions discovered: ${FUNCTION_COUNT}")
    
    # Show each function
    string(REPLACE "\n" ";" FUNCTION_ENTRIES "${DISCOVERY_CONTENT}")
    foreach(ENTRY ${FUNCTION_ENTRIES})
        if(ENTRY MATCHES "^LAYER:")
            string(REGEX MATCH "LAYER:([0-9]+)" LAYER_MATCH "${ENTRY}")
            string(REGEX MATCH "SUBLAYERS:([^;]*)" SUBLAYERS_MATCH "${ENTRY}")
            if(LAYER_MATCH)
                set(LAYER_NUM "${CMAKE_MATCH_1}")
                set(SUBLAYERS "${CMAKE_MATCH_1}")
                
                if(SUBLAYERS)
                    string(REPLACE "," ";" SUBLAYER_LIST "${SUBLAYERS}")
                    list(LENGTH SUBLAYER_LIST SUBLAYER_COUNT)
                    message(STATUS "  Layer ${LAYER_NUM}: ${SUBLAYER_COUNT} sublayers")
                else()
                    message(STATUS "  Layer ${LAYER_NUM}: 0 sublayers")
                endif()
            endif()
        endif()
    endforeach()
    message(STATUS "=========================")
endfunction()