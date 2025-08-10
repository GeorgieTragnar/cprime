#!/bin/bash

# CPrime Compiler Build Script
# Clean, minimal output with conditional test building
# Uses hybrid helper architecture: pipe-through for verbose/normal, capture for quiet

set -e  # Exit on any error

# Source common utilities
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
source "$SCRIPT_DIR/other/build_common.sh"

# Initialize paths
get_project_paths

# Default values
BUILD_TYPE="Debug"
CLEAN_BUILD=false
BUILD_TESTS=false
VERBOSE=false

# Usage function
show_help() {
    echo "CPrime Compiler Build Script"
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -c, --clean     Clean build directory before building"
    echo "  -t, --tests     Build Google Test suite"
    echo "  -r, --release   Build in Release mode (default: Debug)"
    echo "  -v, --verbose   Enable verbose build output"
    echo "  -h, --help      Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0              # Build compiler only"
    echo "  $0 -t           # Build compiler with tests"
    echo "  $0 -ct          # Clean build with tests"
    echo "  $0 -ctr         # Clean Release build with tests"
    echo "  $0 -cv          # Clean build with verbose output"
    echo "  $0 --clean --tests --release  # Long form options"
}

# Parse command line arguments using getopts
while getopts "ctvhrs-:" opt; do
    case $opt in
        c)
            CLEAN_BUILD=true
            ;;
        t)
            BUILD_TESTS=true
            ;;
        v)
            VERBOSE=true
            ;;
        r)
            BUILD_TYPE="Release"
            ;;
        h)
            show_help
            exit 0
            ;;
        s)
            # Handle short options that might be combined
            ;;
        -)
            # Handle long options
            case "${OPTARG}" in
                clean)
                    CLEAN_BUILD=true
                    ;;
                tests)
                    BUILD_TESTS=true
                    ;;
                release)
                    BUILD_TYPE="Release"
                    ;;
                verbose)
                    VERBOSE=true
                    ;;
                help)
                    show_help
                    exit 0
                    ;;
                *)
                    echo -e "${RED}Unknown long option: --${OPTARG}${NC}"
                    exit 1
                    ;;
            esac
            ;;
        \?)
            echo -e "${RED}Unknown option: -${OPTARG}${NC}"
            echo "Use -h for help"
            exit 1
            ;;
    esac
done

# Shift processed options
shift $((OPTIND-1))

# Verify prerequisites
if ! verify_source_directory; then
    exit 1
fi

if ! check_build_tools; then
    exit 1
fi

# Clean build if requested
if [ "$CLEAN_BUILD" = true ]; then
    clean_build_directory "$BUILD_DIR" false
fi

# Create build directory
create_build_directory "$BUILD_DIR"

# Show build information
echo -e "${BLUE}Configuring build...${NC}"

# Prepare helper script arguments
HELPER_ARGS="--build-type $BUILD_TYPE --cores $(get_core_count)"
if [ "$BUILD_TESTS" = true ]; then
    HELPER_ARGS="$HELPER_ARGS --build-tests"
fi

# Execute build using helper script
if [ "$VERBOSE" = true ]; then
    # VERBOSE MODE: Direct pipe-through with full details
    "$SCRIPT_DIR/other/build_helper.sh" $HELPER_ARGS --verbose
    HELPER_EXIT_CODE=$?
else
    # NORMAL MODE: Direct pipe-through with clean filtered output
    "$SCRIPT_DIR/other/build_helper.sh" $HELPER_ARGS --normal
    HELPER_EXIT_CODE=$?
fi

# Handle test build completion message
if [ "$BUILD_TESTS" = true ] && [ $HELPER_EXIT_CODE -eq 0 ]; then
    if [ -f "$BUILD_DIR/tests/cprime_tests" ]; then
        echo -e "${GREEN}✓ Test suite built successfully${NC}"
    else
        echo -e "${YELLOW}⚠ Test suite build failed${NC}"
    fi
fi

# Exit with the same code as the helper script
exit $HELPER_EXIT_CODE