# Vulkan Shader Linking Problem

## Problem Description

The project is experiencing failures when trying to create bgfx shader programs for Vulkan rendering of the GUI overlay system. Specifically:

- GUI overlay shaders (vertex and fragment) compile successfully to SPIR-V binaries
- Individual shaders are created successfully in bgfx
- However, `bgfx::createProgram()` fails to link the vertex and fragment shaders together
- This results in broken GUI overlay rendering (no 2D UI elements displayed)

The error manifests as: `"BgfxGuiService::CreateProgram: bgfx::createProgram failed to link shaders"`

**Note**: The GUI system is a separate 2D overlay library and does not affect 3D scene rendering (walls/ceiling/floor). The test comments suggesting otherwise appear to be incorrect.

## Current Status

- ✅ Shaders compile from GLSL to SPIR-V using shaderc library
- ✅ SPIR-V binaries include bgfx-compatible headers and uniform metadata
- ✅ `bgfx::createShader()` succeeds for both vertex and fragment shaders
- ❌ `bgfx::createProgram()` fails to link the shaders
- ❌ GUI overlay rendering is broken (no 2D UI elements)

**Clarification**: This affects only the 2D GUI overlay system, not 3D scene geometry rendering.

## Background: bgfx Architecture

bgfx is a cross-platform graphics library that abstracts different rendering APIs (OpenGL, Vulkan, DirectX, Metal). For Vulkan, bgfx:

1. **Shader Compilation**: Expects pre-compiled shader binaries in a custom format
2. **Binary Format**: SPIR-V data preceded by bgfx metadata (version, hash, uniform info)
3. **Program Linking**: Links vertex and fragment shaders based on uniform compatibility
4. **Vulkan Translation**: Handles Vulkan-specific details (descriptor sets, pipeline layouts)

### bgfx Shader Binary Format

bgfx shader binaries consist of:
- **Header**: 4-byte magic number, 4-byte hash, SPIR-V size
- **SPIR-V Data**: Compiled shader bytecode
- **Uniform Metadata**: Information about uniforms (names, types, registers)

For Vulkan, uniforms must be properly declared with binding locations.

## How the PipelineCompilerService Works

The `PipelineCompilerService` is our custom shader compilation service that replaces external shaderc executables.

### API Overview

```cpp
class PipelineCompilerService {
public:
    bool Compile(const std::string& inputPath,
                 const std::string& outputPath,
                 const std::vector<std::string>& args);
    std::optional<std::string> GetLastError() const;
};
```

### Compilation Process

1. **Input Reading**: Reads GLSL source from `inputPath`
2. **Argument Parsing**: Extracts shader type (`--type vertex/fragment`) and profile (`--profile spirv`)
3. **Shader Compilation**: Uses shaderc library to compile GLSL to SPIR-V
   - Sets Vulkan target environment: `shaderc_target_env_vulkan`
   - Generates SPIR-V 1.0 bytecode
4. **Binary Formatting**: Creates bgfx-compatible binary
   - Writes magic number (VSH/F SH + version)
   - Writes hash (currently 0)
   - Writes SPIR-V data
   - Writes uniform metadata
5. **Output Writing**: Saves binary to `outputPath`

### Uniform Metadata Format

For each uniform, writes:
- **numUniforms** (uint16_t): Number of uniforms
- **uniformName** (null-terminated string): Uniform name
- **uniformType** (uint8_t): Type (4=Mat4, 5=Sampler2D)
- **numElements** (uint8_t): Array size (1 for scalars)
- **regIndex** (uint16_t): Register/binding index
- **regCount** (uint16_t): Register count
- **textureInfo** (uint8_t x 3): Texture component, dimension, format

## How BgfxShaderCompiler Works

The `BgfxShaderCompiler` integrates with the `PipelineCompilerService` to provide shader compilation within the application.

### API Overview

```cpp
class BgfxShaderCompiler {
public:
    bgfx::ShaderHandle CompileShader(const std::string& label,
                                     const std::string& source,
                                     bool isVertex,
                                     const std::vector<BgfxShaderUniform>& uniforms,
                                     const std::vector<bgfx::Attrib::Enum>& attributes);
};
```

