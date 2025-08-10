#!/bin/bash

# CPrime Compiler Test Runner
# Enhanced layer-based Google Test execution with flexible selection

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get the project root directory (parent of scripts/)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

# Default options
SUPPRESS_VALIDATION=false
GTEST_FILTERS=""
CORE_LAYERS=()
VALIDATION_LAYERS=()
INTEGER_LAYERS=""

# Show comprehensive help
show_help() {
    echo "CPrime Compiler Test Runner"
    echo "Usage: $0 [OPTIONS] [LAYER_SELECTION]"
    echo ""
    echo "Integer-based Layer Selection:"
    echo "  [INTEGERS]      Run specified layers with validation (e.g., 1,3,5 or 1-4)"
    echo "  -l INTEGERS     Run specified layers WITHOUT validation (e.g., -l 1,3,5)"
    echo ""
    echo "Explicit Layer Selection:"  
    echo "  --l1, --l2, --l3, --l4     Run specific core layer tests"
    echo "  --l1v, --l2v, --l3v, --l4v Run specific validation layer tests"
    echo ""
    echo "Legacy Support (backward compatible):"
    echo "  -l1, -l2, -l3, -l4         Run core layer (old syntax)"
    echo "  -l1v, -l2v, -l3v, -l4v     Run validation layer (old syntax)"
    echo ""
    echo "Options:"
    echo "  -l              Suppress validation tests (use with integers)"
    echo "  -h, --help      Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0              # All tests (core + validation)"
    echo "  $0 1,3,5        # Layers 1,3,5 with validation"
    echo "  $0 -l 1,3,5     # Layers 1,3,5 without validation"  
    echo "  $0 1-4          # Layers 1,2,3,4 with validation"
    echo "  $0 -l 1-4 --l2v # Layers 1,2,3,4 (no validation) + Layer 2 validation"
    echo "  $0 --l1 --l3v   # Layer 1 core + Layer 3 validation"
    echo "  $0 -l1 -l3v     # Legacy: Layer 1 + Layer 3 validation"
}

# Parse integer range/list (e.g., "1,3,5" or "1-4" or "1,3-5,8")
parse_integer_layers() {
    local input="$1"
    local result=()
    
    # Split by commas
    IFS=',' read -ra PARTS <<< "$input"
    
    for part in "${PARTS[@]}"; do
        if [[ "$part" =~ ^[0-9]+$ ]]; then
            # Single number
            result+=("$part")
        elif [[ "$part" =~ ^([0-9]+)-([0-9]+)$ ]]; then
            # Range (e.g., 1-4)
            local start=${BASH_REMATCH[1]}
            local end=${BASH_REMATCH[2]}
            for ((i=start; i<=end; i++)); do
                result+=("$i")
            done
        else
            echo -e "${RED}Error: Invalid layer specification: $part${NC}" >&2
            echo "Use integers (1,2,3) or ranges (1-4)" >&2
            return 1
        fi
    done
    
    # Remove duplicates and sort
    printf '%s\n' "${result[@]}" | sort -nu
}

# Add layer to appropriate array
add_layer() {
    local layer_type="$1"  # "core" or "validation"
    local layer_num="$2"   # 1, 2, 3, 4
    
    if [[ ! "$layer_num" =~ ^[1-4]$ ]]; then
        echo -e "${RED}Error: Invalid layer number: $layer_num${NC}" >&2
        echo "Supported layers: 1, 2, 3, 4" >&2
        return 1
    fi
    
    if [[ "$layer_type" == "core" ]]; then
        if [[ ! " ${CORE_LAYERS[*]} " =~ " ${layer_num} " ]]; then
            CORE_LAYERS+=("$layer_num")
        fi
    elif [[ "$layer_type" == "validation" ]]; then
        if [[ ! " ${VALIDATION_LAYERS[*]} " =~ " ${layer_num} " ]]; then
            VALIDATION_LAYERS+=("$layer_num")
        fi
    fi
}

# Parse command line arguments
parse_arguments() {
    local i=1
    while [[ $i -le $# ]]; do
        local arg="${!i}"
        case "$arg" in
            -h|--help)
                show_help
                exit 0
                ;;
            -l)
                # Check if next argument is integers
                local next_i=$((i + 1))
                if [[ $next_i -le $# ]]; then
                    local next_arg="${!next_i}"
                    if [[ "$next_arg" =~ ^[0-9,-]+$ ]]; then
                        # -l followed by integers: suppress validation
                        SUPPRESS_VALIDATION=true
                        INTEGER_LAYERS="$next_arg"
                        i=$((i + 2))
                    else
                        # -l alone: just set flag
                        SUPPRESS_VALIDATION=true
                        i=$((i + 1))
                    fi
                else
                    # -l at end: just set flag
                    SUPPRESS_VALIDATION=true
                    i=$((i + 1))
                fi
                ;;
            --l1|--l2|--l3|--l4)
                # New explicit core layer syntax
                local layer_num="${arg:3}"
                add_layer "core" "$layer_num" || exit 1
                i=$((i + 1))
                ;;
            --l1v|--l2v|--l3v|--l4v)
                # New explicit validation layer syntax
                local layer_num="${arg:3:1}"
                add_layer "validation" "$layer_num" || exit 1
                i=$((i + 1))
                ;;
            -l1|-l2|-l3|-l4)
                # Legacy core layer syntax
                local layer_num="${arg:2}"
                add_layer "core" "$layer_num" || exit 1
                i=$((i + 1))
                ;;
            -l1v|-l2v|-l3v|-l4v)
                # Legacy validation layer syntax
                local layer_num="${arg:2:1}"
                add_layer "validation" "$layer_num" || exit 1
                i=$((i + 1))
                ;;
            [0-9,-]*)
                # Integer layer specification (without -l)
                if [[ -z "$INTEGER_LAYERS" ]]; then
                    INTEGER_LAYERS="$arg"
                else
                    echo -e "${RED}Error: Multiple integer layer specifications${NC}" >&2
                    exit 1
                fi
                i=$((i + 1))
                ;;
            *)
                echo -e "${RED}Unknown option: $arg${NC}" >&2
                echo "Use -h for help" >&2
                exit 1
                ;;
        esac
    done
}

