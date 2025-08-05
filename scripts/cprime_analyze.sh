#!/bin/bash

# CPrime Analyzer - Convenience Script for Compiler CLI
# This script provides easy access to tokenization, AST building and debugging capabilities

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Get the project root directory (parent of scripts/)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CPRIME_CLI="$PROJECT_ROOT/compiler/src/build/cprime_cli"

echo -e "${BLUE}CPrime Code Analyzer${NC}"
echo "==================="

# Check if CLI exists
if [ ! -f "$CPRIME_CLI" ]; then
    echo -e "${RED}Error: CPrime CLI not found at $CPRIME_CLI${NC}"
    echo "Please run: ./scripts/build.sh to build the compiler first"
    exit 1
fi

# Default values
MODE=""
INPUT_FILE=""
OUTPUT_DIR="cprime_analysis"
OPEN_RESULT=false
VERBOSE=false

# Usage function
show_help() {
    echo "Usage: $0 [MODE] [OPTIONS] [input_file]"
    echo ""
    echo "MODES:"
    echo "  tokens       Analyze and dump raw tokens"
    echo "  context      Analyze context-sensitive keyword resolution"
    echo "  ast          Build and analyze AST structure"
    echo "  full         Run full pipeline analysis (tokens + context + AST)"
    echo "  interactive  Interactive mode for quick testing"
    echo ""
    echo "OPTIONS:"
    echo "  -o, --output-dir DIR  Output directory for analysis files (default: cprime_analysis)"
    echo "  --open               Open results with default viewer after analysis"
    echo "  -v, --verbose        Enable verbose output"
    echo "  -h, --help          Show this help message"
    echo ""
    echo "EXAMPLES:"
    echo "  $0 tokens src/main.cp              # Analyze tokens from file"
    echo "  $0 context src/main.cp --open      # Analyze context and open result"
    echo "  $0 ast src/main.cp                 # Build and analyze AST"
    echo "  $0 full src/main.cp -o debug/      # Full analysis to debug/ folder"
    echo "  $0 interactive                     # Start interactive mode"
    echo "  echo 'class Foo {}' | $0 ast       # Build AST from stdin"
    echo ""
    echo "INTERACTIVE MODE:"
    echo "  In interactive mode, you can type CPrime code and see immediate analysis."
    echo "  Type 'quit' or 'exit' to leave interactive mode."
    echo ""
    echo "OUTPUT FILES:"
    echo "  - tokens.txt: Raw token dump with positions"
    echo "  - context.txt: Context-sensitive analysis with keyword resolution"
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        tokens|context|ast|full|interactive)
            if [ -n "$MODE" ]; then
                echo -e "${RED}Error: Multiple modes specified${NC}"
                exit 1
            fi
            MODE="$1"
            shift
            ;;
        -o|--output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --open)
            OPEN_RESULT=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        -*)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
        *)
            if [ -n "$INPUT_FILE" ]; then
                echo -e "${RED}Error: Multiple input files specified${NC}"
                exit 1
            fi
            INPUT_FILE="$1"
            shift
            ;;
    esac
done

# Default mode if not specified
if [ -z "$MODE" ]; then
    echo -e "${YELLOW}No mode specified, defaulting to 'full'${NC}"
    MODE="full"
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Helper function to run analysis
run_analysis() {
    local mode="$1"
    local input="$2"
    local output_file="$3"
    
    echo -e "${CYAN}Running $mode analysis...${NC}"
    
    if [ -n "$input" ]; then
        if [ "$VERBOSE" = true ]; then
            echo "Input file: $input"
            echo "Output file: $output_file"
        fi
        "$CPRIME_CLI" "--$mode" -o "$output_file" "$input"
    else
        # Read from stdin
        "$CPRIME_CLI" "--$mode" -o "$output_file"
    fi
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ $mode analysis completed: $output_file${NC}"
        
        # Show preview of results
        if [ "$VERBOSE" = true ]; then
            echo -e "${CYAN}Preview (first 10 lines):${NC}"
            head -10 "$output_file" | while IFS= read -r line; do
                echo "  $line"
            done
            echo ""
        fi
    else
        echo -e "${RED}✗ $mode analysis failed${NC}"
        exit 1
    fi
}

