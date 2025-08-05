#!/bin/bash

# CPrime Compiler - Multi-Layer Architecture Build Script
# This script builds the new CPrime compiler with GPU-ready architecture

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}CPrime Compiler - Multi-Layer Architecture Build${NC}"
echo "================================================"

# Get the project root directory (parent of scripts/)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
SOURCE_DIR="$PROJECT_ROOT/compiler/src"
BUILD_DIR="$SOURCE_DIR/build"

# Verify source directory exists
if [ ! -d "$SOURCE_DIR" ]; then
    echo -e "${RED}Error: Source directory not found at $SOURCE_DIR${NC}"
    exit 1
fi

# Parse command line arguments
BUILD_TYPE="Debug"
CLEAN_BUILD=false
RUN_TESTS=false
VERBOSE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --release)
            BUILD_TYPE="Release"
            echo -e "${YELLOW}Building in Release mode${NC}"
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            echo -e "${YELLOW}Clean build requested${NC}"
            shift
            ;;
        --test)
            RUN_TESTS=true
            echo -e "${YELLOW}Will run tests after build${NC}"
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  --release    Build in Release mode (default: Debug)"
            echo "  --clean      Clean build directory before building"
            echo "  --test       Run tests after successful build"
            echo "  --verbose    Enable verbose output"
            echo "  -h, --help   Show this help message"
            echo ""
            echo "This script builds the CPrime multi-layer compiler architecture:"
            echo "  - Layer 1: Raw tokenization (raw_token.h/cpp)"
            echo "  - Layer 2: Context enrichment (1:1 token mapping)" 
            echo "  - Layer 3: AST Builder (ast_builder.h/cpp)"
            echo "  - Layer 4: Semantic validation & optimization (planned)"
            echo "  - Layer 5: LLVM IR generation (planned)"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

echo "Project root: $PROJECT_ROOT"
echo "Source: $SOURCE_DIR"
echo "Build directory: $BUILD_DIR"

# Clean build if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Create build directory and navigate to it
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Check for required tools
echo -e "${BLUE}Checking build requirements...${NC}"

if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: cmake is required but not installed${NC}"
    exit 1
fi

if ! command -v make &> /dev/null; then
    echo -e "${RED}Error: make is required but not installed${NC}"
    exit 1
fi

# Check for C++17 compiler
if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    echo -e "${RED}Error: C++17 compatible compiler (g++ or clang++) is required${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Build requirements satisfied${NC}"

# Configure with CMake
echo -e "${BLUE}Configuring build with CMake...${NC}"
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"

if [ "$VERBOSE" = true ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_VERBOSE_MAKEFILE=ON"
fi

# CMake should run from the source directory
cmake "$SOURCE_DIR" $CMAKE_ARGS

if [ $? -ne 0 ]; then
    echo -e "${RED}Error: CMake configuration failed${NC}"
    exit 1
fi

echo -e "${GREEN}✓ CMake configuration successful${NC}"

# Build
echo -e "${BLUE}Building compiler components...${NC}"

# Get number of CPU cores for parallel build
if command -v nproc &> /dev/null; then
    CORES=$(nproc)
elif command -v sysctl &> /dev/null; then
    CORES=$(sysctl -n hw.ncpu)
else
    CORES=4  # fallback
fi

if [ "$VERBOSE" = true ]; then
    make -j$CORES VERBOSE=1
else
    make -j$CORES
fi

if [ $? -ne 0 ]; then
    echo -e "${RED}Error: Build failed${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Build successful${NC}"

# List built artifacts
echo -e "${BLUE}Built artifacts:${NC}"
if [ -f "libcprime_compiler.a" ]; then
    echo -e "  ${GREEN}✓ libcprime_compiler.a${NC} - Multi-layer compiler architecture library"
fi
if [ -f "cprime_cli" ]; then
    echo -e "  ${GREEN}✓ cprime_cli${NC} - CPrime compiler CLI"
fi
if [ -f "test_three_layer" ]; then
    echo -e "  ${GREEN}✓ test_three_layer${NC} - Test executable for context-sensitive parsing"
fi
if [ -f "test_contextual_tokens" ]; then
    echo -e "  ${GREEN}✓ test_contextual_tokens${NC} - Context enrichment demo"
fi
if [ -f "test_ast_builder" ]; then
    echo -e "  ${GREEN}✓ test_ast_builder${NC} - AST building test"
fi

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    echo -e "${BLUE}Running compiler architecture tests...${NC}"
    
    if [ -f "test_contextual_tokens" ]; then
        echo -e "${YELLOW}Testing context enrichment...${NC}"
        ./test_contextual_tokens
        if [ $? -ne 0 ]; then
            echo -e "${RED}✗ Context enrichment test failed${NC}"
            exit 1
        fi
    fi
    
    if [ -f "test_ast_builder" ]; then
        echo -e "${YELLOW}Testing AST building...${NC}"
        ./test_ast_builder
        if [ $? -ne 0 ]; then
            echo -e "${RED}✗ AST builder test failed${NC}"
            exit 1
        fi
    fi
    
    if [ -f "test_three_layer" ]; then
        echo -e "${YELLOW}Testing full pipeline...${NC}"
        ./test_three_layer
        if [ $? -ne 0 ]; then
            echo -e "${RED}✗ Full pipeline test failed${NC}"
            exit 1
        fi
    fi
    
    echo -e "${GREEN}✓ All tests passed${NC}"
fi

echo ""
echo -e "${GREEN}CPrime Compiler Build completed successfully!${NC}"
echo ""
echo "Multi-layer architecture components built:"
echo "  - Layer 1: Raw tokenization with C++ compatibility"
echo "  - Layer 2: Context enrichment with 1:1 token mapping"
echo "  - Layer 3: AST building with symbol table population"
echo "  - Test suite demonstrating GPU-ready architecture"
echo ""
echo "To run the CLI:"
echo "  cd $BUILD_DIR && echo 'class Test {}' | ./cprime_cli --build-ast --dump-ast"
echo ""
echo "To run tests:"
echo "  cd $BUILD_DIR && ./test_contextual_tokens"
echo "  cd $BUILD_DIR && ./test_ast_builder"
echo ""
echo "Library location:"
echo "  $BUILD_DIR/libcprime_compiler.a"
echo ""
echo "Next steps:"
echo "  - Implement semantic validator and optimizer (Layer 4)"
echo "  - Add LLVM IR generation (Layer 5)"
echo "  - GPU kernel implementation for parallel processing"