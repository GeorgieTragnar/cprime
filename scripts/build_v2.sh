#!/bin/bash

# CPrime Compiler V2 - Three-Layer Architecture Build Script
# This script builds the V2 components independently from the main compiler

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}CPrime Compiler V2 - Three-Layer Architecture Build${NC}"
echo "=================================================="

# Get the project root directory (parent of scripts/)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
V2_SOURCE_DIR="$PROJECT_ROOT/compiler/src/v2"
BUILD_DIR="$V2_SOURCE_DIR/build"

# Verify V2 source directory exists
if [ ! -d "$V2_SOURCE_DIR" ]; then
    echo -e "${RED}Error: V2 source directory not found at $V2_SOURCE_DIR${NC}"
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
            echo "This script builds the CPrime V2 three-layer compiler architecture:"
            echo "  - Layer 1: Raw tokenization (raw_token.h/cpp)"
            echo "  - Layer 2: Semantic translation (semantic_translator.h/cpp)" 
            echo "  - Layer 3: LLVM codegen (planned)"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

echo "Project root: $PROJECT_ROOT"
echo "V2 source: $V2_SOURCE_DIR"
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

# CMake should run from the V2 source directory
cmake "$V2_SOURCE_DIR" $CMAKE_ARGS

if [ $? -ne 0 ]; then
    echo -e "${RED}Error: CMake configuration failed${NC}"
    exit 1
fi

echo -e "${GREEN}✓ CMake configuration successful${NC}"

# Build
echo -e "${BLUE}Building V2 components...${NC}"

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
if [ -f "libcprime_v2.a" ]; then
    echo -e "  ${GREEN}✓ libcprime_v2.a${NC} - V2 three-layer architecture library"
fi
if [ -f "test_three_layer" ]; then
    echo -e "  ${GREEN}✓ test_three_layer${NC} - Test executable for context-sensitive parsing"
fi

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    echo -e "${BLUE}Running V2 architecture tests...${NC}"
    
    if [ -f "test_three_layer" ]; then
        echo -e "${YELLOW}Testing raw tokenization, context resolution, and semantic translation...${NC}"
        ./test_three_layer
        
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓ All V2 tests passed${NC}"
        else
            echo -e "${RED}✗ Some V2 tests failed${NC}"
            exit 1
        fi
    else
        echo -e "${RED}Error: test_three_layer executable not found${NC}"
        exit 1
    fi
fi

echo ""
echo -e "${GREEN}V2 Build completed successfully!${NC}"
echo ""
echo "Three-layer architecture components built:"
echo "  - Raw tokenization (Layer 1)"
echo "  - Semantic translation with context-sensitive keywords (Layer 2)"
echo "  - Test suite demonstrating context resolution"
echo ""
echo "To run the test suite:"
echo "  cd $BUILD_DIR && ./test_three_layer"
echo ""
echo "Library location:"
echo "  $BUILD_DIR/libcprime_v2.a"
echo ""
echo "To integrate with main compiler, link against:"
echo "  -L$BUILD_DIR -lcprime_v2"
echo ""
echo "Next steps:"
echo "  - Implement Layer 3: LLVM IR generation (llvm_codegen_v2.h)"
echo "  - Add feature flags for hybrid V1/V2 compilation"
echo "  - Integrate with main compiler pipeline"