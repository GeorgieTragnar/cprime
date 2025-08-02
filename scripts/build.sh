#!/bin/bash

# CPrime Compiler Build Script

set -e  # Exit on any error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
COMPILER_DIR="$PROJECT_ROOT/compiler"
BUILD_DIR="$PROJECT_ROOT/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}CPrime Compiler Build Script${NC}"
echo "Project root: $PROJECT_ROOT"

# Check if LLVM is installed
if ! command -v llvm-config &> /dev/null; then
    echo -e "${RED}Error: LLVM not found. Please install LLVM development packages:${NC}"
    echo "  Ubuntu/Debian: sudo apt install llvm-dev llvm clang libclang-dev"
    echo "  Fedora/RHEL:   sudo dnf install llvm-devel clang"
    echo "  Arch:          sudo pacman -S llvm clang"
    echo "  macOS:         brew install llvm"
    exit 1
fi

echo -e "${GREEN}✓ LLVM found:${NC} $(llvm-config --version)"

# Check if clang is installed
if ! command -v clang &> /dev/null; then
    echo -e "${RED}Error: clang not found. Please install clang.${NC}"
    exit 1
fi

echo -e "${GREEN}✓ clang found:${NC} $(clang --version | head -1)"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Run cmake
echo -e "${YELLOW}Running cmake...${NC}"
cmake "$COMPILER_DIR"

# Build
echo -e "${YELLOW}Building cprime compiler...${NC}"
make -j$(nproc)

# Check if build was successful
if [ -f "bin/cprime" ]; then
    echo -e "${GREEN}✓ Build successful!${NC}"
    echo "Compiler binary: $BUILD_DIR/bin/cprime"
    
    # Test with hello world
    if [ -f "examples/hello.cprime" ]; then
        echo -e "${YELLOW}Testing with hello.cprime...${NC}"
        cd "$BUILD_DIR"
        ./bin/cprime examples/hello.cprime -o hello_test
        
        if [ -f "hello_test" ]; then
            echo -e "${GREEN}✓ Test compilation successful!${NC}"
            echo "Running hello_test:"
            ./hello_test
            echo -e "${GREEN}✓ CPrime compiler is working!${NC}"
        else
            echo -e "${RED}✗ Test compilation failed${NC}"
            exit 1
        fi
    fi
else
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi

echo
echo -e "${GREEN}=== CPrime Compiler Ready! ===${NC}"
echo "Usage:"
echo "  $BUILD_DIR/bin/cprime <file.cprime>           # Compile to executable"
echo "  $BUILD_DIR/bin/cprime <file.cprime> -o <name> # Specify output name"
echo "  $BUILD_DIR/bin/cprime <file.cprime> --emit-llvm # Output LLVM IR"
echo "  $BUILD_DIR/bin/cprime <file.cprime> --emit-obj  # Output object file"
echo
echo "Example:"
echo "  cd $BUILD_DIR"
echo "  ./bin/cprime examples/hello.cprime"
echo "  ./a.out"