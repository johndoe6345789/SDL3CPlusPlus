# Mega-Strict Shader Pipeline Validation

## Overview

This system provides comprehensive validation of shader pipelines BEFORE they reach the GPU driver, preventing system crashes from malformed shader state.

## Architecture

### 1. Runtime C++ Validator (`shader_pipeline_validator.hpp/cpp`)

**Location:** `src/services/impl/shader_pipeline_validator.*`

**Purpose:** Validates shader pipelines at runtime before compilation

**Checks Performed:**
- ✅ Vertex layout matches shader input expectations
- ✅ Vertex struct size matches layout stride
- ✅ Vertex shader outputs match fragment shader inputs
- ✅ SPIR-V requirements (all inputs/outputs have `layout(location=N)`)
- ✅ Type compatibility between GLSL and vertex attributes
- ✅ Location continuity and gaps

**Integration Point:** `MaterialXShaderGenerator::Generate()` - validates every MaterialX shader before returning

**Failure Mode:** **Throws exception** if validation fails, preventing GPU submission

### 2. Static Clang-Tidy Configuration (`.clang-tidy`)

**Location:** `./.clang-tidy`

**Purpose:** Static analysis during development

**Enabled Checks:**
- `bugprone-*` - Bug-prone code patterns
- `cert-*` - CERT secure coding guidelines
- `clang-analyzer-*` - Deep static analysis
- `cppcoreguidelines-*` - C++ Core Guidelines
- `performance-*` - Performance issues
- `misc-*`, `modernize-*`, `portability-*`, `readability-*`

**Critical Errors:** Bug-prone and CERT violations are treated as errors

### 3. Multi-Layer Linting Script (`scripts/lint.sh`)

**Layers:**
1. **Clang-Tidy** - Static analysis with compile_commands.json
2. **Cppcheck** - Independent static analyzer
3. **Compiler Warnings** - Maximum strictness build (`-Wall -Wextra -Wpedantic -Werror`)
4. **Sanitizer Build** - AddressSanitizer + UndefinedBehaviorSanitizer

**Usage:**
```bash
./scripts/lint.sh [build-dir]
```

### 4. Python Shader Validator (`scripts/validate_shaders.py`)

**Purpose:** Standalone shader validation tool

**Features:**
- Extracts shader attributes from GLSL source
- Validates vertex layout compatibility
- Checks SPIR-V requirements
- Validates inter-stage matching
- Naming convention validation

**Usage:**
```bash
python3 scripts/validate_shaders.py [--verbose]
```

## Validation Rules

### Vertex Attribute Mapping

**bgfx Vertex Layout Order (MUST MATCH):**
```cpp
location 0: Position  (vec3, 12 bytes)
location 1: Normal    (vec3, 12 bytes)
location 2: Tangent   (vec3, 12 bytes)
location 3: TexCoord0 (vec2,  8 bytes)
location 4: Color0    (vec3, 12 bytes)
Total:                       56 bytes
```

**Vertex Struct (`core::Vertex`):**
```cpp
struct Vertex {
    std::array<float, 3> position;  // offset  0, 12 bytes
    std::array<float, 3> normal;    // offset 12, 12 bytes
    std::array<float, 3> tangent;   // offset 24, 12 bytes
    std::array<float, 2> texcoord;  // offset 36,  8 bytes
    std::array<float, 3> color;     // offset 44, 12 bytes
};  // Total: 56 bytes
```

**GLSL Shader Inputs (MaterialX):**
```glsl
layout (location = 0) in vec3 i_position;
layout (location = 1) in vec3 i_normal;
layout (location = 2) in vec3 i_tangent;
layout (location = 3) in vec2 i_texcoord_0;
// location 4 (color) optional - not used by MaterialX
```

### Critical Validations

1. **Location Match**
   - Every shader input MUST have corresponding vertex layout attribute at same location
   - **Violation = CRASH**

2. **Type Compatibility**
   - GLSL `vec3` ↔ C++ `std::array<float, 3>`
   - GLSL `vec2` ↔ C++ `std::array<float, 2>`
   - **Violation = CRASH or garbage data**

