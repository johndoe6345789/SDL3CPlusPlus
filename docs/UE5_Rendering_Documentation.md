# Unreal Engine 5 Rendering System Documentation
## Understanding How Professional Game Engines Prevent "Full Bright or Completely Dark" Scenes

This documentation explains how Unreal Engine 5's rendering pipeline works, with specific focus on the systems that prevent scenes from appearing too bright or too dark.

---

## Table of Contents
1. [The Core Problem: Why Scenes Appear Too Bright or Dark](#the-core-problem)
2. [Overall Rendering Pipeline](#overall-rendering-pipeline)
3. [Deferred Shading Architecture](#deferred-shading-architecture)
4. [The Critical Solution: Exposure Control](#the-critical-solution-exposure-control)
5. [Tone Mapping](#tone-mapping)
6. [Implementation Checklist for Your Engine](#implementation-checklist)
7. [Key Files Reference](#key-files-reference)

---

## The Core Problem

When rendering in **HDR (High Dynamic Range)**, scene luminance values can range from 0.001 to 100,000+. However, displays can only show values from 0 to 1 (or 0 to 255 in 8-bit).

**Without proper exposure and tone mapping:**
- Bright scenes: Values > 1.0 get clamped to white → "Full Bright"
- Dark scenes: Values < 0.1 appear black on screen → "Completely Dark"

**The solution requires TWO separate systems:**
1. **Exposure** - Scale HDR values to a reasonable range
2. **Tone Mapping** - Map HDR to LDR with a curve that preserves detail

---

## Overall Rendering Pipeline

UE5 uses a deferred rendering pipeline with distinct stages:

```
┌─────────────────────────────────────────────────────────┐
│ 1. GEOMETRY PASS (Write to GBuffer)                    │
│    - Depth PrePass                                      │
│    - Base Pass: Albedo, Normals, Roughness, Metallic   │
│    Output: GBuffer (multiple render targets in HDR)    │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│ 2. LIGHTING PASS (Deferred Lighting)                   │
│    - Read GBuffer                                       │
│    - Accumulate all lights                             │
│    - Shadows, reflections, ambient occlusion           │
│    Output: Scene Color (HDR linear, can be 0-10000+)   │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│ 3. POST-PROCESSING (THIS IS WHERE THE MAGIC HAPPENS)   │
│    a. Calculate scene luminance (histogram/average)    │
│    b. Eye Adaptation (auto exposure calculation)       │
│    c. Bloom                                            │
│    d. Tone Mapping (exposure + tone curve)             │
│    e. Color grading                                     │
│    f. Gamma correction                                  │
│    Output: Final LDR image (0-1 range for display)    │
└─────────────────────────────────────────────────────────┘
```

**Key File:** `Engine/Source/Runtime/Renderer/Private/DeferredShadingRenderer.cpp:1699`
- Function: `FDeferredShadingSceneRenderer::Render()`
- This is the main rendering loop that coordinates all stages

---

## Deferred Shading Architecture

### What is Deferred Shading?

Traditional forward rendering:
```
For each object:
    Apply all lights to this object
    Output final color
```

Deferred rendering:
```
Pass 1 - Geometry Pass:
    For each object:
        Write material properties to GBuffer (albedo, normal, roughness, etc.)

Pass 2 - Lighting Pass:
    For each pixel:
        Read material properties from GBuffer
        For each light:
            Calculate lighting contribution
        Sum all lighting
        Output HDR scene color
```

### GBuffer Layout

UE5's GBuffer contains:
- **GBufferA** - World Normal (RGB), Per-object data (A)
- **GBufferB** - Metallic (R), Specular (G), Roughness (B), Shading Model (A)
- **GBufferC** - Base Color (RGB), Ambient Occlusion (A)
- **GBufferD** - Custom data / Subsurface color
- **GBufferE** - Precomputed shadow factors

**Key Files:**
- `Engine/Source/Runtime/Renderer/Private/BasePassRendering.cpp` - GBuffer generation
- `Engine/Shaders/Private/DeferredShadingCommon.ush` - GBuffer encoding/decoding
- `Engine/Source/Runtime/Renderer/Private/LightRendering.cpp` - Deferred lighting

### Why This Matters for Your Engine

The lighting pass outputs **HDR linear light values** that can be much brighter than 1.0. For example:
- A sunlit white surface might have luminance of 10.0
- A bright light source might contribute 50.0
- A dark shadow might be 0.01

These HDR values MUST be processed by exposure and tone mapping to look correct on screen.

---

## The Critical Solution: Exposure Control

This is the #1 system you're missing that causes "full bright or completely dark" scenes.

### What is Exposure?

In photography and rendering, **exposure** is how much light reaches the sensor/film. It's controlled by:
- **Aperture (f-stop)** - How wide the lens opens
- **Shutter Speed** - How long the sensor is exposed
- **ISO** - Sensor sensitivity

UE5 uses the **EV100** system (Exposure Value at ISO 100):
```
EV100 = log2((f-stop² × shutter_speed × 100) / ISO)
```

**Key Insight:** Different scenes need different exposure values:
- Bright outdoor scene: EV100 = 15 (low exposure, dim it down)
- Indoor scene: EV100 = 8 (high exposure, brighten it up)
- Night scene: EV100 = 2 (very high exposure)

### Eye Adaptation (Auto Exposure)

UE5 implements automatic exposure similar to a camera or human eye adapting to brightness.

**File:** `Engine/Source/Runtime/Renderer/Private/PostProcess/PostProcessEyeAdaptation.cpp`

**Three Methods:**

1. **Histogram-based (AEM_Histogram)** - Most accurate
   - Analyzes luminance histogram of the scene
   - Finds median luminance
   - Adjusts exposure to make median map to "middle grey" (18% or EV 0)

2. **Basic (AEM_Basic)** - Faster
   - Calculates average scene luminance
   - Adjusts exposure based on average

3. **Manual (AEM_Manual)** - Artist controlled
   - Fixed exposure value set by designer

### How Eye Adaptation Works

**Step 1: Calculate Scene Luminance**
```cpp
// Shader: Engine/Shaders/Private/PostProcessHistogram.usf
// For each pixel in downsampled scene:
float Luminance = dot(SceneColor.rgb, float3(0.299, 0.587, 0.114));
// Build histogram of luminance values
```

**Step 2: Determine Target Exposure**
```cpp
// From PostProcessEyeAdaptation.cpp:395
float BasePhysicalCameraEV100 =
    log2((DepthOfFieldFstop² × CameraShutterSpeed × 100) / max(1, CameraISO));

// Apply exposure compensation (artist adjustment)
float FinalEV100 = BasePhysicalCameraEV100 + ExposureCompensation;

// Convert to linear exposure multiplier
float Exposure = EV100ToLuminance(FinalEV100);
```

**Step 3: Temporal Smoothing**
```cpp
// Smooth exposure changes over time to avoid flickering
float NewExposure = lerp(OldExposure, TargetExposure, AdaptationSpeed × DeltaTime);
```

**Critical Function:** `FViewInfo::UpdatePreExposure()` at line 1383

### Pre-Exposure: A Critical Optimization

UE5 applies exposure **during rendering** (not just in post-processing) to maintain precision in HDR buffers.

```cpp
// During lighting pass:
SceneColor = BaseColor × LightColor × LightIntensity × PreExposure;

// PreExposure is calculated as:
PreExposure = CalculatedExposure × GlobalExposure × VignetteMask;
```

**Why Pre-Exposure Matters:**
- Prevents HDR values from getting too large (overflow)
- Maintains precision in 16-bit float buffers
- Reduces banding in bright and dark areas

**Key File:** `Engine/Shaders/Private/EyeAdaptationCommon.ush`

### EV100 Conversion Functions

```cpp
// Engine/Source/Runtime/Renderer/Private/PostProcess/PostProcessEyeAdaptation.cpp

// Convert EV100 to linear luminance multiplier
float EV100ToLuminance(float EV100)
{
    return exp2(EV100 - 3.0f);
}

// Convert luminance to EV100
float LuminanceToEV100(float Luminance)
{
    return log2(Luminance) + 3.0f;
}

// The -3.0 offset accounts for the ISO 100 baseline and lens attenuation
```

### Console Variables for Exposure

```cpp
// Override exposure calculation
r.EyeAdaptation.PreExposureOverride = -1.0  // -1 = auto, > 0 = manual override
r.EyeAdaptation.MethodOverride = -1         // -1 = auto, 0 = manual, 1 = basic, 2 = histogram
r.EyeAdaptation.LensAttenuation = 0.78      // Lens q factor (calibration)
r.EyeAdaptation.ExposureCompensation = 0.0  // Artist bias in EV stops
r.EyeAdaptation.MinBrightness = 0.0         // Minimum scene luminance
r.EyeAdaptation.MaxBrightness = 2.0         // Maximum scene luminance
```

---

## Tone Mapping

After exposure scales the HDR values, **tone mapping** converts HDR to LDR for display.

**File:** `Engine/Source/Runtime/Renderer/Private/PostProcess/PostProcessTonemap.cpp`

### Why Tone Mapping is Necessary

Even with perfect exposure, HDR still has values outside [0, 1]:
- Bright highlights might be 2.0
- Very bright lights might be 5.0
- Specular reflections might be 10.0

**Linear clamp** (your current approach?) would make these all white, losing detail.

**Tone mapping** uses a **curve** that:
- Preserves midtones (values near 0.18)
- Compresses highlights smoothly (values > 1.0)
- Darkens shadows gently (values < 0.18)

### Tone Mapping Curve Types in UE5

**1. Filmic Tone Curve (Default)**

A custom curve with artistic control:
```cpp
// Parameters:
- Toe: Controls shadow compression
- Shoulder: Controls highlight compression
- Slope: Controls midtone contrast
- WhitePoint: Brightest value that maps to white
```

The curve looks like an "S" shape that smoothly compresses both bright and dark values.

**2. ACES (Academy Color Encoding System)**

Industry-standard tone mapper used in film:
- ACES 1.3 (previous standard)
- ACES 2.0 (current standard)

More accurate color preservation, especially for bright lights and HDR workflows.

**Shader:** `Engine/Shaders/Private/TonemapCommon.ush:415`

### Filmic Tone Curve Math

```hlsl
// Simplified version from TonemapCommon.ush
float3 FilmicTonemap(float3 LinearColor)
{
    // Apply exposure first
    float3 ExposedColor = LinearColor * Exposure;

    // Filmic curve (simplified)
    float3 x = max(0, ExposedColor - 0.004);
    float3 ToneMappedColor = (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);

    return ToneMappedColor;
}
```

**Full implementation:** `Engine/Shaders/Private/TonemapCommon.ush:97` (FilmToneMap function)

### ACES Tone Mapping

```hlsl
// From TonemapCommon.ush
float3 ACESTonemap(float3 LinearColor)
{
    // Convert to ACES color space
    float3 ACESColor = LinearToACES(LinearColor);

    // Apply RRT (Reference Rendering Transform) + ODT (Output Device Transform)
    float3 ODTColor = ACESOutputTransforms(ACESColor);

    return ODTColor;
}
```

**Full ACES implementation:** `Engine/Shaders/Private/ACES.ush`

### Gamma Correction

After tone mapping, the final step is **gamma correction** for display:

```hlsl
// sRGB gamma curve
float3 LinearToSRGB(float3 LinearColor)
{
    // Simplified: FinalColor = pow(LinearColor, 1/2.2)
    // Actual sRGB is piecewise with linear segment near black
}
```

Most displays expect sRGB gamma, so this is applied at the very end.

### Tone Mapping Console Variables

```cpp
r.Tonemapper.Sharpen = 0.5              // Sharpening amount
r.TonemapperGamma = 2.2                 // Gamma curve
r.Tonemapper.GrainQuantization = 1      // Film grain
r.Tonemapper.Quality = 5                // Quality level
```

---

## Implementation Checklist for Your Engine

Based on UE5's architecture, here's what you need to fix the "full bright or completely dark" problem:

### Phase 1: Basic Exposure (Minimum Viable)

- [ ] **Calculate scene luminance**
  - Downsample scene to 64×64 or smaller
  - Calculate average luminance: `Lum = 0.299×R + 0.587×G + 0.114×B`

- [ ] **Calculate exposure value**
  ```cpp
  float targetLuminance = 0.18; // Middle grey
  float avgLuminance = CalculateSceneLuminance();
  float exposure = targetLuminance / (avgLuminance + 0.001);
  exposure = clamp(exposure, 0.1, 10.0); // Prevent extremes
  ```

- [ ] **Apply basic tone mapping**
  ```cpp
  // Reinhard tone mapping (simplest)
  float3 ToneMapped = ExposedColor / (ExposedColor + 1.0);

  // Or Uncharted 2 (better highlights)
  float3 ToneMapped = FilmicCurve(ExposedColor);
  ```

- [ ] **Add gamma correction**
  ```cpp
  float3 Final = pow(ToneMapped, 1.0/2.2); // sRGB gamma
  ```

### Phase 2: Smooth Adaptation

- [ ] **Temporal smoothing of exposure**
  ```cpp
  float adaptationSpeed = 2.0; // EV per second
  newExposure = lerp(oldExposure, targetExposure,
                     adaptationSpeed × deltaTime);
  ```

- [ ] **Store exposure in texture/buffer**
  - Use 1×1 texture to store previous frame's exposure
  - Read in next frame for smooth interpolation

### Phase 3: Advanced (Match UE5)

- [ ] **Histogram-based exposure**
  - Build 64-bin luminance histogram
  - Find median or 80th percentile luminance
  - More robust than average

- [ ] **EV100 system**
  ```cpp
  float CalculateEV100(float aperture, float shutterSpeed, float ISO)
  {
      return log2((aperture * aperture * shutterSpeed * 100.0) / ISO);
  }

  float EV100ToExposure(float EV100)
  {
      return exp2(EV100 - 3.0); // -3 for calibration
  }
  ```

- [ ] **Pre-exposure application**
  - Apply exposure during lighting pass
  - Modify light intensity by pre-exposure value

- [ ] **ACES tone mapping**
  - Use ACES color space transforms
  - Industry-standard color accuracy

### Phase 4: Artist Control

- [ ] **Exposure compensation**
  - Allow artists to bias exposure up/down
  - Additive offset to EV100

- [ ] **Min/max luminance clamps**
  - Prevent adaptation from going too bright/dark
  - Useful for specific game scenarios

- [ ] **Zone-based exposure**
  - Weight center of screen more than edges
  - Prevents sky from overexposing character

---

## Key Files Reference

### C++ Source Files

**Main Rendering Loop:**
- `Engine/Source/Runtime/Renderer/Private/DeferredShadingRenderer.cpp:1699`
  - `FDeferredShadingSceneRenderer::Render()` - Main render loop

**Post-Processing Coordination:**
- `Engine/Source/Runtime/Renderer/Private/PostProcess/PostProcessing.cpp`
  - `AddPostProcessingPasses()` - Builds post-process chain
  - `AddHistogramEyeAdaptationPass()` - Histogram exposure calculation
  - `AddBasicEyeAdaptationPass()` - Simple exposure calculation

**Eye Adaptation (Auto Exposure):**
- `Engine/Source/Runtime/Renderer/Private/PostProcess/PostProcessEyeAdaptation.cpp`
  - Line 395: Physical camera EV100 calculation
  - Line 1383: `FViewInfo::UpdatePreExposure()` - Pre-exposure calculation
  - EV100 conversion functions

**Tone Mapping:**
- `Engine/Source/Runtime/Renderer/Private/PostProcess/PostProcessTonemap.cpp`
  - Tone mapping pass setup
  - Color grading integration
  - LUT application

**Lighting:**
- `Engine/Source/Runtime/Renderer/Private/LightRendering.cpp`
  - Deferred light rendering
  - Shadow application
  - Light accumulation in HDR

**GBuffer Generation:**
- `Engine/Source/Runtime/Renderer/Private/BasePassRendering.cpp`
  - Base pass rendering
  - Material property encoding

### Shader Files (HLSL/USH)

**Exposure/Eye Adaptation:**
- `Engine/Shaders/Private/EyeAdaptationCommon.ush`
  - Exposure buffer access functions
  - `EyeAdaptationLookup()` - Read current exposure value

**Tone Mapping:**
- `Engine/Shaders/Private/TonemapCommon.ush`
  - Line 97: `FilmToneMap()` - Filmic tone curve
  - Line 415: ACES tone mapping
  - Color grading functions
  - Gamma correction

**ACES:**
- `Engine/Shaders/Private/ACES.ush`
  - Complete ACES color space implementation
  - RRT and ODT transforms

**Histogram Generation:**
- `Engine/Shaders/Private/PostProcessHistogram.usf`
  - Luminance calculation
  - Histogram bin accumulation

**Deferred Shading:**
- `Engine/Shaders/Private/DeferredShadingCommon.ush`
  - GBuffer encoding/decoding
  - Material property access

**Lighting Calculations:**
- `Engine/Shaders/Private/DeferredLightingCommon.ush`
  - BRDF functions
  - Light attenuation
  - Shadow sampling

### Key Data Structures

**View Information:**
- `Engine/Source/Runtime/Engine/Public/SceneView.h`
  - `FSceneView` - Camera and view settings
  - Contains exposure values, FOV, projection matrices

**Scene Representation:**
- `Engine/Source/Runtime/Renderer/Private/ScenePrivate.h`
  - `FScene` - Scene graph and objects
  - Light arrays, primitive arrays

---

## Quick Reference: The Exposure Pipeline

```
Frame N:
┌─────────────────────────────────────────────────────┐
│ 1. Render scene with PreExposure from Frame N-1    │
│    SceneColor = Lighting × PreExposure              │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 2. Calculate luminance of rendered scene           │
│    - Downsample to 64×64                           │
│    - Build histogram (or calculate average)        │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 3. Calculate target exposure for next frame        │
│    - Find median/average luminance                 │
│    - Convert to EV100                              │
│    - Apply compensation and limits                 │
│    - Smooth with previous exposure                 │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ 4. Apply tone mapping to current frame             │
│    - Apply additional exposure if needed           │
│    - Run tone mapping curve (Filmic/ACES)          │
│    - Color grading                                 │
│    - Gamma correction                              │
└─────────────────────────────────────────────────────┘
                        ↓
                Display Frame N
```

---

## Console Commands for Testing

When studying UE5, try these console commands to see how exposure affects the scene:

```cpp
// Disable auto exposure to see raw HDR
r.EyeAdaptation.MethodOverride 0
r.EyeAdaptation.PreExposureOverride 1.0

// Now manually adjust exposure
r.EyeAdaptation.PreExposureOverride 0.5  // Darker
r.EyeAdaptation.PreExposureOverride 2.0  // Brighter
r.EyeAdaptation.PreExposureOverride 10.0 // Very bright

// Re-enable auto exposure
r.EyeAdaptation.MethodOverride -1

// Test different tone mappers
r.Tonemapper.Quality 0  // Legacy
r.Tonemapper.Quality 5  // Full quality with ACES

// Show debug histogram
r.EyeAdaptation.VisualizeDebugType 1
```

---

## Summary: Why Your Engine Shows "Full Bright or Completely Dark"

Your game engine likely has one or more of these issues:

1. **Missing Auto Exposure**
   - Exposure value is fixed (probably 1.0)
   - Bright scenes overflow to white
   - Dark scenes underflow to black

2. **Missing Tone Mapping**
   - HDR values > 1.0 get clamped to white
   - No smooth rolloff for highlights
   - All bright lights look the same

3. **Missing Gamma Correction**
   - Linear light values appear darker on screen
   - Should apply gamma 2.2 for sRGB displays

4. **Incorrect Light Units**
   - Light intensities might be in wrong units
   - Too bright or too dark by default

**The minimal fix:**
```cpp
// Pseudocode for post-processing
float avgLuminance = CalculateAverageLuminance(sceneColor);
float exposure = 0.18 / (avgLuminance + 0.001);
exposure = clamp(exposure, 0.1, 10.0);

float3 exposedColor = sceneColor.rgb * exposure;
float3 toneMapped = exposedColor / (exposedColor + 1.0); // Reinhard
float3 gammaCorrected = pow(toneMapped, 1.0/2.2);

outputColor = gammaCorrected;
```

This simple change will prevent "full bright or completely dark" and give you a starting point to build a more sophisticated system like UE5's.

---

## Further Study

**Recommended reading order:**

1. Start with: `PostProcessEyeAdaptation.cpp` - Understand exposure calculation
2. Then read: `TonemapCommon.ush` - See actual tone mapping shaders
3. Study: `PostProcessing.cpp` - See how passes connect
4. Finally: `DeferredShadingRenderer.cpp:1699` - Full render loop

**Key concepts to understand:**

- HDR vs LDR rendering
- EV100 exposure system
- Histogram-based exposure
- Temporal smoothing
- Pre-exposure optimization
- Tone mapping curves (Filmic, ACES)
- Gamma correction and color spaces

Good luck with your game engine! The difference between "full bright or completely dark" and "properly exposed" is entirely in these post-processing steps.