### Compilation Flow

1. **Renderer Detection**: Checks `bgfx::getRendererType()` to determine target API
2. **OpenGL Path**: For OpenGL, passes raw GLSL source to `bgfx::createShader()`
3. **Non-OpenGL Path**: 
   - Writes GLSL source to temp file
   - Calls `PipelineCompilerService::Compile()` with args like `{"--type", "vertex", "--profile", "spirv"}`
   - Reads compiled binary from temp file
   - Calls `bgfx::createShader()` with binary data
4. **Cleanup**: Removes temp files

### In-Memory Compilation Attempt

The compiler first tries to use dlsym to find shaderc functions in loaded libraries, but this fails because our shaderc is linked statically.

## How bgfx Program Creation Works

### API Overview

```cpp
bgfx::ProgramHandle bgfx::createProgram(bgfx::ShaderHandle vs, bgfx::ShaderHandle fs, bool destroyShaders = false);
```

### Linking Process

1. **Shader Validation**: Ensures both shaders are valid
2. **Uniform Matching**: Matches uniforms between vertex and fragment shaders
3. **Pipeline Creation**: For Vulkan, creates VkPipeline with appropriate descriptor sets
4. **Error Handling**: Returns invalid handle on failure

### Vulkan-Specific Requirements

For Vulkan linking to succeed:
- **Uniform Compatibility**: Uniforms must have matching names, types, and binding locations
- **Descriptor Sets**: Proper binding assignments for uniform buffers and samplers
- **SPIR-V Validity**: SPIR-V must be valid and contain required metadata

## Attempted Solutions

### 1. Initial Integration (Failed)
- Used bgfx_tools shaderc executable
- Failed due to missing dependencies and external process overhead

### 2. Conan shaderc Integration (Failed)
- Linked shaderc library statically
- Compilation failed due to missing bgfx 3rdparty headers

### 3. bgfx_tools Source Integration (Partial Success)
- Copied bgfx_tools shaderc sources
- Added bgfx_deps with required headers
- Shaders compile but linking fails

### 4. Shader Source Modifications
- Updated shaders to use Vulkan uniform blocks:
  ```glsl
  layout (binding = 0) uniform UniformBuffer {
      mat4 u_modelViewProj;
  };
  layout (binding = 1) uniform sampler2D s_tex;
  ```
- Changed from `#version 450` to Vulkan-compatible syntax

### 5. Compilation Target Adjustments
- Tried different shaderc target environments (OpenGL, Vulkan)
- Settled on Vulkan target for proper SPIR-V generation

### 6. Uniform Metadata Embedding
- Manually append uniform information to SPIR-V binaries
- Tried different uniform naming schemes (`u_modelViewProj` vs `UniformBuffer.u_modelViewProj`)

### 7. In-Memory Shaderc (Present but Not Wired)
- bgfx_tools provides `shaderc_compile_from_memory[_with_target]` in `src/bgfx_tools/shaderc/shaderc_mem.cpp`
- The compiler code only tries to `dlsym` these symbols, which fails when they are statically linked and not exported
- This forces the temp-file + `PipelineCompilerService` path

## Key Findings

### Successful Aspects
- **SPIR-V Generation**: shaderc successfully compiles Vulkan GLSL to SPIR-V
- **Binary Format**: bgfx accepts our custom binary format
- **Individual Shaders**: `bgfx::createShader()` works for both vertex and fragment

### Failure Points
- **Program Linking**: `bgfx::createProgram()` fails despite valid individual shaders
- **Binary Layout Mismatch**: The custom shader binary layout does not match bgfx shaderc's SPIR-V format
- **Uniform/Attribute Metadata**: Current metadata encoding (names, flags, reg counts, attr list, size footer) is incomplete or incompatible

### Technical Insights
- bgfx Vulkan renderer requires SPIR-V with embedded uniform metadata
- Uniform blocks in Vulkan need proper binding decoration in SPIR-V
- bgfx may expect specific uniform naming conventions for Vulkan

