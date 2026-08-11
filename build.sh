#!/bin/bash
###############################################################################
# GameEngine Build Automation Script
#
# This script automates the complete Conan + CMake + Ninja build pipeline
# for the SDL3/bgfx game engine.
#
# Usage:
#   ./build.sh [target]              # Build specific target (default: sdl3_app)
#   ./build.sh test_bootstrap_orchestration  # Build first test
#   ./build.sh --clean              # Clean and rebuild from scratch
#   ./build.sh --list-targets        # List all available targets
#   ./build.sh --help               # Show this help message
###############################################################################

set -e  # Exit on first error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build/Release"
SOURCE_DIR="${PROJECT_ROOT}"
CMAKE_CONFIG="${PROJECT_ROOT}/cmake_config.json"
CMAKE_TEMPLATE="${PROJECT_ROOT}/CMakeLists.txt.jinja2"
CMAKE_OUTPUT="${PROJECT_ROOT}/CMakeLists.txt"
GENERATE_SCRIPT="${PROJECT_ROOT}/generate_cmake.py"
BUILD_TYPE="Release"
GENERATOR="Ninja"
VERBOSE=0
CLEAN_BUILD=0

# Functions
print_section() {
    echo ""
    echo -e "${BLUE}===============================================================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}===============================================================================${NC}"
}

print_step() {
    echo -e "${GREEN}▶ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ ERROR: $1${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ WARNING: $1${NC}"
}

show_help() {
    cat "${BASH_SOURCE[0]}" | grep "^#" | tail -n +2 | head -n 20
}

list_targets() {
    if [ -f "${CMAKE_OUTPUT}" ]; then
        print_section "Available CMake Targets"
        # Extract executable and test targets from CMakeLists.txt
        grep -E "^add_executable|^add_test" "${CMAKE_OUTPUT}" | sed 's/add_executable(\([^ ]*\).*/  - \1 (executable)/' | sed 's/add_test(\([^ ]*\).*/  - \1 (test)/' | sort | uniq
    else
        print_error "CMakeLists.txt not found. Please run build step first."
        exit 1
    fi
}

verify_requirements() {
    print_section "Verifying Requirements"

    local missing=0

    # Check for Conan
    if ! command -v conan &> /dev/null; then
        print_error "Conan is not installed"
        missing=1
    else
        print_step "Conan: $(conan --version)"
    fi

    # Check for CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake is not installed"
        missing=1
    else
        print_step "CMake: $(cmake --version | head -1)"
    fi

    # Check for Ninja
    if ! command -v ninja &> /dev/null; then
        print_error "Ninja is not installed"
        missing=1
    else
        print_step "Ninja: $(ninja --version)"
    fi

    # Check for Python 3
    if ! command -v python3 &> /dev/null; then
        print_error "Python 3 is not installed"
        missing=1
    else
        print_step "Python: $(python3 --version)"
    fi

    # Check for Jinja2
    if ! python3 -c "import jinja2" 2>/dev/null; then
        print_error "Python Jinja2 is not installed"
        print_step "Install with: pip3 install jinja2"
        missing=1
    else
        print_step "Jinja2: Available"
    fi

    if [ $missing -eq 1 ]; then
        print_error "Some required tools are missing. Please install them and try again."
        exit 1
    fi

    print_success "All requirements verified"
}

install_conan_deps() {
    print_section "Installing Conan Dependencies"

    if [ ! -f "${PROJECT_ROOT}/conanfile.py" ]; then
        print_error "conanfile.py not found at ${PROJECT_ROOT}/conanfile.py"
        exit 1
    fi

    print_step "Running: conan install . --build=missing"
    cd "${PROJECT_ROOT}"

    if [ $VERBOSE -eq 1 ]; then
        conan install . --build=missing -vv
    else
        conan install . --build=missing
    fi

    print_success "Conan dependencies installed"
}

