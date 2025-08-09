#!/bin/bash

# CPrime Compiler Build Script
# Clean, minimal output with conditional test building

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
SOURCE_DIR="$PROJECT_ROOT/compiler"
BUILD_DIR="$PROJECT_ROOT/build"

# Verify source directory exists
if [ ! -d "$SOURCE_DIR" ]; then
    echo -e "${RED}Error: Source directory not found at $SOURCE_DIR${NC}"
    exit 1
fi

# Parse command line arguments
BUILD_TYPE="Debug"
CLEAN_BUILD=false
BUILD_TESTS=false
VERBOSE=false

# Use getopts for proper short option parsing
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
            exit 0
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

# Clean build if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Create build directory and navigate to it
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Check for required tools
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: cmake is required but not installed${NC}"
    exit 1
fi

if ! command -v make &> /dev/null; then
    echo -e "${RED}Error: make is required but not installed${NC}"
    exit 1
fi

if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    echo -e "${RED}Error: C++17 compatible compiler is required${NC}"
    exit 1
fi

# Configure with CMake
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"

# Set test building based on flag
if [ "$BUILD_TESTS" = true ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_TESTS=ON"
else
    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_TESTS=OFF"
fi

if [ "$VERBOSE" = true ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_VERBOSE_MAKEFILE=ON"
    echo "Configuring with: cmake $SOURCE_DIR $CMAKE_ARGS"
fi

# CMake configuration
if [ "$VERBOSE" = true ]; then
    cmake "$SOURCE_DIR" $CMAKE_ARGS
else
    cmake "$SOURCE_DIR" $CMAKE_ARGS > /dev/null 2>&1
fi

if [ $? -ne 0 ]; then
    echo -e "${RED}Error: CMake configuration failed${NC}"
    exit 1
fi

# Build
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
    make -j$CORES 2>/dev/null
fi

if [ $? -ne 0 ]; then
    echo -e "${RED}Error: Build failed${NC}"
    exit 1
fi

# Build completion
if [ -f "src/libcprime_compiler.a" ]; then
    echo -e "${GREEN}✓ CPrime compiler built successfully${NC}"
else
    echo -e "${RED}Error: Compiler build failed - library not found${NC}"
    exit 1
fi

if [ "$BUILD_TESTS" = true ]; then
    if [ -f "tests/cprime_tests" ]; then
        echo -e "${GREEN}✓ Test suite built successfully${NC}"
    else
        echo -e "${YELLOW}⚠ Test suite build failed${NC}"
    fi
fi