# Process integer layers into core/validation arrays
process_integer_layers() {
    if [[ -n "$INTEGER_LAYERS" ]]; then
        local layers
        if ! layers=$(parse_integer_layers "$INTEGER_LAYERS"); then
            exit 1
        fi
        
        while IFS= read -r layer; do
            [[ -n "$layer" ]] || continue
            add_layer "core" "$layer" || exit 1
            
            # Add validation unless suppressed
            if [[ "$SUPPRESS_VALIDATION" != true ]]; then
                add_layer "validation" "$layer" || exit 1
            fi
        done <<< "$layers"
    fi
}

# Build Google Test filters based on selected layers
build_filters() {
    local filters=()
    
    # Add core layer filters
    for layer in "${CORE_LAYERS[@]}"; do
        case $layer in
            1)
                filters+=("RawTokenizationTest.*:ProgramEntryPointTest.*")
                ;;
            2)
                filters+=("*Layer2*:*Context*:*Semantic*")
                ;;
            3)
                filters+=("*Layer3*:*AST*:*Symbol*")
                ;;
            4)
                filters+=("*Layer4*:*RAII*")
                ;;
        esac
    done
    
    # Add validation layer filters
    for layer in "${VALIDATION_LAYERS[@]}"; do
        case $layer in
            1)
                filters+=("*Layer1*Validation*")
                ;;
            2)
                filters+=("*Layer2*Validation*")
                ;;
            3)
                filters+=("*Layer3*Validation*")
                ;;
            4)
                filters+=("*Layer4*Validation*")
                ;;
        esac
    done
    
    # Join filters with ':'
    if [ ${#filters[@]} -gt 0 ]; then
        IFS=':' eval 'GTEST_FILTERS="${filters[*]}"'
    fi
}

# Main execution
main() {
    # Parse all arguments
    parse_arguments "$@"
    
    # Process integer layers
    process_integer_layers
    
    # Check if build directory exists
    if [ ! -d "$BUILD_DIR" ]; then
        echo -e "${RED}Error: Build directory not found at $BUILD_DIR${NC}"
        echo "Please run './scripts/build.sh -t' to build the test suite"
        exit 1
    fi
    
    # Check if test executable exists
    if [ ! -f "$BUILD_DIR/tests/cprime_tests" ]; then
        echo -e "${RED}Error: Test executable not found${NC}"
        echo "Please run './scripts/build.sh -t' to build the test suite"
        exit 1
    fi
    
    # Build the filter string
    build_filters
    
    # Change to build directory
    cd "$BUILD_DIR"
    
    # Prepare Google Test arguments
    GTEST_ARGS="--gtest_color=yes"
    
    if [ -n "$GTEST_FILTERS" ]; then
        GTEST_ARGS="$GTEST_ARGS --gtest_filter=$GTEST_FILTERS"
    fi
    
    # Show what we're running
    local layer_description=""
    if [ ${#CORE_LAYERS[@]} -gt 0 ] || [ ${#VALIDATION_LAYERS[@]} -gt 0 ]; then
        local parts=()
        [ ${#CORE_LAYERS[@]} -gt 0 ] && parts+=("Core: ${CORE_LAYERS[*]}")
        [ ${#VALIDATION_LAYERS[@]} -gt 0 ] && parts+=("Validation: ${VALIDATION_LAYERS[*]}")
        layer_description=$(IFS=', '; echo "${parts[*]}")
        echo -e "${BLUE}Running tests for layers: $layer_description${NC}"
    else
        echo -e "${BLUE}Running all tests${NC}"
    fi
    
    echo ""
    
    # Run the tests with the same output filtering as before
    ./tests/cprime_tests $GTEST_ARGS 2>&1 | tee /tmp/test_output.log | while IFS= read -r line; do
        case "$line" in
            *"[==========]"*|*"[----------]"*|*"[  PASSED  ]"*|*"[  FAILED  ]"*)
                echo "$line"
                ;;
            *"PASSED"*|*"FAILED"*)
                echo "$line"
                ;;
            *"SUCCESS:"*|*"FAILED:"*)
                echo "$line"
                ;;
        esac
    done
    
    EXIT_CODE=${PIPESTATUS[0]}
    
    echo ""
    if [ $EXIT_CODE -eq 0 ]; then
        echo -e "${GREEN}✓ All selected tests passed${NC}"
    else
        echo -e "${RED}✗ Some tests failed${NC}"
    fi
    
    # Clean up
    rm -f /tmp/test_output.log
    
    exit $EXIT_CODE
}

# Call main with all arguments
main "$@"