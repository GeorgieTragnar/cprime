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
    
    # Test with all .cprime files in examples folder
    echo -e "${YELLOW}Testing all .cprime files in examples...${NC}"
    cd "$BUILD_DIR"
    
    test_passed=0
    test_total=0
    
    for cprime_file in examples/*.cprime; do
        if [ -f "$cprime_file" ]; then
            test_total=$((test_total + 1))
            filename=$(basename "$cprime_file" .cprime)
            output_name="${filename}_test"
            
            echo -e "${YELLOW}Testing $cprime_file...${NC}"
            
            # Compile the file
            if ./bin/cprime "$cprime_file" -o "$output_name"; then
                if [ -f "$output_name" ]; then
                    echo -e "${GREEN}✓ $filename compilation successful${NC}"
                    echo "Running $output_name:"
                    echo "----------------------------------------"
                    if ./"$output_name"; then
                        echo "----------------------------------------"
                        echo -e "${GREEN}✓ $filename execution successful${NC}"
                        test_passed=$((test_passed + 1))
                        # Clean up the executable
                        rm -f "$output_name"
                    else
                        echo "----------------------------------------"
                        echo -e "${RED}✗ $filename execution failed${NC}"
                    fi
                else
                    echo -e "${RED}✗ $filename compilation failed - no output file${NC}"
                fi
            else
                echo -e "${RED}✗ $filename compilation failed${NC}"
            fi
            echo
        fi
    done
    
    # Summary
    if [ $test_total -eq 0 ]; then
        echo -e "${YELLOW}No .cprime test files found in examples/${NC}"
    else
        echo -e "${GREEN}=== Test Summary ===${NC}"
        echo "Passed: $test_passed/$test_total tests"
        
        if [ $test_passed -eq $test_total ]; then
            echo -e "${GREEN}✓ All tests passed!${NC}"
        else
            echo -e "${RED}✗ Some tests failed${NC}"
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