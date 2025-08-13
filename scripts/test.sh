#!/bin/bash

# CPrime Compiler Test Runner
# Simplified robust test execution for the comprehensive integration testing framework

set -e  # Exit on any error

# Colors for output (consistent with other scripts)
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Get the project root directory (parent of scripts/)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_EXECUTABLE="$BUILD_DIR/tests/cprime_tests"

# Default options
RUN_LAYER1=false
RUN_UNIT=false
RUN_INTEGRATION=false
RUN_ALL=true
VERBOSE=false
SHOW_DISCOVERY=false

# Show help message
show_help() {
    echo -e "${BLUE}CPrime Compiler Test Runner${NC}"
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Test Selection:"
    echo "  --layer1, --l1, -l  Run Layer 1 integration tests only"
    echo "  --unit, -u          Run unit tests only"  
    echo "  --integration, -i   Run integration tests only"
    echo "  --all, -a           Run all available tests (default)"
    echo ""
    echo "Options:"
    echo "  --verbose, -v       Show verbose test output"
    echo "  --discovery, -d     Show test case discovery information"
    echo "  --help, -h          Show this help message"
    echo ""
    echo "Short Options (can be combined):"
    echo "  -d                  Discovery mode"
    echo "  -u                  Unit tests"
    echo "  -i                  Integration tests"  
    echo "  -l                  Layer 1 tests"
    echo "  -a                  All tests"
    echo "  -v                  Verbose output"
    echo "  -duia               Discovery + unit + integration + all + verbose (example)"
    echo ""
    echo "Examples:"
    echo "  $0                  # Run all tests"
    echo "  $0 -l               # Run only Layer 1 integration tests"
    echo "  $0 --integration    # Run all integration tests"
    echo "  $0 -iv              # Run integration tests with verbose output"
    echo "  $0 -d --unit        # Show discovery then run unit tests"
    echo "  $0 -duv             # Discovery + unit tests + verbose"
    echo ""
    echo "Test Structure:"
    echo "  - Integration tests: Auto-discovered from test_cases/"
    echo "  - Layer 1 tests: Comprehensive tokenization validation"
    echo "  - Future layers: Will be added as they are implemented"
}

# Parse command line arguments using getopt
parse_arguments() {
    # Use getopt to parse short and long options
    local options
    if ! options=$(getopt -o hluiavd -l help,layer1,l1,unit,integration,all,verbose,discovery -- "$@"); then
        echo "Use --help for usage information" >&2
        exit 1
    fi
    
    # Set the positional parameters
    eval set -- "$options"
    
    while true; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -l|--layer1|--l1)
                RUN_LAYER1=true
                RUN_ALL=false
                shift
                ;;
            -u|--unit)
                RUN_UNIT=true
                RUN_ALL=false
                shift
                ;;
            -i|--integration)
                RUN_INTEGRATION=true
                RUN_ALL=false
                shift
                ;;
            -a|--all)
                RUN_ALL=true
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -d|--discovery)
                SHOW_DISCOVERY=true
                shift
                ;;
            --)
                shift
                break
                ;;
            *)
                echo -e "${RED}Internal error parsing options${NC}" >&2
                exit 1
                ;;
        esac
    done
}

# Check if build system is ready
check_build_system() {
    if [ ! -d "$BUILD_DIR" ]; then
        echo -e "${RED}Error: Build directory not found at $BUILD_DIR${NC}" >&2
        echo "Please run './scripts/build.sh -t' to build the test suite" >&2
        exit 1
    fi
    
    if [ ! -f "$TEST_EXECUTABLE" ]; then
        echo -e "${RED}Error: Test executable not found at $TEST_EXECUTABLE${NC}" >&2
        echo "Please run './scripts/build.sh -t' to build the test suite" >&2
        exit 1
    fi
    
    if [ ! -x "$TEST_EXECUTABLE" ]; then
        echo -e "${RED}Error: Test executable is not executable${NC}" >&2
        exit 1
    fi
}

# Show test case discovery information
show_test_discovery() {
    echo -e "${CYAN}=== Test Case Discovery ===${NC}"
    
    local test_cases_dir="$PROJECT_ROOT/test_cases"
    if [ -d "$test_cases_dir" ]; then
        local case_count=$(find "$test_cases_dir" -maxdepth 1 -type d ! -path "$test_cases_dir" | wc -l)
        echo -e "${BLUE}Integration test cases directory: ${test_cases_dir}${NC}"
        echo -e "${BLUE}Available test cases: ${case_count}${NC}"
        
        if [ $case_count -gt 0 ]; then
            echo -e "${BLUE}Test cases:${NC}"
            find "$test_cases_dir" -maxdepth 1 -type d ! -path "$test_cases_dir" -exec basename {} \; | sort | while read -r case_name; do
                local layer1_file="$test_cases_dir/$case_name/layer1"
                local layer2_file="$test_cases_dir/$case_name/layer2"
                
                local status="${GREEN}✓${NC}"
                if [ ! -f "$layer1_file" ] || [ ! -f "$layer2_file" ]; then
                    status="${RED}✗${NC}"
                fi
                
                echo -e "  $status $case_name"
            done
        fi
    else
        echo -e "${YELLOW}Warning: Test cases directory not found${NC}"
    fi
    echo ""
}

