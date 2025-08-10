#!/bin/bash

# CPrime Compilation Script
# Easy compilation of example CPrime files using the built compiler

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Get the project root directory (parent of scripts/)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CPRIME_BINARY="$PROJECT_ROOT/build/bin/cprime"
EXAMPLES_DIR="$PROJECT_ROOT/examples"

# Default values
EXAMPLE_NAME=""
DEBUG_MODE=false
QUIET_MODE=false
EXTRA_FLAGS=""

# Usage function
show_help() {
    echo -e "${BOLD}CPrime Compilation Script${NC}"
    echo "=========================="
    echo ""
    echo "Usage: $0 [OPTIONS] <example_name>"
    echo ""
    echo -e "${BOLD}Arguments:${NC}"
    echo "  example_name    Name of the example file to compile (without extension)"
    echo "                  Will look for examples/{name}.cprime first, then examples/{name}.cp"
    echo ""
    echo -e "${BOLD}Options:${NC}"
    echo "  --debug         Enable debug mode in cprime compiler"
    echo "  --quiet         Suppress verbose headers and file info"
    echo "  -h, --help      Show this help message"
    echo "  --              Pass remaining arguments directly to cprime compiler"
    echo ""
    echo -e "${BOLD}Examples:${NC}"
    echo "  $0 hello                    # Compile examples/hello.cprime"
    echo "  $0 simple                   # Compile examples/simple.cp"
    echo "  $0 class_test               # Compile examples/class_test.cprime"
    echo "  $0 --debug hello            # Compile with debug output"
    echo "  $0 --quiet simple           # Compile with minimal output"
    echo "  $0 hello -- --dump-ast      # Pass --dump-ast flag to compiler"
    echo ""
    echo -e "${BOLD}Available Examples:${NC}"
    if [ -d "$EXAMPLES_DIR" ]; then
        for file in "$EXAMPLES_DIR"/*.cprime "$EXAMPLES_DIR"/*.cp; do
            if [ -f "$file" ]; then
                basename "$file" | sed 's/\.[^.]*$//'
            fi
        done | sort | uniq | sed 's/^/  /'
    else
        echo "  (examples directory not found)"
    fi
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            DEBUG_MODE=true
            shift
            ;;
        --quiet)
            QUIET_MODE=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        --)
            shift
            EXTRA_FLAGS="$*"
            break
            ;;
        -*)
            echo -e "${RED}Error: Unknown option $1${NC}"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
        *)
            if [ -z "$EXAMPLE_NAME" ]; then
                EXAMPLE_NAME="$1"
            else
                echo -e "${RED}Error: Multiple example names provided${NC}"
                echo "Use -h or --help for usage information"
                exit 1
            fi
            shift
            ;;
    esac
done

# Validate required argument
if [ -z "$EXAMPLE_NAME" ]; then
    echo -e "${RED}Error: No example name provided${NC}"
    echo ""
    show_help
    exit 1
fi

# Check if cprime binary exists
if [ ! -f "$CPRIME_BINARY" ]; then
    echo -e "${RED}Error: CPrime compiler not found at $CPRIME_BINARY${NC}"
    echo "Please run: ./scripts/build.sh to build the compiler first"
    exit 1
fi

if [ ! -x "$CPRIME_BINARY" ]; then
    echo -e "${RED}Error: CPrime compiler is not executable${NC}"
    exit 1
fi

# Find the example file (try .cprime first, then .cp)
EXAMPLE_FILE=""
if [ -f "$EXAMPLES_DIR/${EXAMPLE_NAME}.cprime" ]; then
    EXAMPLE_FILE="$EXAMPLES_DIR/${EXAMPLE_NAME}.cprime"
elif [ -f "$EXAMPLES_DIR/${EXAMPLE_NAME}.cp" ]; then
    EXAMPLE_FILE="$EXAMPLES_DIR/${EXAMPLE_NAME}.cp"
else
    echo -e "${RED}Error: Example file not found${NC}"
    echo "Looked for:"
    echo "  - $EXAMPLES_DIR/${EXAMPLE_NAME}.cprime"
    echo "  - $EXAMPLES_DIR/${EXAMPLE_NAME}.cp"
    echo ""
    echo "Available examples:"
    if [ -d "$EXAMPLES_DIR" ]; then
        for file in "$EXAMPLES_DIR"/*.cprime "$EXAMPLES_DIR"/*.cp; do
            if [ -f "$file" ]; then
                basename "$file" | sed 's/\.[^.]*$//'
            fi
        done | sort | uniq | sed 's/^/  /'
    fi
    exit 1
fi

# Build compiler flags
COMPILER_FLAGS="--verbose"
if [ "$DEBUG_MODE" = true ]; then
    COMPILER_FLAGS="$COMPILER_FLAGS --debug"
fi
if [ -n "$EXTRA_FLAGS" ]; then
    COMPILER_FLAGS="$COMPILER_FLAGS $EXTRA_FLAGS"
fi

# Display header information (unless quiet mode)
if [ "$QUIET_MODE" = false ]; then
    echo -e "${BOLD}${BLUE}=== Compiling $(basename "$EXAMPLE_FILE") ===${NC}"
    
    # Show file information
    FILE_SIZE=$(stat -f%z "$EXAMPLE_FILE" 2>/dev/null || stat -c%s "$EXAMPLE_FILE" 2>/dev/null || echo "unknown")
    CHAR_COUNT=$(wc -c < "$EXAMPLE_FILE" 2>/dev/null || echo "unknown")
    LINE_COUNT=$(wc -l < "$EXAMPLE_FILE" 2>/dev/null || echo "unknown")
    
    echo -e "${CYAN}File:${NC} $EXAMPLE_FILE"
    echo -e "${CYAN}Size:${NC} $FILE_SIZE bytes, $CHAR_COUNT characters, $LINE_COUNT lines"
    echo -e "${CYAN}Flags:${NC} $COMPILER_FLAGS"
    echo ""
fi

# Execute compilation and capture output
if [ "$QUIET_MODE" = false ]; then
    echo -e "${BOLD}${YELLOW}=== CPrime Compiler Output ===${NC}"
fi

# Run the compiler and capture exit status
set +e  # Temporarily disable exit on error
COMPILER_OUTPUT_FILE=$(mktemp)
"$CPRIME_BINARY" $COMPILER_FLAGS "$EXAMPLE_FILE" > "$COMPILER_OUTPUT_FILE" 2>&1
COMPILER_EXIT_CODE=$?
set -e  # Re-enable exit on error

# Display the output
cat "$COMPILER_OUTPUT_FILE"

# Clean up temporary file
rm -f "$COMPILER_OUTPUT_FILE"

# Display result
if [ "$QUIET_MODE" = false ]; then
    echo ""
    if [ $COMPILER_EXIT_CODE -eq 0 ]; then
        echo -e "${BOLD}${GREEN}=== Compilation Result: SUCCESS ===${NC}"
    else
        echo -e "${BOLD}${RED}=== Compilation Result: FAILED (exit code: $COMPILER_EXIT_CODE) ===${NC}"
    fi
fi

# Exit with the same code as the compiler
exit $COMPILER_EXIT_CODE