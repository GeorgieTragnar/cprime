#!/bin/bash

# CPrime Analyzer - Debug Analysis Script for CPrime Files
# Easy analysis of example CPrime files using the CLI debugging tool

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
CPRIME_CLI="$PROJECT_ROOT/build/bin/cprime_cli"
EXAMPLES_DIR="$PROJECT_ROOT/examples"

# Default values
MODE=""
EXAMPLE_NAME=""
OUTPUT_DIR="cprime_analysis"
OUTPUT_FILE=""
OPEN_RESULT=false
VERBOSE=false
QUIET_MODE=false
EXTRA_FLAGS=""

# Usage function
show_help() {
    echo -e "${BOLD}CPrime Analysis Script${NC}"
    echo "====================="
    echo ""
    echo "Usage: $0 [MODE] [OPTIONS] <example_name>"
    echo ""
    echo -e "${BOLD}Arguments:${NC}"
    echo "  example_name    Name of the example file to analyze (without extension)"
    echo "                  Will look for examples/{name}.cprime first, then examples/{name}.cp"
    echo ""
    echo -e "${BOLD}LAYER 0 MODES (Input Processing - AVAILABLE):${NC}"
    echo "  input        Debug input file processing pipeline"
    echo "  streams      Analyze processed stringstreams in detail"  
    echo "  validation   Show file validation process"
    echo "  layer0       Complete Layer 0 analysis (input + streams + validation)"
    echo ""
    echo -e "${BOLD}LAYER 1 MODES (Tokenization - AVAILABLE):${NC}"
    echo "  tokens       Analyze and dump raw tokens (Layer 1)"
    echo ""
    echo -e "${BOLD}FUTURE LAYER MODES (Not Yet Implemented):${NC}"
    echo "  context      Analyze context-sensitive keyword resolution (Layer 2)"
    echo "  ast          Build and analyze AST structure (Layer 3)"
    echo "  full         Run complete pipeline analysis (all layers)"
    echo "  interactive  Interactive mode for quick testing"
    echo ""
    echo -e "${BOLD}OPTIONS:${NC}"
    echo "  -v, --verbose        Enable verbose debug output"
    echo "  --quiet             Suppress verbose headers and file info"
    echo "  -o, --output FILE   Write token output to file (for tokens mode)"
    echo "  -h, --help          Show this help message"
    echo "  --                  Pass remaining arguments directly to CLI"
    echo ""
    echo -e "${BOLD}EXAMPLES:${NC}"
    echo "  $0 input hello                      # Debug input processing for hello.cprime"
    echo "  $0 streams hello                    # Analyze stringstreams for hello.cprime"
    echo "  $0 validation simple                # Show file validation for simple.cp"
    echo "  $0 layer0 class_test                # Complete Layer 0 analysis"
    echo "  $0 tokens test_custom_arithmetic     # Dump Layer 1 tokens"
    echo "  $0 tokens arithmetic_operators -o layer2  # Save tokens to file"
    echo "  $0 layer0 -v hello                  # Verbose Layer 0 analysis"
    echo ""
    echo -e "${BOLD}AVAILABLE EXAMPLES:${NC}"
    if [ -d "$EXAMPLES_DIR" ]; then
        for file in "$EXAMPLES_DIR"/*.cprime "$EXAMPLES_DIR"/*.cp; do
            if [ -f "$file" ]; then
                basename "$file" | sed 's/\.[^.]*$//'
            fi
        done | sort | uniq | sed 's/^/  /'
    fi
    echo ""
    echo -e "${BOLD}CURRENT FOCUS - LAYER 0:${NC}"
    echo "  Layer 0 handles input file processing - the foundation for compilation"
    echo "  Debug capabilities help understand file-to-stringstream conversion"
    echo "  This layer validates files and prepares them for tokenization"
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        # Layer 0 modes (implemented)
        input|streams|validation|layer0)
            if [ -n "$MODE" ]; then
                echo -e "${RED}Error: Multiple modes specified${NC}"
                exit 1
            fi
            MODE="$1"
            shift
            ;;
        # Layer 1 modes (implemented)
        tokens)
            if [ -n "$MODE" ]; then
                echo -e "${RED}Error: Multiple modes specified${NC}"
                exit 1
            fi
            MODE="$1"
            shift
            ;;
        # Future layer modes (not yet implemented)
        context|ast|full|interactive)
            if [ -n "$MODE" ]; then
                echo -e "${RED}Error: Multiple modes specified${NC}"
                exit 1
            fi
            MODE="$1"
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        --quiet)
            QUIET_MODE=true
            shift
            ;;
        -o|--output)
            if [ -z "$2" ]; then
                echo -e "${RED}Error: --output requires a filename${NC}"
                exit 1
            fi
            OUTPUT_FILE="$2"
            shift 2
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
            if [ -z "$MODE" ]; then
                # First positional argument is the mode
                case $1 in
                    input|streams|validation|layer0|tokens|context|ast|full|interactive)
                        MODE="$1"
                        ;;
                    *)
                        # Not a mode, must be example name with default mode
                        EXAMPLE_NAME="$1"
                        ;;
                esac
            elif [ -z "$EXAMPLE_NAME" ]; then
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

# Default mode if not specified
if [ -z "$MODE" ]; then
    if [ "$QUIET_MODE" = false ]; then
        echo -e "${YELLOW}No mode specified, defaulting to 'layer0'${NC}"
    fi
    MODE="layer0"
fi

# Check if CLI exists
if [ ! -f "$CPRIME_CLI" ]; then
    echo -e "${RED}Error: CPrime CLI not found at $CPRIME_CLI${NC}"
    echo "Please run: ./scripts/build.sh to build the compiler first"
    exit 1
fi

if [ ! -x "$CPRIME_CLI" ]; then
    echo -e "${RED}Error: CPrime CLI is not executable${NC}"
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

# Display header information (unless quiet mode)
if [ "$QUIET_MODE" = false ]; then
    echo -e "${BOLD}${BLUE}CPrime Code Analyzer${NC}"
    echo "==================="
    echo ""
    echo -e "${BOLD}${BLUE}=== Analyzing $(basename "$EXAMPLE_FILE") (Mode: $MODE) ===${NC}"
    
    # Show file information
    FILE_SIZE=$(stat -f%z "$EXAMPLE_FILE" 2>/dev/null || stat -c%s "$EXAMPLE_FILE" 2>/dev/null || echo "unknown")
    CHAR_COUNT=$(wc -c < "$EXAMPLE_FILE" 2>/dev/null || echo "unknown")
    LINE_COUNT=$(wc -l < "$EXAMPLE_FILE" 2>/dev/null || echo "unknown")
    
    echo -e "${CYAN}File:${NC} $EXAMPLE_FILE"
    echo -e "${CYAN}Size:${NC} $FILE_SIZE bytes, $CHAR_COUNT characters, $LINE_COUNT lines"
    echo -e "${CYAN}Mode:${NC} $MODE"
    if [ "$VERBOSE" = true ]; then
        echo -e "${CYAN}Verbose:${NC} enabled"
    fi
    if [ -n "$EXTRA_FLAGS" ]; then
        echo -e "${CYAN}Extra flags:${NC} $EXTRA_FLAGS"
    fi
    echo ""
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Helper function to run Layer 1 token analysis with the CLI
run_layer1_token_analysis() {
    local output_file="$1"  # Optional output file parameter
    
    if [ "$QUIET_MODE" = false ]; then
        echo -e "${CYAN}Running Layer 1 token dumping analysis...${NC}"
    fi
    
    # Prepare CLI arguments
    local cli_args="--dump-tokens"
    
    # Add output file if specified
    if [ -n "$output_file" ]; then
        cli_args="$cli_args -o $output_file"
        if [ "$QUIET_MODE" = false ]; then
            echo -e "${CYAN}Output will be written to: $output_file${NC}"
        fi
    fi
    
    # Add verbose flag if requested
    if [ "$VERBOSE" = true ]; then
        cli_args="$cli_args --verbose"
    fi
    
    # Add extra flags if specified
    if [ -n "$EXTRA_FLAGS" ]; then
        cli_args="$cli_args $EXTRA_FLAGS"
    fi
    
    if [ "$VERBOSE" = true ]; then
        echo "Input file: $EXAMPLE_FILE"
        echo "CLI command: $CPRIME_CLI $cli_args $EXAMPLE_FILE"
    fi
    
    # Execute the CLI
    $CPRIME_CLI $cli_args "$EXAMPLE_FILE"
    
    local exit_code=$?
    if [ $exit_code -eq 0 ]; then
        if [ "$QUIET_MODE" = false ]; then
            echo -e "${GREEN}✓ Layer 1 token analysis completed${NC}"
            if [ -n "$output_file" ]; then
                echo -e "${CYAN}Token output saved to: $output_file${NC}"
            fi
        fi
    else
        echo -e "${RED}✗ Layer 1 token analysis failed (exit code: $exit_code)${NC}"
        exit 1
    fi
}

# Helper function to run Layer 0 analysis with the CLI
run_layer0_analysis() {
    local mode="$1"
    
    if [ "$QUIET_MODE" = false ]; then
        echo -e "${CYAN}Running Layer 0 $mode analysis...${NC}"
    fi
    
    # Prepare CLI arguments
    local cli_args=""
    
    # Map mode to CLI flags
    case "$mode" in
        "debug-input")
            cli_args="--debug-input"
            ;;
        "analyze-streams")
            cli_args="--debug-input --analyze-streams"
            ;;
        "show-file-validation")
            cli_args="--show-file-validation"
            ;;
        *)
            echo -e "${RED}Unknown Layer 0 mode: $mode${NC}"
            exit 1
            ;;
    esac
    
    # Add verbose flag if requested
    if [ "$VERBOSE" = true ]; then
        cli_args="$cli_args --verbose"
    fi
    
    # Add extra flags if specified
    if [ -n "$EXTRA_FLAGS" ]; then
        cli_args="$cli_args $EXTRA_FLAGS"
    fi
    
    if [ "$VERBOSE" = true ]; then
        echo "Input file: $EXAMPLE_FILE"
        echo "CLI command: $CPRIME_CLI $cli_args $EXAMPLE_FILE"
    fi
    
    # Execute the CLI
    $CPRIME_CLI $cli_args "$EXAMPLE_FILE"
    
    local exit_code=$?
    if [ $exit_code -eq 0 ]; then
        if [ "$QUIET_MODE" = false ]; then
            echo -e "${GREEN}✓ Layer 0 $mode analysis completed${NC}"
        fi
    else
        echo -e "${RED}✗ Layer 0 $mode analysis failed (exit code: $exit_code)${NC}"
        exit 1
    fi
}


# Main execution based on mode
case $MODE in
    # Layer 0 modes (implemented)
    input)
        run_layer0_analysis "debug-input"
        ;;
    streams)
        run_layer0_analysis "analyze-streams"
        ;;
    validation)
        run_layer0_analysis "show-file-validation"
        ;;
    layer0)
        run_layer0_analysis "debug-input"
        run_layer0_analysis "analyze-streams"
        run_layer0_analysis "show-file-validation"
        ;;
    # Layer 1 modes (implemented)
    tokens)
        run_layer1_token_analysis "$OUTPUT_FILE"
        ;;
    # Future layer modes (not yet implemented)
    context)
        echo -e "${RED}Context analysis (Layer 2) not yet implemented${NC}"
        echo "This will analyze context resolution using future CLI capabilities"
        exit 1
        ;;
    ast)
        echo -e "${RED}AST analysis (Layer 3) not yet implemented${NC}"
        echo "This will analyze AST building using future CLI capabilities"
        exit 1
        ;;
    full)
        echo -e "${RED}Full pipeline analysis not yet implemented${NC}"
        echo "This will run all layers when implemented"
        exit 1
        ;;
    interactive)
        echo -e "${RED}Interactive mode not yet implemented${NC}"
        exit 1
        ;;
    *)
        echo -e "${RED}Error: Unknown mode '$MODE'${NC}"
        exit 1
        ;;
esac

# Final completion message
if [ "$QUIET_MODE" = false ]; then
    echo ""
    echo -e "${GREEN}Analysis completed successfully!${NC}"
    
    # Show what was analyzed
    case $MODE in
        layer0)
            echo -e "${CYAN}Completed Layer 0 analysis: input processing, streams, and validation${NC}"
            ;;
        input)
            echo -e "${CYAN}Completed Layer 0 input processing analysis${NC}"
            ;;
        streams)
            echo -e "${CYAN}Completed Layer 0 streams analysis${NC}"
            ;;
        tokens)
            echo -e "${CYAN}Completed Layer 1 token dumping analysis${NC}"
            ;;
        validation)
            echo -e "${CYAN}Completed Layer 0 validation analysis${NC}"
            ;;
        *)
            echo -e "${CYAN}Completed $MODE analysis${NC}"
            ;;
    esac
fi