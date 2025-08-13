#!/bin/bash

# CPrime Build System - Common Utilities
# Shared constants, utilities, and functions for build scripts

# Colors for output
export RED='\033[0;31m'
export GREEN='\033[0;32m'
export YELLOW='\033[1;33m'
export BLUE='\033[0;34m'
export CYAN='\033[0;36m'
export BOLD='\033[1m'
export NC='\033[0m' # No Color

# Get the project root directory (parent of scripts/)
get_project_paths() {
    # Find the project root by going up from this script's directory
    local this_file_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
    local scripts_dir="$(dirname "$this_file_dir")"
    export PROJECT_ROOT="$(dirname "$scripts_dir")"
    export SOURCE_DIR="$PROJECT_ROOT/compiler"
    export BUILD_DIR="$PROJECT_ROOT/build"
}

# Get number of CPU cores for parallel build
get_core_count() {
    if command -v nproc &> /dev/null; then
        nproc
    elif command -v sysctl &> /dev/null; then
        sysctl -n hw.ncpu
    else
        echo "4"  # fallback
    fi
}

# Verify required tools are installed
check_build_tools() {
    local missing_tools=()
    
    if ! command -v cmake &> /dev/null; then
        missing_tools+=("cmake")
    fi
    
    if ! command -v make &> /dev/null; then
        missing_tools+=("make")
    fi
    
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        missing_tools+=("C++17 compatible compiler (g++ or clang++)")
    fi
    
    if [ ${#missing_tools[@]} -ne 0 ]; then
        echo -e "${RED}Error: Missing required tools:${NC}" >&2
        for tool in "${missing_tools[@]}"; do
            echo -e "${RED}  - $tool${NC}" >&2
        done
        return 1
    fi
    
    return 0
}

# Verify source directory exists
verify_source_directory() {
    get_project_paths
    if [ ! -d "$SOURCE_DIR" ]; then
        echo -e "${RED}Error: Source directory not found at $SOURCE_DIR${NC}" >&2
        return 1
    fi
    return 0
}

# Check if build completed successfully by verifying expected files exist
verify_build_artifacts() {
    local build_dir="$1"
    
    # Expected build artifacts
    local expected_files=(
        "bin/cprime"
        "src/libcprime_commons.a"
        "src/libcprime_layer0.a"
        "src/libcprime_orchestrator.a"
    )
    
    local missing_files=()
    
    for file in "${expected_files[@]}"; do
        if [ ! -f "$build_dir/$file" ]; then
            missing_files+=("$file")
        fi
    done
    
    if [ ${#missing_files[@]} -ne 0 ]; then
        echo -e "${RED}Error: Build failed - missing components:${NC}" >&2
        for file in "${missing_files[@]}"; do
            echo -e "${RED}  - $file${NC}" >&2
        done
        return 1
    fi
    
    return 0
}

# Clean build directory if it exists
clean_build_directory() {
    local build_dir="$1"
    local quiet_mode="$2"
    
    if [ -d "$build_dir" ]; then
        if [ "$quiet_mode" != "true" ]; then
            echo -e "${YELLOW}Cleaning build directory...${NC}"
        fi
        rm -rf "$build_dir"
        
        # Verify the directory was actually removed
        if [ -d "$build_dir" ]; then
            echo -e "${RED}Error: Failed to clean build directory at $build_dir${NC}" >&2
            exit 1
        fi
    fi
}

# Create build directory
create_build_directory() {
    local build_dir="$1"
    mkdir -p "$build_dir"
}