3. **Stride Match**
   - `sizeof(core::Vertex)` MUST equal sum of layout attribute sizes
   - **Violation = Memory corruption, reading wrong vertices**

4. **Interface Matching**
   - Vertex shader outputs MUST match fragment shader inputs (by location & type)
   - **Violation = Undefined behavior, may crash**

5. **SPIR-V Requirements**
   - All `in`/`out` variables MUST have `layout(location=N)`
   - **Violation = Compilation failure**

## How It Prevented System Crash

### The Bug

**Before Validation:**
1. MaterialX generated shaders with locations: `[0, 1, 2, 3]`
2. But actual order was `[position=0, texcoord=1, normal=2, tangent=3]`
3. bgfx vertex layout expected: `[position=0, normal=1, tangent=2, texcoord=3]`
4. **Mismatch → Vulkan driver received malformed pipeline state**
5. **AMD Radeon driver bug → System crash (not just app crash!)**

**After Validation:**
- Validator detects location mismatch
- Throws exception with detailed error message
- **Prevents submission to GPU → No driver crash**

### Fix Applied

```cpp
// MaterialX shader generator now remaps locations to match bgfx:
std::map<std::string, int> bgfxLocationMap;
bgfxLocationMap["i_position"] = 0;    // MUST be 0
bgfxLocationMap["i_normal"] = 1;      // MUST be 1
bgfxLocationMap["i_tangent"] = 2;     // MUST be 2
bgfxLocationMap["i_texcoord_0"] = 3;  // MUST be 3
```

## Integration Points

### MaterialX Pipeline

```cpp
// In MaterialXShaderGenerator::Generate()
ShaderPipelineValidator validator(logger_);

// Define expected layout
std::vector<AttributeInfo> expectedLayout = { /* ... */ };

// Validate BEFORE returning shaders
auto result = validator.ValidatePipeline(
    vertexSource, fragmentSource, expectedLayout,
    sizeof(core::Vertex), pipelineName
);

if (!result.passed) {
    throw std::runtime_error("Validation failed");  // PREVENTS CRASH
}
```

### Pre-Commit Hook (Recommended)

```bash
# .git/hooks/pre-commit
#!/bin/bash
./scripts/lint.sh build-ninja || {
    echo "❌ Linting failed! Fix errors before committing."
    exit 1
}
```

## Future Enhancements

1. **Compile-Time Validation**
   - Use `static_assert` to validate vertex struct layout at compile time
   - Template metaprogramming for layout/struct correspondence

2. **Shader Hot-Reload Validation**
   - Re-validate on shader modification
   - Runtime checks in debug builds

3. **GPU Capability Checks**
   - Validate against actual GPU limits
   - Query max vertex attributes, texture units, etc.

4. **Vulkan Validation Layers**
   - Enable comprehensive Vulkan validation in debug builds
   - Parse validation layer output for early warnings

## Debugging

### Enable Verbose Validation

```cpp
// In shader_pipeline_validator.cpp, re-enable trace logging
logger_->Info("Attribute at location " + std::to_string(loc) + ": " + name);
```

### Manual Validation

```python
# Run standalone validator
python3 scripts/validate_shaders.py --verbose

# Check specific shader
grep -A20 "RAW_VERTEX_SHADER" sdl3_app.log
```

### Sanitizer Run

```bash
# Build with sanitizers
cmake -B build-asan -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"

# Run and check for issues
./build-asan/sdl3_app -j config/seed_runtime.json
```

## Summary

**The mega-strict validation system prevents GPU driver crashes by:**

1. ✅ Validating shader/vertex layout compatibility BEFORE GPU submission
2. ✅ Enforcing strict type and location matching
3. ✅ Catching SPIR-V requirement violations early
4. ✅ Providing detailed error messages for debugging
5. ✅ **THROWING EXCEPTIONS on validation failure (fail-safe)**

**Result:** No more system crashes from shader pipeline bugs! 🎉
