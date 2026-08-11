# SDL3 & bgfx Graphics Examples

Graphics rendering examples demonstrating both pure SDL3 and bgfx approaches.

## Examples

### ✅ sdl3_cubes.cpp - WORKING!
- **121 rotating colored cubes in 11x11 grid**
- ~240 lines of pure SDL3 code
- Uses SDL_RenderGeometry for actual GPU rendering
- **Status**: ✅ Fully functional on macOS M4 with Metal

### ✅ standalone_cubes.cpp - NOW WORKING!
- **121 rotating cubes using bgfx + Metal**
- ~280 lines of code (with shader loading)
- **Status**: ✅ Fully functional on macOS M4 with Metal backend!
- Demonstrates complete bgfx + SDL3 + Metal integration

### simple_triangle.cpp
- Basic colored triangle with bgfx
- ~150 lines of code
- **Status**: Needs same fixes as standalone_cubes

## Building

```bash
cmake -B build
cmake --build build
```

## Running

```bash
cd build
./sdl3_cubes        # ✅ Pure SDL3 with Metal rendering
./standalone_cubes  # ✅ bgfx + Metal with pre-compiled shaders
./simple_triangle   # ⚠️  Needs renderFrame() fix
```

## Working Demos

### sdl3_cubes.cpp - Pure SDL3 Approach

**Pure SDL3 approach** - no bgfx dependency, direct GPU rendering:

- **SDL_RenderGeometry** for drawing 3D vertices
- **Manual 3D transformations** (rotation matrices, projection)
- **Per-face colors** with depth sorting
- **60 FPS** with vsync
- **M4 Metal backend** (automatic via SDL3)

**Key advantages**:
- No shader compilation needed
- No code signing/entitlements required
- Works out of the box on macOS
- Simpler dependency chain (SDL3 only)

### standalone_cubes.cpp - bgfx + Metal Approach ✅

**bgfx approach** - full control over GPU rendering pipeline:

- **bgfx Metal backend** with code signing + entitlements
- **Pre-compiled Metal shaders** (vs_cubes.bin, fs_cubes.bin)
- **Single-threaded mode** via `bgfx::renderFrame()` before init
- **Full GPU acceleration** with vertex/index buffers
- **60 FPS** with vsync and depth testing

**Key requirements**:
- Code signing with Metal entitlements
- `bgfx::renderFrame()` BEFORE `bgfx::init()` (critical!)
- Pre-compiled .bin shaders for Metal
- Platform data set in TWO places

## BREAKTHROUGH: bgfx + Metal Working!

### The Solution - 3 Critical Steps

After inference from SDL3's success, we discovered the complete solution:

#### 1. Code Signing with Metal Entitlements

```bash
codesign --force --sign - --entitlements metal.entitlements ./standalone_cubes
```

**metal.entitlements**:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "...">
<plist version="1.0">
<dict>
    <key>com.apple.security.cs.allow-unsigned-executable-memory</key>
    <true/>
    <key>com.apple.security.cs.allow-jit</key>
    <true/>
    <key>com.apple.security.cs.disable-library-validation</key>
    <true/>
    <key>com.apple.security.cs.debugger</key>
    <true/>
</dict>
</plist>
```

#### 2. Single-Threaded Mode (THE CRITICAL FIX!)

**Must call `bgfx::renderFrame()` BEFORE `bgfx::init()`:**

```cpp
// Setup platform data
bgfx::setPlatformData(pd);

// CRITICAL: Signal single-threaded mode!
bgfx::renderFrame();  // <-- This fixed the hang!

// Now initialize
bgfx::Init init;
init.platformData = pd;  // Also set here!
init.type = bgfx::RendererType::Metal;
bgfx::init(init);
```

**Without this**, bgfx hangs indefinitely waiting for a render thread that never starts.

#### 3. Pre-Compiled Metal Shaders

Copy Metal .bin shaders to build directory:
```bash
cp vs_cubes.bin fs_cubes.bin build/
```

Load shaders:
```cpp
const bgfx::Memory* vsMem = loadShader("vs_cubes.bin");
const bgfx::Memory* fsMem = loadShader("fs_cubes.bin");
bgfx::ShaderHandle vsh = bgfx::createShader(vsMem);
bgfx::ShaderHandle fsh = bgfx::createShader(fsMem);
bgfx::ProgramHandle program = bgfx::createProgram(vsh, fsh, true);
```

### Critical bgfx Setup (Complete)

Platform data must be set in **THREE places**:

```cpp
bgfx::PlatformData pd = /* NSWindow pointer */;
bgfx::setPlatformData(pd);    // 1. Global

bgfx::renderFrame();           // 2. CRITICAL - Single-threaded mode!

bgfx::Init init;
init.platformData = pd;        // 3. Also in Init struct!
init.type = bgfx::RendererType::Metal;
bgfx::init(init);
```

## Lessons Learned

### From SDL3 Success to bgfx Solution

1. **SDL3 works immediately** - Trusted system library, handles everything internally
2. **bgfx requires explicit setup** - But provides full low-level GPU control
3. **Single-threaded mode is critical** - Must call `renderFrame()` before `init()`
4. **Code signing necessary** - macOS requires entitlements for Metal GPU access
5. **Platform data twice** - Both globally and in Init struct (bgfx requirement)
6. **SDL3 API changes** - `SDL_Init()` returns bool, no `SDL_WINDOW_SHOWN` flag
7. **Pre-compiled shaders** - Metal shaders must be .bin format (use shaderc)

### Why Both Approaches Have Value

**Use SDL3 when:**
- Prototyping or simple demos
- Want to avoid shader compilation
- Don't need low-level GPU control
- Want cross-platform simplicity

**Use bgfx when:**
- Need full GPU pipeline control
- Advanced rendering techniques
- Custom shaders and effects
- Production game engine

## Dependencies

### sdl3_cubes (Pure SDL3)
- SDL 3.2.20 (only)

### standalone_cubes (bgfx + Metal)
- SDL 3.2.20
- bgfx 1.129 (via Conan)
- bx, bimg (bgfx components)
- Pre-compiled Metal shaders (vs_cubes.bin, fs_cubes.bin)
- Code signing with entitlements

## Source

Inspired by bgfx examples but adapted to demonstrate both pure SDL3 and bgfx approaches.
The critical `renderFrame()` fix was discovered through inference from SDL3's success and
analysis of working bgfx examples.

## License

This code: MIT License
bgfx: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
