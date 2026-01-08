# BGFX Vulkan Shader Crash Fix

## Problem Summary

The application was crashing with `SIGSEGV` in `bgfx::vk::ShaderVK::create()` during shader creation on the render thread.

**Root Cause:** In `src/bgfx_tools/shaderc/shaderc_spirv.cpp`, integer uniform types were mapped to `UniformType::Sampler`, which caused data uniforms (like `u_numActiveLightSources`, `u_lightData.type`, `noise_octaves`) to be treated as samplers. Vulkan then attempted to create sampler bindings for non-sampler data, leading to invalid descriptor layout data and a crash inside `ShaderVK::create()`.

## The Bug

```cpp
// BEFORE (WRONG):
case 0x1404: // GL_INT:
    un.type = UniformType::Sampler;  // ❌ INCORRECT
    break;
```

## The Fix

```cpp
// AFTER (CORRECT):
const uint32_t uniformType = program->getUniformType(ii);

switch (uniformType)
{
case 0x1404: // GL_INT:
case 0x8B53: // GL_INT_VEC2:
case 0x8B54: // GL_INT_VEC3:
case 0x8B55: // GL_INT_VEC4:
    // Integer uniforms are data uniforms, not texture samplers.
    // Map to Vec4 to match bgfx's data-uniform layout rules.
    un.type = UniformType::Vec4;  // ✅ CORRECT
    if (bgfx::g_verbose)
    {
        BX_TRACE("shaderc_spirv: int uniform mapped to Vec4 (%s, glType=0x%X, offset=%u)",
            un.name.c_str(),
            uniformType,
            offset);
    }
    break;
...
default:
    if (bgfx::g_verbose)
    {
        BX_TRACE("shaderc_spirv: skipping uniform %s (glType=0x%X)",
            un.name.c_str(),
            uniformType);
    }
    continue;
}
```

## Evidence From Log Analysis

From `sdl3_app.log` before the fix:

- `ceiling:fragment` uniforms showed data ints incorrectly tagged as `Sampler`
  - `u_numActiveLightSources` (int) → `Sampler` ❌
  - `u_lightData.type` (int) → `Sampler` ❌
  - `noise_octaves` (int) → `Sampler` ❌
- At least one of these had `regIndex=0`, which is invalid for sampler bindings in Vulkan.
- The crash followed immediately after shader creation, inside `bgfx::vk::ShaderVK::create()`.

## Memory Lifetime Note

The shader creation path already uses `bgfx::copy(...)` before `bgfx::createShader(...)`, so the crash is not explained by `bgfx::makeRef()` use-after-free in the current code path.

## How To Validate

1. Rebuild:
   ```bash
   ./scripts/dev_commands.py build --build-dir build-ninja --target sdl3_app
   ```
2. Run with trace logs and verify:
   - With verbose enabled, `shaderc_spirv: int uniform mapped to Vec4 ...` appears for the int uniforms
   - The shader uniform list no longer shows those ints as samplers
   - The Vulkan SIGSEGV no longer occurs

## Files Modified

- `src/bgfx_tools/shaderc/shaderc_spirv.cpp`

## Follow-Ups

- GUI fragment shader uniforms may still appear missing if the reflection path does not include combined image samplers; that is a separate issue from this crash.

--- /mnt/user-data/uploads/shaderc_spirv.cpp	2026-01-07 21:56:15.260349800 +0000
+++ /home/claude/shaderc_spirv_fixed.cpp	2026-01-07 21:57:01.341824916 +0000
@@ -683,7 +683,15 @@
 						switch (program->getUniformType(ii) )
 						{
 						case 0x1404: // GL_INT:
-							un.type = UniformType::Sampler;
+							// CRITICAL FIX: GL_INT should NOT be mapped to Sampler
+							// Integer uniforms are data uniforms, not texture samplers
+							// Map to Vec4 to maintain compatibility with bgfx uniform system
+							un.type = UniformType::Vec4;
+							if (bgfx::g_verbose)
+							{
+								BX_TRACE("Uniform %s: GL_INT mapped to Vec4 (regIndex=%d, NOT a sampler)", 
+									un.name.c_str(), un.regIndex);
+							}
 							break;
 
 						case 0x8B52: // GL_FLOAT_VEC4:
@@ -701,6 +709,11 @@
 							break;
 
 						default:
+							if (bgfx::g_verbose)
+							{
+								BX_TRACE("Uniform %s: unknown type 0x%04X, skipping", 
+									un.name.c_str(), program->getUniformType(ii));
+							}
 							continue;
 						}
 

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
