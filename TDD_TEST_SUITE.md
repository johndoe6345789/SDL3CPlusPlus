# TDD Test Suite for Documented Problems

## Overview

This document summarizes the TDD (Test-Driven Development) test suites created based on problems documented in the markdown files. These tests serve as living documentation and will help ensure fixes are properly implemented.

## Test Suites Created

### 1. Buffer Overflow Validation Tests
**File**: `tests/bgfx_draw_bounds_validation_test.cpp`  
**Status**: ✅ 8/8 tests passing (documenting missing validations)  
**Purpose**: Documents the buffer overflow issue that caused the sdl3_app.log crash

#### Problem Documented
From the crash analysis:
- Last draw command: `vertexOffset=1170, indexOffset=5232, indexCount=72, totalVertices=1194`
- Only 24 vertices available after offset (1194 - 1170 = 24)
- Drawing 72 indices with no validation if they reference vertices beyond buffer bounds
- Crash in AMD Vulkan driver during `bgfx::vk::RendererContextVK::getPipeline()`

#### Tests Cover
1. **CrashScenario_IndexOutOfBounds** - Exact scenario from crash log
2. **IndexReferencesWithinVertexBufferBounds** - Multiple draw calls validation
3. **VertexOffsetWithinBounds** - Offset must not exceed buffer size
4. **IndexCountWithinBufferBounds** - Index buffer bounds checking
5. **EmptyDraw_ZeroIndices** - Edge case: zero index count
6. **NegativeVertexOffset** - Invalid negative offset detection
7. **RequiredValidations** - Documents 6 missing validation checks
8. **CombinedMeshBuffer_MultipleDraws** - Real-world multi-mesh scenario

#### Missing Validations Identified
In `BgfxGraphicsBackend::Draw()` (lines ~1093-1145):
1. Check `vertexOffset >= 0`
2. Check `vertexOffset < totalVertices`
3. Check `indexOffset + indexCount <= indexBufferSize`
4. Check all indices < totalVertices (requires reading index buffer)
5. Log detailed error on validation failure
6. Return early instead of calling `bgfx::submit()` with invalid parameters

### 2. GUI Shader Linking Failure Tests
**File**: `tests/gui_shader_linking_failure_test.cpp`  
**Status**: ✅ 12/12 tests passing (documenting the bug and fix)  
**Purpose**: Documents the GL_INT→Sampler mapping bug in shaderc_spirv.cpp

#### Problem Documented
From VULKAN_SHADER_LINKING_PROBLEM.md:
- Integer uniforms incorrectly mapped to `UniformType::Sampler`
- Should be mapped to `UniformType::Vec4` (data uniform, not texture sampler)
- Caused GUI shader linking to fail in Vulkan backend
- Error: "BgfxGuiService::CreateProgram: bgfx::createProgram failed to link shaders"

#### Tests Cover
1. **LinkFailure_IntUniformsAsSamplers** - Documents the incorrect mapping
2. **IntVectorTypes_AlsoMappedIncorrectly** - All int vector types affected
3. **ActualSamplers_ShouldBeMappedCorrectly** - Distinguishes real samplers
4. **GuiShader_AffectedUniforms** - Specific uniforms that caused crash
5. **CrashLocation_VulkanShaderCreation** - Stack trace location
6. **TheFix_MapIntToVec4** - Documents the correct fix
7. **AfterFix_ShadersShouldLink** - Expected behavior after fix
8. **MixedUniforms_DataAndSamplers** - Mixed uniform types handling
9. **NotMemoryLifetimeIssue** - Rules out alternative causes
10. **ValidationSteps** - How to verify the fix works
11. **BugLocation** - Exact file and line number
12. **Impact_GuiNotRendering** - Severity assessment

#### The Fix Required
In `src/bgfx_tools/shaderc/shaderc_spirv.cpp` line ~685:

