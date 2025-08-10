#!/bin/bash

# CPrime Build Helper - Pure Build Execution
# Handles verbose and normal modes directly via pipe-through
# Quiet mode is handled by the main script

set -e

# Source common utilities
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
source "$SCRIPT_DIR/build_common.sh"

# Initialize paths
get_project_paths

# Default values
BUILD_TYPE="Debug"
BUILD_TESTS=false
CORES=$(get_core_count)
OUTPUT_MODE="normal"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --build-tests)
            BUILD_TESTS=true
            shift
            ;;
        --cores)
            CORES="$2"
            shift 2
            ;;
        --verbose)
            OUTPUT_MODE="verbose"
            shift
            ;;
        --normal)
            OUTPUT_MODE="normal"
            shift
            ;;
        --source-dir)
            SOURCE_DIR="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

# Execute CMake configuration
execute_cmake() {
    local cmake_args="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    
    # Set test building based on flag
    if [ "$BUILD_TESTS" = true ]; then
        cmake_args="$cmake_args -DBUILD_TESTS=ON"
    else
        cmake_args="$cmake_args -DBUILD_TESTS=OFF"
    fi
    
    # Add verbose makefile for verbose mode
    if [ "$OUTPUT_MODE" = "verbose" ]; then
        cmake_args="$cmake_args -DCMAKE_VERBOSE_MAKEFILE=ON"
    fi
    
    # Change to build directory
    cd "$BUILD_DIR"
    
    # Execute CMake with appropriate output handling
    if [ "$OUTPUT_MODE" = "verbose" ]; then
        # Verbose: show everything
        cmake "$SOURCE_DIR" $cmake_args
    else
        # Normal: show filtered cmake output
        cmake "$SOURCE_DIR" $cmake_args 2>&1 | grep -E "(--|ERROR|WARNING|FATAL_ERROR|found|Configuring done|Generating done)" || true
    fi
}

# Execute Make build
execute_make() {
    # Change to build directory (should already be there from cmake)
    cd "$BUILD_DIR"
    
    # Execute make with appropriate output handling
    if [ "$OUTPUT_MODE" = "verbose" ]; then
        # Verbose: show everything including makefile details
        make -j$CORES VERBOSE=1
    else
        # Normal: show filtered make output
        echo -e "${BLUE}Building with $CORES parallel jobs...${NC}"
        make -j$CORES --no-print-directory 2>&1 | grep -E "(\[[[:space:]]*[0-9]+%\]|Building|Linking|warning:|error:|Error|Built target)" || true
    fi
}

# Show completion message for normal mode
show_completion() {
    if [ "$OUTPUT_MODE" = "normal" ]; then
        echo -e "${GREEN}✓ CPrime compiler built successfully${NC}"
        echo -e "${GREEN}  - Main executable: bin/cprime${NC}"
        echo -e "${GREEN}  - Core libraries: commons, layer0, orchestrator${NC}"
    fi
}

# Main execution
main() {
    # Execute build phases
    execute_cmake
    execute_make
    
    # Verify build completed successfully
    if ! verify_build_artifacts "$BUILD_DIR"; then
        echo -e "${RED}Error: Build failed - missing required artifacts${NC}" >&2
        exit 1
    fi
    
    # Show completion message for normal mode
    show_completion
}

# Run main function
main "$@"