# Helper function to open results
open_results() {
    if [ "$OPEN_RESULT" = true ]; then
        echo -e "${CYAN}Opening results...${NC}"
        
        # Try different viewers
        if command -v code &> /dev/null; then
            code "$OUTPUT_DIR"
        elif command -v xdg-open &> /dev/null; then
            xdg-open "$OUTPUT_DIR"
        elif command -v open &> /dev/null; then
            open "$OUTPUT_DIR"
        else
            echo -e "${YELLOW}No suitable viewer found. Results are in: $OUTPUT_DIR${NC}"
        fi
    fi
}

# Interactive mode
run_interactive() {
    echo -e "${CYAN}Starting interactive CPrime analysis mode${NC}"
    echo "Type CPrime code and press Enter to analyze. Type 'quit' or 'exit' to leave."
    echo ""
    
    local counter=1
    
    while true; do
        echo -n -e "${BLUE}cprime> ${NC}"
        read -r input
        
        if [ "$input" = "quit" ] || [ "$input" = "exit" ]; then
            echo -e "${GREEN}Goodbye!${NC}"
            break
        fi
        
        if [ -z "$input" ]; then
            continue
        fi
        
        echo -e "${CYAN}Analyzing: ${NC}$input"
        echo ""
        
        # Create temporary files
        local temp_tokens="$OUTPUT_DIR/interactive_${counter}_tokens.txt"
        local temp_context="$OUTPUT_DIR/interactive_${counter}_context.txt"
        
        # Run both analyses
        echo "$input" | "$CPRIME_CLI" --dump-tokens -o "$temp_tokens"
        echo "$input" | "$CPRIME_CLI" --debug-context -o "$temp_context"
        
        # Show compact results
        echo -e "${YELLOW}=== TOKENS ===${NC}"
        grep "RawToken" "$temp_tokens" | head -5
        if [ $(grep -c "RawToken" "$temp_tokens") -gt 5 ]; then
            echo "... ($(grep -c "RawToken" "$temp_tokens") total tokens, see $temp_tokens for full output)"
        fi
        
        echo ""
        echo -e "${YELLOW}=== CONTEXT RESOLUTION ===${NC}"
        grep -E "(Keyword.*resolved|-> Pushed|-> Popped)" "$temp_context" | head -5
        echo ""
        
        ((counter++))
    done
}

# Main execution based on mode
case $MODE in
    tokens)
        run_analysis "dump-tokens" "$INPUT_FILE" "$OUTPUT_DIR/tokens.txt"
        ;;
    context)
        run_analysis "debug-context" "$INPUT_FILE" "$OUTPUT_DIR/context.txt"
        ;;
    ast)
        run_analysis "build-ast" "$INPUT_FILE" "$OUTPUT_DIR/ast.txt"
        ;;
    full)
        run_analysis "dump-tokens" "$INPUT_FILE" "$OUTPUT_DIR/tokens.txt"
        run_analysis "debug-context" "$INPUT_FILE" "$OUTPUT_DIR/context.txt"
        run_analysis "build-ast" "$INPUT_FILE" "$OUTPUT_DIR/ast.txt"
        ;;
    interactive)
        run_interactive
        ;;
    *)
        echo -e "${RED}Error: Unknown mode '$MODE'${NC}"
        exit 1
        ;;
esac

# Open results if requested
if [ "$MODE" != "interactive" ]; then
    open_results
    
    echo ""
    echo -e "${GREEN}Analysis completed successfully!${NC}"
    echo -e "${CYAN}Results saved to: $OUTPUT_DIR${NC}"
    
    if [ "$MODE" = "full" ] || [ "$MODE" = "tokens" ]; then
        echo -e "  📄 Token analysis: $OUTPUT_DIR/tokens.txt"
    fi
    if [ "$MODE" = "full" ] || [ "$MODE" = "context" ]; then
        echo -e "  🔍 Context analysis: $OUTPUT_DIR/context.txt"
    fi
    if [ "$MODE" = "full" ] || [ "$MODE" = "ast" ]; then
        echo -e "  🌳 AST analysis: $OUTPUT_DIR/ast.txt"
    fi
fi