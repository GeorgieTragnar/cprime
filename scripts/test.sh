#!/bin/bash

# CPrime Compiler Test Script

set -e  # Exit on any error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
EXAMPLES_DIR="$PROJECT_ROOT/examples"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Options
COMPILE_ONLY=false
KEEP_EXECUTABLES=false
SPECIFIC_TEST=""
VERBOSE=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --compile-only)
            COMPILE_ONLY=true
            shift
            ;;
        --keep-executables)
            KEEP_EXECUTABLES=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            echo "CPrime Test Script"
            echo "Usage: $0 [OPTIONS] [TEST_FILE]"
            echo ""
            echo "Options:"
            echo "  --compile-only      Only compile tests, don't execute them"
            echo "  --keep-executables  Don't delete test executables after running"
            echo "  --verbose           Show detailed output"
            echo "  -h, --help          Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                    # Run all tests"
            echo "  $0 hello.cprime       # Run specific test"
            echo "  $0 --compile-only     # Only compile all tests"
            exit 0
            ;;
        *.cprime)
            SPECIFIC_TEST="$1"
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo -e "${BLUE}=== CPrime Compiler Test Script ===${NC}"
echo "Project root: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"

# Check if compiler exists
if [ ! -f "$BUILD_DIR/bin/cprime" ]; then
    echo -e "${RED}Error: CPrime compiler not found at $BUILD_DIR/bin/cprime${NC}"
    echo "Please run './scripts/build.sh' first to build the compiler"
    exit 1
fi

echo -e "${GREEN}✓ CPrime compiler found${NC}"

# Change to build directory
cd "$BUILD_DIR"

# Initialize counters
test_passed=0
test_total=0

# Function to run a single test
run_test() {
    local cprime_file="$1"
    local filename=$(basename "$cprime_file" .cprime)
    local output_name="${filename}_test"
    
    echo -e "${YELLOW}Testing $cprime_file...${NC}"
    
    # Compile the file
    if [ "$VERBOSE" = true ]; then
        echo "  Command: ./bin/cprime \"$cprime_file\" -o \"$output_name\""
    fi
    
    if ./bin/cprime "$cprime_file" -o "$output_name" 2>/dev/null; then
        if [ -f "$output_name" ]; then
            echo -e "${GREEN}  ✓ $filename compilation successful${NC}"
            
            if [ "$COMPILE_ONLY" = false ]; then
                echo "  Running $output_name:"
                echo "  ----------------------------------------"
                if ./"$output_name"; then
                    echo "  ----------------------------------------"
                    echo -e "${GREEN}  ✓ $filename execution successful${NC}"
                    test_passed=$((test_passed + 1))
                else
                    echo "  ----------------------------------------"
                    echo -e "${RED}  ✗ $filename execution failed${NC}"
                fi
            else
                test_passed=$((test_passed + 1))
            fi
            
            # Clean up the executable unless requested to keep
            if [ "$KEEP_EXECUTABLES" = false ]; then
                rm -f "$output_name"
            else
                echo -e "${BLUE}  → Keeping executable: $output_name${NC}"
            fi
        else
            echo -e "${RED}  ✗ $filename compilation failed - no output file${NC}"
        fi
    else
        echo -e "${RED}  ✗ $filename compilation failed${NC}"
    fi
    
    test_total=$((test_total + 1))
    echo
}

# Run tests
if [ -n "$SPECIFIC_TEST" ]; then
    # Run specific test
    if [ -f "$EXAMPLES_DIR/$SPECIFIC_TEST" ]; then
        echo -e "${YELLOW}Running specific test: $SPECIFIC_TEST${NC}"
        echo
        run_test "$EXAMPLES_DIR/$SPECIFIC_TEST"
    else
        echo -e "${RED}Error: Test file not found: $EXAMPLES_DIR/$SPECIFIC_TEST${NC}"
        exit 1
    fi
else
    # Run all tests
    echo -e "${YELLOW}Running all .cprime files in examples...${NC}"
    echo
    
    for cprime_file in examples/*.cprime; do
        if [ -f "$cprime_file" ]; then
            run_test "$cprime_file"
        fi
    done
fi

# Summary
echo -e "${BLUE}=== Test Results ===${NC}"
if [ $test_total -eq 0 ]; then
    echo -e "${YELLOW}No .cprime test files found${NC}"
else
    echo "Total tests: $test_total"
    echo "Passed: $test_passed"
    echo "Failed: $((test_total - test_passed))"
    
    if [ $test_passed -eq $test_total ]; then
        echo -e "${GREEN}✓ All tests passed!${NC}"
        exit 0
    else
        echo -e "${RED}✗ Some tests failed${NC}"
        exit 1
    fi
fi