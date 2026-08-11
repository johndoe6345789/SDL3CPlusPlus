.PHONY: help build clean test configure install-deps list-targets rebuild verbose

# Configuration
PROJECT_ROOT := $(CURDIR)
BUILD_DIR := $(PROJECT_ROOT)/build/Release
BUILD_TYPE := Release
GENERATOR := Ninja
PYTHON := python3

# Default target
.DEFAULT_GOAL := help

help:
	@echo "GameEngine Build Automation"
	@echo ""
	@echo "Usage:"
	@echo "  make build              - Build default target (sdl3_app)"
	@echo "  make build TARGET=<name> - Build specific target"
	@echo "  make rebuild            - Clean and rebuild from scratch"
	@echo "  make test               - Build and run tests"
	@echo "  make test-<name>        - Build specific test target"
	@echo "  make clean              - Clean build directory"
	@echo "  make configure          - Run CMake configuration only"
	@echo "  make install-deps       - Install Conan dependencies only"
	@echo "  make list-targets       - List all available targets"
	@echo "  make verify             - Verify build requirements"
	@echo "  make verbose            - Build with verbose output"
	@echo ""
	@echo "Examples:"
	@echo "  make build TARGET=sdl3_app"
	@echo "  make build TARGET=test_bootstrap_orchestration"
	@echo "  make test"
	@echo ""

build: verify install-deps configure
	@echo "Building target: $(TARGET)"
	@cmake --build $(BUILD_DIR) --target $(TARGET)
	@echo "✓ Build complete"

rebuild: clean build

test: build-tests run-tests

build-tests: verify install-deps configure
	@echo "Building all tests..."
	@cmake --build $(BUILD_DIR) --target all

run-tests:
	@echo "Running tests..."
	@cd $(BUILD_DIR) && ctest --verbose

test-%: verify install-deps configure
	@echo "Building test: test_$*"
	@cmake --build $(BUILD_DIR) --target test_$*

clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)
	@echo "✓ Clean complete"

configure: install-deps generate-cmake
	@echo "Configuring CMake..."
	@mkdir -p $(BUILD_DIR)
	@cd $(PROJECT_ROOT) && \
	cmake \
		-B $(BUILD_DIR) \
		-S $(PROJECT_ROOT) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_TOOLCHAIN_FILE=$(BUILD_DIR)/generators/conan_toolchain.cmake \
		-G $(GENERATOR) \
		-DBUILD_SDL3_APP=ON \
		-DSDL_VERSION=SDL3
	@echo "✓ CMake configuration complete"

install-deps:
	@echo "Installing Conan dependencies..."
	@cd $(PROJECT_ROOT) && conan install . --build=missing
	@echo "✓ Conan dependencies installed"

generate-cmake:
	@echo "Generating CMakeLists.txt from JSON..."
	@cd $(PROJECT_ROOT) && $(PYTHON) generate_cmake.py --config cmake_config.json --template CMakeLists.txt.jinja2 --output CMakeLists.txt
	@echo "✓ CMakeLists.txt generated"

list-targets:
	@echo "Available targets:"
	@grep -E "^add_executable|^add_test" $(PROJECT_ROOT)/CMakeLists.txt 2>/dev/null | \
		sed 's/add_executable(\([^ ]*\).*/  - \1 (executable)/' | \
		sed 's/add_test(\([^ ]*\).*/  - \1 (test)/' | sort | uniq || \
		echo "  (CMakeLists.txt not found - run 'make configure' first)"

verify:
	@echo "Verifying build requirements..."
	@command -v conan > /dev/null || (echo "ERROR: conan not found"; exit 1)
	@command -v cmake > /dev/null || (echo "ERROR: cmake not found"; exit 1)
	@command -v ninja > /dev/null || (echo "ERROR: ninja not found"; exit 1)
	@command -v $(PYTHON) > /dev/null || (echo "ERROR: python3 not found"; exit 1)
	@$(PYTHON) -c "import jinja2" || (echo "ERROR: jinja2 not installed"; exit 1)
	@echo "✓ All requirements verified"

verbose: verify install-deps configure
	@echo "Building with verbose output..."
	@cmake --build $(BUILD_DIR) --target $(TARGET) --verbose

# Phony targets that might have a corresponding directory
.PHONY: build clean test
