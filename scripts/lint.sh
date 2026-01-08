#!/bin/bash
set -e

echo "=== Multi-Layer C++ Linting Suite ==="
echo ""

BUILD_DIR="${1:-build-ninja}"
SRC_DIRS="src/"
ERRORS_FOUND=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    local status=$1
    local message=$2
    if [ "$status" = "OK" ]; then
        echo -e "${GREEN}✓${NC} $message"
    elif [ "$status" = "WARN" ]; then
        echo -e "${YELLOW}⚠${NC} $message"
    else
        echo -e "${RED}✗${NC} $message"
        ERRORS_FOUND=1
    fi
}

# 1. Clang-Tidy (Static Analysis)
echo "=== Layer 1: Clang-Tidy Static Analysis ==="
if command -v clang-tidy &> /dev/null; then
    if [ -f "$BUILD_DIR/compile_commands.json" ]; then
        echo "Running clang-tidy on modified files..."
        
        # Get list of source files
        FILES=$(find src/ -name "*.cpp" -o -name "*.hpp" | grep -v "bgfx_deps\|bgfx_docs_examples" || true)
        
        TIDY_ERRORS=0
        for file in $FILES; do
            if ! clang-tidy -p "$BUILD_DIR" "$file" 2>&1 | tee /tmp/clang-tidy-$$.txt | grep -q "error:"; then
                :
            else
                TIDY_ERRORS=$((TIDY_ERRORS + 1))
                echo "Errors in $file"
            fi
        done
        
        if [ $TIDY_ERRORS -eq 0 ]; then
            print_status "OK" "Clang-Tidy: No critical issues"
        else
            print_status "ERROR" "Clang-Tidy: $TIDY_ERRORS files with errors"
        fi
    else
        print_status "WARN" "Clang-Tidy: compile_commands.json not found (run cmake with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON)"
    fi
else
    print_status "WARN" "Clang-Tidy: not installed (install with: sudo dnf install clang-tools-extra)"
fi

# 2. Cppcheck (Static Analysis)
echo ""
echo "=== Layer 2: Cppcheck Static Analysis ==="
if command -v cppcheck &> /dev/null; then
    echo "Running cppcheck..."
    if cppcheck --enable=all --inconclusive --suppress=missingIncludeSystem \
        --suppress=unmatchedSuppression --suppress=unusedFunction \
        --error-exitcode=1 --inline-suppr \
        -I src/ src/ 2>&1 | tee /tmp/cppcheck-$$.txt | grep -E "error:|warning:" > /tmp/cppcheck-errors-$$.txt; then
        
        ERROR_COUNT=$(grep -c "error:" /tmp/cppcheck-errors-$$.txt || echo 0)
        WARN_COUNT=$(grep -c "warning:" /tmp/cppcheck-errors-$$.txt || echo 0)
        
        if [ $ERROR_COUNT -eq 0 ]; then
            print_status "OK" "Cppcheck: No errors ($WARN_COUNT warnings)"
        else
            print_status "ERROR" "Cppcheck: $ERROR_COUNT errors, $WARN_COUNT warnings"
        fi
    else
        print_status "OK" "Cppcheck: Clean"
    fi
else
    print_status "WARN" "Cppcheck: not installed (install with: sudo dnf install cppcheck)"
fi

# 3. Compiler Warnings (Maximum strictness)
echo ""
echo "=== Layer 3: Compiler Warning Check ==="
echo "Rebuilding with maximum warnings enabled..."

if [ ! -d "$BUILD_DIR-lint" ]; then
    cmake -B "$BUILD_DIR-lint" -G Ninja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Werror -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wconversion -Wsign-conversion -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wnull-dereference -Wuseless-cast -Wdouble-promotion -Wformat=2" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi

if cmake --build "$BUILD_DIR-lint" --target sdl3_app 2>&1 | tee /tmp/compile-warnings-$$.txt | grep -E "error:|warning:"; then
    print_status "ERROR" "Compiler: Warnings/Errors found"
else
    print_status "OK" "Compiler: Clean build with strict warnings"
fi

# 4. Include-what-you-use (Optional but recommended)
echo ""
echo "=== Layer 4: Include What You Use ==="
if command -v include-what-you-use &> /dev/null; then
    echo "Running include-what-you-use..."
    # This requires special cmake configuration
    print_status "WARN" "IWYU: Requires manual cmake configuration"
else
    print_status "WARN" "IWYU: not installed (install from: https://include-what-you-use.org/)"
fi

# 5. Sanitizer Builds
echo ""
echo "=== Layer 5: Sanitizer Build Check ==="
echo "Building with AddressSanitizer + UndefinedBehaviorSanitizer..."

if [ ! -d "$BUILD_DIR-asan" ]; then
    cmake -B "$BUILD_DIR-asan" -G Ninja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" \
        -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
fi

if cmake --build "$BUILD_DIR-asan" --target sdl3_app 2>&1 | tee /tmp/asan-build-$$.txt; then
    print_status "OK" "Sanitizer build succeeded (run with: $BUILD_DIR-asan/sdl3_app)"
else
    print_status "ERROR" "Sanitizer build failed"
fi

# Summary
echo ""
echo "=== Linting Summary ==="
if [ $ERRORS_FOUND -eq 0 ]; then
    echo -e "${GREEN}All checks passed!${NC}"
    exit 0
else
    echo -e "${RED}Some checks failed. Review output above.${NC}"
    exit 1
fi