## In-Process Shaderc Integration (bgfx_tools)

bgfx already ships an in-process compiler wrapper for its shader binary format:

- `src/bgfx_tools/shaderc/shaderc_mem.h`
- `src/bgfx_tools/shaderc/shaderc_mem.cpp`

`BgfxShaderCompiler` currently tries to load these functions via `dlsym`, but the symbols are not exported because the code is statically linked into the executable. As a result, it falls back to the custom temp-file + `PipelineCompilerService` path, which writes a shader binary layout that does not match bgfx's SPIR-V format (uniform table, shader size, attribute list, size footer).

## Root Cause Hypothesis

The most likely cause is a shader binary format mismatch. The `PipelineCompilerService` writes a simplified header and uniform metadata that does not match the layout produced by bgfx's own `shaderc_spirv` pipeline (uniform table encoding, shader size, attribute list, and size footer). This causes `bgfx::createProgram()` to fail even though `bgfx::createShader()` succeeds.

## Next Steps

### Immediate Actions
1. **Wire In-Process Shaderc**: Call `shaderc_compile_from_memory[_with_target]` directly (no `dlsym`) or export the symbols from the main executable
2. **Remove Custom Binary Writer**: Stop writing manual uniform metadata once bgfx shaderc output is used
3. **Validate Binary Layout**: Compare the produced shader blob with a known bgfx SPIR-V blob (`src/bgfx_tools/texturev/*_spv`)
4. **Keep Vulkan Bindings**: Ensure GLSL bindings match bgfx uniform names (`u_modelViewProj`, `s_tex`)

### Potential Solutions
1. **Direct Call Path**: Replace `dlsym` with direct calls to `shaderc_mem` functions
2. **Symbol Export Path**: Add linker flags to export `shaderc_mem` symbols so `dlsym` works
3. **Fallback Cleanup**: Retain temp-file compile only for external shaderc usage, not for Vulkan GUI shaders

### Validation Steps
1. **Test with OpenGL**: Verify shaders work with OpenGL renderer
2. **Compare Binaries**: Diff working vs broken shader binaries
3. **Minimal Reproduction**: Create minimal test case for bgfx community

## Files Involved

- `src/services/impl/pipeline_compiler_service.cpp`: Shader compilation logic
- `src/services/impl/bgfx_shader_compiler.cpp`: bgfx integration
- `src/services/impl/bgfx_gui_service.cpp`: Shader sources and program creation (GUI overlay rendering)
- `src/bgfx_tools/shaderc/shaderc_mem.h`: In-process shaderc API
- `src/bgfx_tools/shaderc/shaderc_mem.cpp`: In-process shaderc implementation
- `src/bgfx_deps/`: Required headers for shaderc compilation
- `tests/test_vulkan_shader_linking.cpp`: Test case that validates GUI shader linking (note: test comments incorrectly suggest this affects 3D scene rendering)

## References

