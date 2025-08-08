#!/bin/bash

# CPrime Compiler - Safe Test Runner with Timeouts
# Runs all CPrime compiler unit tests with timeout protection to prevent system freezes

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

# Parse command line arguments
VERBOSE=false
STOP_ON_FAILURE=false
FILTER=""
TIMEOUT_SECONDS=10  # Default timeout for each test

while [[ $# -gt 0 ]]; do
    case $1 in
        --verbose)
            VERBOSE=true
            shift
            ;;
        --stop-on-failure)
            STOP_ON_FAILURE=true
            shift
            ;;
        --timeout)
            TIMEOUT_SECONDS="$2"
            shift 2
            ;;
        --filter)
            FILTER="$2"
            shift 2
            ;;
        -h|--help)
            echo "CPrime Safe Test Runner"
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --verbose           Show verbose output from tests"
            echo "  --stop-on-failure   Stop running tests after first failure"
            echo "  --timeout SECONDS   Set timeout for each test (default: 10s)"
            echo "  --filter PATTERN    Only run tests matching PATTERN"
            echo "  -h, --help          Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                          # Run all tests with 10s timeout"
            echo "  $0 --verbose                # Run all tests with verbose output"
            echo "  $0 --timeout 30             # Run tests with 30s timeout"
            echo "  $0 --filter validation      # Run only validation tests"
            echo "  $0 --stop-on-failure        # Stop at first test failure"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo -e "${BLUE}CPrime Compiler Test Suite (Safe Mode)${NC}"
echo "======================================="
echo "Build directory: $BUILD_DIR"
echo -e "${YELLOW}Timeout per test: ${TIMEOUT_SECONDS}s${NC}"
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory not found at $BUILD_DIR${NC}"
    echo "Please run './scripts/build.sh' first to build the compiler"
    exit 1
fi

# Change to build directory
cd "$BUILD_DIR"

# Define all test executables with metadata
declare -a TEST_SUITES=(
    "test_validation_layers:Validation System:Tests the 4-layer validation pipeline with RAII and defer functionality"
    "test_raw_tokenization:Raw Tokenization:Tests Layer 1 raw tokenization with error handling"
    "test_context_stack:Context Stack:Tests context tracking and resolution during parsing"
    "test_contextual_tokens:Contextual Tokens:Tests Layer 2 context enrichment with 1:1 token mapping"
    "test_semantic_translation:Semantic Translation:Tests semantic token generation and analysis"
    "test_feature_registry:Feature Registry:Tests implementation status tracking for language features"
    "test_ast_builder:AST Builder:Tests Layer 3 AST construction with symbol table population"
    "test_raii_injector:RAII Injector:Tests Layer 4 RAII injection and cleanup code generation"
    "test_pipeline_integration:Pipeline Integration:Tests full multi-layer compilation pipeline"
)

# Initialize counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_TIMEOUT=0
FAILED_TESTS=()
TIMEOUT_TESTS=()

echo -e "${BLUE}Running Test Suites:${NC}"
echo ""