generate_cmakelists() {
    print_section "Generating CMakeLists.txt"

    if [ ! -f "${CMAKE_CONFIG}" ]; then
        print_error "cmake_config.json not found at ${CMAKE_CONFIG}"
        exit 1
    fi

    if [ ! -f "${GENERATE_SCRIPT}" ]; then
        print_error "generate_cmake.py not found at ${GENERATE_SCRIPT}"
        exit 1
    fi

    print_step "Running: python3 generate_cmake.py --config cmake_config.json --template CMakeLists.txt.jinja2 --output CMakeLists.txt"
    cd "${PROJECT_ROOT}"
    python3 generate_cmake.py --config cmake_config.json --template CMakeLists.txt.jinja2 --output CMakeLists.txt

    if [ ! -f "${CMAKE_OUTPUT}" ]; then
        print_error "CMakeLists.txt generation failed"
        exit 1
    fi

    print_success "CMakeLists.txt generated ($(wc -l < "${CMAKE_OUTPUT}") lines)"
}

configure_cmake() {
    print_section "Configuring CMake"

    # Create build directory
    mkdir -p "${BUILD_DIR}"

    print_step "Running: cmake -B ${BUILD_DIR} -S ${SOURCE_DIR} ..."

    # Construct cmake command
    local cmake_cmd=(
        cmake
        "-B" "${BUILD_DIR}"
        "-S" "${SOURCE_DIR}"
        "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
        "-DCMAKE_TOOLCHAIN_FILE=${BUILD_DIR}/generators/conan_toolchain.cmake"
        "-G" "${GENERATOR}"
        "-DBUILD_SDL3_APP=ON"
        "-DSDL_VERSION=SDL3"
    )

    if [ $VERBOSE -eq 1 ]; then
        cmake_cmd+=("--debug-output")
    fi

    "${cmake_cmd[@]}"

    print_success "CMake configuration complete"
}

build_target() {
    local target="${1:-sdl3_app}"

    print_section "Building Target: $target"

    if [ ! -d "${BUILD_DIR}" ]; then
        print_error "Build directory not configured. Please run full build first."
        exit 1
    fi

    print_step "Running: cmake --build ${BUILD_DIR} --target $target"

    if [ $VERBOSE -eq 1 ]; then
        cmake --build "${BUILD_DIR}" --target "$target" --verbose
    else
        cmake --build "${BUILD_DIR}" --target "$target"
    fi

    print_success "Build complete for target: $target"
}

clean_build() {
    print_section "Cleaning Build Directory"

    if [ -d "${BUILD_DIR}" ]; then
        print_step "Removing: ${BUILD_DIR}"
        rm -rf "${BUILD_DIR}"
        print_success "Build directory cleaned"
    else
        print_warning "Build directory does not exist"
    fi
}

full_build() {
    local target="${1:-sdl3_app}"

    print_section "FULL BUILD PIPELINE"
    echo "Target: $target"
    echo "Build Type: ${BUILD_TYPE}"
    echo "Generator: ${GENERATOR}"
    echo "Project Root: ${PROJECT_ROOT}"

    verify_requirements
    install_conan_deps
    generate_cmakelists
    configure_cmake
    build_target "$target"

    print_section "BUILD COMPLETE"
    echo -e "${GREEN}Successfully built target: $target${NC}"

    # Show binary location if it's sdl3_app
    if [ "$target" = "sdl3_app" ]; then
        echo -e "${GREEN}Binary location: ${BUILD_DIR}/sdl3_app${NC}"
    else
        echo -e "${GREEN}Target built: $target${NC}"
    fi
}

# Main script logic
main() {
    case "${1:-}" in
        --help|-h)
            show_help
            exit 0
            ;;
        --list-targets)
            list_targets
            exit 0
            ;;
        --clean)
            clean_build
            CLEAN_BUILD=1
            # If no second argument, stop after clean
            if [ -z "${2:-}" ]; then
                exit 0
            fi
            # Otherwise, rebuild with second argument as target
            full_build "${2:-sdl3_app}"
            ;;
        --verbose|-v)
            VERBOSE=1
            full_build "${2:-sdl3_app}"
            ;;
        *)
            # Default: build specified target or sdl3_app
            full_build "${1:-sdl3_app}"
            ;;
    esac
}

# Run main
main "$@"