- [bgfx Documentation](https://bkaradzic.github.io/bgfx/)
- [shaderc API](https://github.com/google/shaderc)
- [SPIR-V Specification](https://www.khronos.org/registry/spir-v/)
- [Vulkan GLSL Specification](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#shaders)


# BGFX Vulkan Shader Crash Fix

## Problem Summary

The application was crashing with `SIGSEGV` in `bgfx::vk::ShaderVK::create()` during shader creation on the render thread.

**Root Cause:** In `shaderc_spirv.cpp` line 685-686, `GL_INT` uniform types were being incorrectly mapped to `UniformType::Sampler`, causing integer data uniforms (`u_numActiveLightSources`, `u_lightData.type`, `noise_octaves`) to be treated as texture samplers.

## The Bug

```cpp
// BEFORE (WRONG):
case 0x1404: // GL_INT:
    un.type = UniformType::Sampler;  // ❌ INCORRECT
    break;
```

**Why this crashes:**
1. Integer uniforms like `u_numActiveLightSources` get marked as samplers
2. Vulkan backend tries to create descriptor bindings for these "samplers"
3. With `regIndex=0` and malformed sampler metadata, the descriptor layout creation fails
4. Results in segmentation fault in `ShaderVK::create()`

## The Fix

```cpp
// AFTER (CORRECT):
case 0x1404: // GL_INT:
    // CRITICAL FIX: GL_INT should NOT be mapped to Sampler
    // Integer uniforms are data uniforms, not texture samplers
    // Map to Vec4 to maintain compatibility with bgfx uniform system
    un.type = UniformType::Vec4;  // ✅ CORRECT
    if (bgfx::g_verbose)
    {
        BX_TRACE("Uniform %s: GL_INT mapped to Vec4 (regIndex=%d, NOT a sampler)", 
            un.name.c_str(), un.regIndex);
    }
    break;
```

**Additional improvements:**
- Added trace logging to track GL_INT uniform handling
- Added logging for unknown uniform types in the default case

## Evidence from Log Analysis

Your `sdl3_app.log` showed:

- **Line 490-496:** `ceiling:fragment` shader had integer uniforms incorrectly tagged as `baseType=Sampler`
  - `u_numActiveLightSources` (int) → marked as Sampler ❌
  - `u_lightData.type` (int) → marked as Sampler ❌
  - `noise_octaves` (int) → marked as Sampler ❌
  - One with `regIndex=0` which triggers the crash

- **Line 1041:** SIGSEGV immediately after shader creation, confirming the malformed sampler metadata theory

## How to Apply the Fix

### Option 1: Apply the Patch File
```bash
cd /path/to/your/project/src
patch -p0 < shaderc_spirv.patch
```

### Option 2: Manual Edit
Edit `shaderc_spirv.cpp` around line 685:
1. Change `un.type = UniformType::Sampler;` to `un.type = UniformType::Vec4;`
2. Add the trace logging (optional but recommended)

### Option 3: Replace the Entire File
Use the fixed version: `shaderc_spirv_fixed.cpp`

## Rebuild Instructions

After applying the fix:

```bash
# Rebuild the shader compiler
python scripts/dev_commands.py build --target shaderc

# Or full rebuild
python scripts/dev_commands.py build

# Run with verbose logging to see the new traces
python scripts/dev_commands.py run -- --json-file-in config/your_config.json
```

## Expected Outcome

After the fix:
- Integer uniforms will be correctly classified as `Vec4` data uniforms
- No more malformed sampler bindings in Vulkan
- The SIGSEGV crash in `ShaderVK::create()` will be resolved
- With verbose logging enabled, you'll see: `"Uniform u_numActiveLightSources: GL_INT mapped to Vec4 (regIndex=X, NOT a sampler)"`

## Additional Notes

**Why Map to Vec4?**
- BGFX's uniform system expects data uniforms to be Vec4-compatible
- This maintains compatibility while preventing sampler misclassification
- Integer uniforms in shaders are typically used for counts, flags, or indices

**Actual Texture Samplers:**
- Real texture samplers are handled separately in lines 767-808
- They use `resourcesrefl.separate_images` from SPIRV-Cross reflection
- They correctly set `UniformType::Sampler | kUniformSamplerBit`

## Testing Recommendations

1. **Run with verbose logging first:**
   ```bash
   BGFX_VERBOSE=1 python scripts/dev_commands.py run
   ```

2. **Check that integer uniforms are now Vec4:**
   - Look for the new trace messages in the log
   - Verify no more "invalid sampler binding" errors

3. **Test all shader variants:**
   - Test with MaterialX-generated shaders
   - Test GUI shaders
   - Test ceiling/lighting shaders

4. **Verify no regression:**
   - Actual texture samplers should still work correctly
   - Check that textures are still rendering properly

## Files Modified

- `shaderc_spirv.cpp` (lines 685-705)

## Related Issues

This fix addresses:
- SIGSEGV in `bgfx::vk::ShaderVK::create()`
- "No valid uniforms" warnings for shaders with integer uniforms
- Malformed descriptor layouts in Vulkan backend
- Use-after-free-like symptoms (though actual cause was malformed metadata)