# Run each test suite
for test_entry in "${TEST_SUITES[@]}"; do
    # Split on ':' to get executable, name, and description
    IFS=':' read -r test_exe test_name test_desc <<< "$test_entry"
    
    # Apply filter if specified
    if [ -n "$FILTER" ] && [[ ! "$test_name" =~ $FILTER ]] && [[ ! "$test_exe" =~ $FILTER ]]; then
        continue
    fi
    
    # Check if executable exists
    if [ ! -f "$test_exe" ]; then
        echo -e "${YELLOW}⚠ Skipping $test_name - executable not found${NC}"
        continue
    fi
    
    echo -e "${BLUE}Running $test_name...${NC}"
    if [ "$VERBOSE" = true ]; then
        echo -e "${YELLOW}  Description: $test_desc${NC}"
        echo -e "${YELLOW}  Executable: $test_exe${NC}"
        echo ""
    fi
    
    # Run the test with timeout
    TESTS_RUN=$((TESTS_RUN + 1))
    
    # Create temp file for output
    TEMP_OUTPUT="/tmp/cprime_test_$$.txt"
    
    # Run test with timeout
    if [ "$VERBOSE" = true ]; then
        # Run with full output and timeout
        timeout --preserve-status ${TIMEOUT_SECONDS}s ./"$test_exe" 2>&1 | tee "$TEMP_OUTPUT"
        EXIT_CODE=${PIPESTATUS[0]}
    else
        # Run with captured output and timeout
        timeout --preserve-status ${TIMEOUT_SECONDS}s ./"$test_exe" > "$TEMP_OUTPUT" 2>&1
        EXIT_CODE=$?
    fi
    
    # Check exit code
    if [ $EXIT_CODE -eq 124 ]; then
        # Timeout occurred
        TESTS_TIMEOUT=$((TESTS_TIMEOUT + 1))
        TIMEOUT_TESTS+=("$test_name")
        echo -e "${RED}  ✗ TIMEOUT (exceeded ${TIMEOUT_SECONDS}s)${NC}"
        echo -e "${YELLOW}    Last output before timeout:${NC}"
        tail -5 "$TEMP_OUTPUT" | sed 's/^/      /'
        
        if [ "$STOP_ON_FAILURE" = true ]; then
            echo -e "${RED}Stopping due to timeout${NC}"
            rm -f "$TEMP_OUTPUT"
            break
        fi
    elif [ $EXIT_CODE -eq 0 ]; then
        # Test passed
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo -e "${GREEN}  ✓ All tests passed${NC}"
    else
        # Test failed
        TESTS_FAILED=$((TESTS_FAILED + 1))
        FAILED_TESTS+=("$test_name")
        echo -e "${RED}  ✗ Some tests failed (exit code: $EXIT_CODE)${NC}"
        
        if [ "$VERBOSE" = false ]; then
            echo -e "${YELLOW}    Error output:${NC}"
            tail -10 "$TEMP_OUTPUT" | sed 's/^/      /'
            echo -e "${YELLOW}    Run with --verbose to see full output${NC}"
        fi
        
        if [ "$STOP_ON_FAILURE" = true ]; then
            echo -e "${RED}Stopping on first failure as requested${NC}"
            rm -f "$TEMP_OUTPUT"
            break
        fi
    fi
    
    # Clean up temp file
    rm -f "$TEMP_OUTPUT"
    
    echo ""
done

# Print summary
echo -e "${BLUE}=== Test Suite Summary ===${NC}"
echo "Total test suites run: $TESTS_RUN"
echo -e "${GREEN}Passed: $TESTS_PASSED${NC}"
echo -e "${RED}Failed: $TESTS_FAILED${NC}"
echo -e "${RED}Timeout: $TESTS_TIMEOUT${NC}"

# Show timeout warning if any tests timed out
if [ $TESTS_TIMEOUT -gt 0 ]; then
    echo ""
    echo -e "${RED}⚠ WARNING: The following tests timed out:${NC}"
    for timeout_test in "${TIMEOUT_TESTS[@]}"; do
        echo -e "${RED}  - $timeout_test${NC}"
    done
    echo ""
    echo -e "${YELLOW}Timeout issues may indicate:${NC}"
    echo "  • Infinite loops in test or production code"
    echo "  • Excessive memory allocation causing thrashing"
    echo "  • Deadlocks or synchronization issues"
    echo ""
    echo -e "${YELLOW}To debug, try:${NC}"
    echo "  • Running the test individually with gdb"
    echo "  • Using valgrind to check for memory issues"
    echo "  • Increasing timeout with --timeout option"
fi

if [ $TESTS_FAILED -gt 0 ]; then
    echo ""
    echo -e "${RED}✗ The following test suites failed:${NC}"
    for failed_test in "${FAILED_TESTS[@]}"; do
        echo -e "${RED}  - $failed_test${NC}"
    done
fi

if [ $TESTS_FAILED -eq 0 ] && [ $TESTS_TIMEOUT -eq 0 ] && [ $TESTS_RUN -gt 0 ]; then
    echo ""
    echo -e "${GREEN}✓ All test suites passed!${NC}"
    echo -e "${GREEN}CPrime compiler test suite completed successfully!${NC}"
    echo "All validation layers, tokenization, parsing, and integration tests are working."
    exit 0
elif [ $TESTS_RUN -eq 0 ]; then
    if [ -n "$FILTER" ]; then
        echo -e "${YELLOW}⚠ No tests matched filter '$FILTER'${NC}"
    else
        echo -e "${YELLOW}⚠ No test executables found${NC}"
        echo "Please run './scripts/build.sh' to build the test suite"
    fi
    exit 1
else
    echo ""
    if [ $TESTS_TIMEOUT -gt 0 ]; then
        echo -e "${RED}✗ Test suite failed with timeouts${NC}"
    else
        echo -e "${RED}✗ Test suite failed${NC}"
    fi
    echo -e "${YELLOW}Run with --verbose to see detailed output${NC}"
    exit 1
fi