**BEFORE (WRONG)**:
```cpp
case 0x1404: // GL_INT:
    un.type = UniformType::Sampler;  // ❌ INCORRECT
    break;
```

**AFTER (CORRECT)**:
```cpp
case 0x1404: // GL_INT:
case 0x8B53: // GL_INT_VEC2:
case 0x8B54: // GL_INT_VEC3:
case 0x8B55: // GL_INT_VEC4:
    un.type = UniformType::Vec4;  // ✅ CORRECT
    break;
```

## How to Use These Tests

### Build Tests
```bash
cd /home/rewrich/Documents/GitHub/SDL3CPlusPlus
cmake --build build-ninja --target bgfx_draw_bounds_validation_test gui_shader_linking_failure_test
```

### Run Tests
```bash
cd build-ninja
./bgfx_draw_bounds_validation_test
./gui_shader_linking_failure_test
```

### Expected Results
- **Currently**: All tests PASS (documenting issues, not yet validating fixes)
- **After implementing fixes**: Tests should be updated to actually validate the fixes work

## TDD Workflow

### For Buffer Overflow Fix:

1. **Current State**: Tests document missing validation
2. **Implement Fix**: Add bounds checking in `BgfxGraphicsBackend::Draw()`
3. **Update Tests**: Add tests that actually call Draw with invalid parameters
4. **Verify**: Tests should detect and reject invalid draw calls

### For Shader Linking Fix:

1. **Current State**: Tests document the incorrect mapping
2. **Implement Fix**: Update shaderc_spirv.cpp to map GL_INT to Vec4
3. **Update Tests**: Add tests that verify uniform reflection results
4. **Verify**: GUI shaders should link successfully, GUI should render

## Additional Test Suites Already Present

From the existing test infrastructure:

1. **shader_pipeline_validator_test.cpp** - 22 tests ✅
2. **materialx_shader_generator_integration_test.cpp** - 5 tests ✅
3. **bgfx_texture_loading_test.cpp** - 7 tests ✅
4. **bgfx_initialization_order_test.cpp** - 12 tests ✅
5. **bgfx_frame_requirement_test.cpp** - Tests actual bgfx behavior ✅

**Total Test Coverage**: 66 tests documenting and validating the rendering system

## Next Steps

### Priority 1: Fix Buffer Overflow (CRITICAL)
- Add bounds validation to `BgfxGraphicsBackend::Draw()`
- Prevents GPU driver crashes
- Update tests to verify validation works

### Priority 2: Fix GUI Shader Linking (HIGH)
- Update `shaderc_spirv.cpp` to map GL_INT correctly
- Enables GUI rendering
- Verify with GUI tests

### Priority 3: Monitor and Validate
- Run tests after each fix
- Ensure no regressions
- Add more integration tests as needed

## References

### Documentation
- [CRASH_ANALYSIS.md](../CRASH_ANALYSIS.md) - Original crash investigation
- [VULKAN_SHADER_LINKING_PROBLEM.md](../VULKAN_SHADER_LINKING_PROBLEM.md) - Shader linking bug analysis
- [FIXES_IMPLEMENTED.md](../FIXES_IMPLEMENTED.md) - Summary of implemented fixes
- [INITIALIZATION_ORDER_BUG.md](../INITIALIZATION_ORDER_BUG.md) - bgfx init order issue

### Test Files
- `tests/bgfx_draw_bounds_validation_test.cpp` - Buffer overflow tests
- `tests/gui_shader_linking_failure_test.cpp` - Shader linking tests

### Source Files Needing Fixes
- `src/services/impl/bgfx_graphics_backend.cpp:1093-1145` - Add Draw validation
- `src/bgfx_tools/shaderc/shaderc_spirv.cpp:~685` - Fix GL_INT mapping

---

**Test Philosophy**: These tests serve as executable documentation. They pass not because the system is correct, but because they document problems that exist. When fixes are implemented, the tests should be enhanced to actually validate the fixes work correctly.