# Build Google Test filter based on selection
build_gtest_filter() {
    local filters=()
    
    if [ "$RUN_ALL" = true ]; then
        # Run all tests - no filter needed
        echo ""
        return
    fi
    
    if [ "$RUN_LAYER1" = true ]; then
        filters+=("*Layer1IntegrationTest*")
    fi
    
    if [ "$RUN_INTEGRATION" = true ]; then
        filters+=("*IntegrationTest*:*DiscoveryTest*")
    fi
    
    if [ "$RUN_UNIT" = true ]; then
        # Add unit test patterns when they exist
        # For now, this will match nothing since we removed the old unit tests
        filters+=("*UnitTest*")
    fi
    
    # Join filters with ':' for Google Test
    if [ ${#filters[@]} -gt 0 ]; then
        IFS=':' 
        echo "${filters[*]}"
    else
        echo ""
    fi
}

# Run the tests with appropriate filtering and output
run_tests() {
    local gtest_filter=$(build_gtest_filter)
    local gtest_args="--gtest_color=yes"
    
    if [ -n "$gtest_filter" ]; then
        gtest_args="$gtest_args --gtest_filter=$gtest_filter"
    fi
    
    # Show what we're running
    echo -e "${BLUE}=== Running CPrime Compiler Tests ===${NC}"
    
    if [ "$RUN_ALL" = true ]; then
        echo -e "${BLUE}Test scope: All available tests${NC}"
    elif [ "$RUN_LAYER1" = true ]; then
        echo -e "${BLUE}Test scope: Layer 1 integration tests${NC}"
    elif [ "$RUN_INTEGRATION" = true ]; then
        echo -e "${BLUE}Test scope: Integration tests${NC}"
    elif [ "$RUN_UNIT" = true ]; then
        echo -e "${BLUE}Test scope: Unit tests${NC}"
    fi
    
    echo -e "${BLUE}Test executable: $TEST_EXECUTABLE${NC}"
    echo ""
    
    # Change to build directory for test execution
    cd "$BUILD_DIR"
    
    # Run tests with appropriate output level
    local exit_code
    if [ "$VERBOSE" = true ]; then
        # Verbose mode: show all output
        echo -e "${CYAN}Running tests in verbose mode...${NC}"
        echo ""
        ./tests/cprime_tests $gtest_args
        exit_code=$?
    else
        # Normal mode: filtered output with debug logs included
        echo -e "${CYAN}Running tests...${NC}"
        echo ""
        ./tests/cprime_tests $gtest_args 2>&1 | while IFS= read -r line; do
            case "$line" in
                *"[==========]"*|*"[----------]"*|*"[  PASSED  ]"*|*"[  FAILED  ]"*)
                    echo "$line"
                    ;;
                *"PASSED"*|*"FAILED"*|*"Running"*|*"from"*)
                    echo "$line"
                    ;;
                *"RUN"*|*"OK"*|*"FAILED"*)
                    echo "$line"
                    ;;
                # Include debug logs from CPrime logging system (colored format)
                *"D0"*"|"*|*"I0"*"|"*|*"W0"*"|"*|*"E0"*"|"*)
                    echo "$line"
                    ;;
                # Include exception messages and test failures
                *"exception"*|*"Exception"*|*"EXCEPTION"*|*"Failure"*|*"Error"*)
                    echo "$line"
                    ;;
                # Include Google Test trace information
                *"Google Test trace"*|*"SCOPED_TRACE"*|*"Test case:"*)
                    echo "$line"
                    ;;
            esac
        done
        exit_code=${PIPESTATUS[0]}
    fi
    
    echo ""
    
    # Show results and any additional information
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✓ All selected tests passed${NC}"
    else
        echo -e "${RED}✗ Some tests failed${NC}"
        
        # Show information about failure logs for integration tests
        local tmp_dir="$PROJECT_ROOT/tmp"
        if [ -d "$tmp_dir" ]; then
            local failure_logs=$(find "$tmp_dir" -name "*.log" -type f 2>/dev/null | wc -l)
            if [ $failure_logs -gt 0 ]; then
                echo -e "${YELLOW}Integration test failure logs available:${NC}"
                find "$tmp_dir" -name "*.log" -type f 2>/dev/null | while read -r log_file; do
                    echo -e "  ${YELLOW}$log_file${NC}"
                done
            fi
        fi
    fi
    
    return $exit_code
}

# Main execution function
main() {
    # Parse command line arguments
    parse_arguments "$@"
    
    # Validate build system
    check_build_system
    
    # Show test discovery if requested
    if [ "$SHOW_DISCOVERY" = true ]; then
        show_test_discovery
    fi
    
    # Run the tests
    run_tests
    exit_code=$?
    
    # Exit with the same code as the tests
    exit $exit_code
}

# Run main with all arguments
main "$@"