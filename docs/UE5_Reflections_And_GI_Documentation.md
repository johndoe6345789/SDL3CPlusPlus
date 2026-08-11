# Unreal Engine 5 Reflections and Global Illumination Documentation
## Modern Techniques for Real-Time Game Engines

This documentation explains UE5's reflection and global illumination systems, from traditional techniques to cutting-edge Lumen.

---

## Table of Contents
1. [System Overview](#system-overview)
2. [Screen Space Reflections (SSR)](#screen-space-reflections-ssr)
3. [Reflection Capture Probes](#reflection-capture-probes)
4. [Planar Reflections](#planar-reflections)
5. [Ambient Occlusion (SSAO/GTAO)](#ambient-occlusion-ssaogtao)
6. [Lumen Global Illumination](#lumen-global-illumination)
7. [Lumen Reflections](#lumen-reflections)
8. [Implementation Strategy for Custom Engines](#implementation-strategy-for-custom-engines)
9. [bgfx Implementation Examples](#bgfx-implementation-examples)
10. [Key Files Reference](#key-files-reference)

---

## System Overview

UE5 uses a **hybrid approach** combining multiple reflection and GI techniques:

```
Reflection and GI Stack (from cheapest to most expensive):
┌────────────────────────────────────────────────────┐
│ 1. Ambient Occlusion (SSAO/GTAO)                  │
│    - Cost: ~0.5ms                                  │
│    - Purpose: Local shadowing, depth perception    │
└────────────────────────────────────────────────────┘
                         ↓
┌────────────────────────────────────────────────────┐
│ 2. Reflection Capture Probes                      │
│    - Cost: Free (pre-computed)                     │
│    - Purpose: Baseline environment reflections     │
└────────────────────────────────────────────────────┘
                         ↓
┌────────────────────────────────────────────────────┐
│ 3. Screen Space Reflections (SSR)                 │
│    - Cost: ~1-2ms                                  │
│    - Purpose: Sharp local reflections              │
└────────────────────────────────────────────────────┘
                         ↓
┌────────────────────────────────────────────────────┐
│ 4. Lumen GI + Reflections                         │
│    - Cost: ~4-8ms (scalable)                       │
│    - Purpose: Dynamic global illumination          │
└────────────────────────────────────────────────────┘
                         ↓ (optional)
┌────────────────────────────────────────────────────┐
│ 5. Planar Reflections                             │
│    - Cost: ~Full scene render                      │
│    - Purpose: Perfect mirrors/water                │
└────────────────────────────────────────────────────┘
```

**Key Principle:** Each technique fills gaps in the others:
- **Probes**: Baseline, always available, static environment
- **SSR**: Sharp reflections of visible geometry
- **Lumen**: Dynamic GI, rough reflections, off-screen content
- **Planar**: Perfect mirrors (expensive, used sparingly)
- **AO**: Enhances all the above with contact shadowing

---

## Screen Space Reflections (SSR)

SSR traces rays in screen space to find reflections. Fast but limited to visible geometry.

**Key Files:**
- `Engine/Source/Runtime/Renderer/Private/ScreenSpaceReflectionTiles.cpp`
- `Engine/Shaders/Private/ScreenSpaceReflectionTileCommons.ush`

### Tiled Optimization

UE5 uses tile-based optimization to skip pixels that don't need SSR:

```cpp
// Tile categorization (8×8 pixel tiles)
struct FSSRTileClassification
{
    bool bNeedsSSR;           // Any pixel in tile needs SSR?
    float MaxRoughness;       // Roughest pixel in tile
    float MinSpecular;        // Least specular pixel
};

// Mark tiles that need processing
void MarkSSRTiles(FRDGBuilder& GraphBuilder)
{
    // For each 8×8 tile:
    //   Sample GBuffer (roughness, specular, shading model)
    //   If roughness < threshold AND specular > threshold:
    //       Mark tile for SSR processing
    //   Else:
    //       Skip tile (use probes instead)

    AddPass_MarkSSRTiles<<<NumTiles>>>();
}

// Build coherent lists for better cache performance
void BuildSSRTileLists(FRDGBuilder& GraphBuilder)
{
    // Z-order curve for spatial coherency
    // Creates indirect dispatch arguments
    // Enables efficient skipping of non-SSR tiles
}
```

**Console Variables:**
```cpp
r.SSR.TiledComposite = 1                      // Enable tiled optimization
r.SSR.TiledComposite.OverrideMaxRoughness = -1 // Max roughness (-1 = use post-process)
r.SSR.TiledComposite.MinSpecular = 0.0        // Ignore low-specular pixels
```

### Ray Marching Algorithm

```hlsl
// Basic SSR ray marching
struct SSRHit
{
    bool bHit;
    float2 HitUV;
    float Confidence; // How reliable is this hit?
};

SSRHit TraceScreenSpaceRay(float3 startPos, float3 direction,
                           Texture2D depthBuffer, float maxDistance)
{
    const int numSteps = 32; // Linear steps
    const int numBinarySearchSteps = 4; // Refinement

    float3 rayPos = startPos;
    float stepSize = maxDistance / numSteps;

    // Linear ray march
    for (int i = 0; i < numSteps; ++i)
    {
        rayPos += direction * stepSize;

        // Project to screen space
        float4 screenPos = mul(ViewProjection, float4(rayPos, 1.0));
        screenPos.xyz /= screenPos.w;
        float2 screenUV = screenPos.xy * 0.5 + 0.5;
        screenUV.y = 1.0 - screenUV.y;

        // Out of screen bounds?
        if (any(screenUV < 0.0) || any(screenUV > 1.0))
            return (SSRHit)0;

        // Sample depth buffer
        float sceneDepth = depthBuffer.SampleLevel(PointSampler, screenUV, 0).r;
        float rayDepth = LinearizeDepth(screenPos.z);

        // Check if ray crossed surface
        if (rayDepth > sceneDepth)
        {
            // Binary search refinement
            float3 hitPos = BinarySearchIntersection(rayPos - direction * stepSize,
                                                     rayPos,
                                                     depthBuffer,
                                                     numBinarySearchSteps);

            // Calculate confidence (fade at edges, based on angle, etc.)
            float edgeFade = EdgeFade(screenUV);
            float angleFade = AngleFade(direction, sceneNormal);

            SSRHit hit;
            hit.bHit = true;
            hit.HitUV = ProjectToScreen(hitPos);
            hit.Confidence = edgeFade * angleFade;
            return hit;
        }
    }

    return (SSRHit)0; // Miss
}
```

### Hierarchical Ray Marching (HZB)

More advanced technique using Hi-Z buffer:

```hlsl
// Use mip hierarchy to accelerate ray marching
SSRHit TraceScreenSpaceRayHZB(float3 startPos, float3 direction,
                              Texture2D hzbBuffer, int maxLevel)
{
    float3 rayPos = startPos;
    int currentMipLevel = 0;

    while (currentMipLevel >= 0)
    {
        // Sample HZB at current mip
        float2 screenUV = ProjectToScreen(rayPos);
        float hzbDepth = hzbBuffer.SampleLevel(PointSampler, screenUV, currentMipLevel).r;
        float rayDepth = LinearizeDepth(rayPos.z);

        if (rayDepth > hzbDepth)
        {
            // Hit something - refine with lower mip
            currentMipLevel--;
        }
        else
        {
            // Empty space - advance ray
            // Step size based on mip level (coarser mips = larger steps)
            float stepSize = CalculateStepSize(currentMipLevel);
            rayPos += direction * stepSize;

            // Increase mip level if we can (larger steps)
            if (CanIncreaseMip(rayPos, currentMipLevel, maxLevel))
                currentMipLevel++;
        }

        // Reached finest mip and still hitting? Found intersection
        if (currentMipLevel < 0)
        {
            return CreateHit(rayPos);
        }
    }

    return (SSRHit)0;
}
```

### Temporal Reprojection

SSR benefits greatly from temporal accumulation:

```hlsl
// Current frame SSR (noisy, few samples)
float3 currentSSR = TraceSSR(worldPos, viewDir, roughness);

// Reproject previous frame
float2 velocity = VelocityBuffer.Sample(screenUV).xy;
float2 prevUV = screenUV - velocity;

float3 previousSSR = PreviousSSRBuffer.Sample(prevUV).rgb;

// Temporal blend
float temporalWeight = 0.9; // High persistence

// Reject history if scene changed too much
if (DepthDifference(prevUV) > threshold || NormalDifference(prevUV) > threshold)
    temporalWeight = 0.0; // Use current frame only

float3 finalSSR = lerp(currentSSR, previousSSR, temporalWeight);
```

### When to Use SSR

**Good for:**
- Smooth surfaces (roughness < 0.4)
- Reflections of nearby visible geometry
- Indoor scenes with lots of screen-space detail

**Bad for:**
- Rough surfaces (use probes or Lumen instead)
- Reflections of off-screen objects
- Screen edges (fadeout needed)

---

## Reflection Capture Probes

Pre-computed cubemaps placed in the world. Free at runtime, static lighting.

**Key Files:**
- `Engine/Source/Runtime/Renderer/Private/ReflectionEnvironmentCapture.cpp`
- `Engine/Source/Runtime/Renderer/Private/ReflectionEnvironment.cpp`
- `Engine/Shaders/Private/ReflectionEnvironmentShared.ush`

### Capture Process

```cpp
// Capture reflection cubemap
void CaptureReflectionProbe(FReflectionCaptureComponent* Probe)
{
    const int32 CubemapSize = 128; // Typical: 128, 256, 512
    const int32 NumMips = 7; // For roughness filtering

    // Create temp cubemap
    FTextureCubeRHIRef TempCubemap = CreateTextureCube(CubemapSize, PF_FloatRGBA, NumMips);

    // Render 6 faces
    for (int face = 0; face < 6; ++face)
    {
        // Setup camera facing this direction
        FVector CaptureDir = GetCubeFaceDirection(face);
        FVector CaptureUp = GetCubeFaceUp(face);

        FViewInfo CaptureView;
        CaptureView.Location = Probe->GetComponentLocation();
        CaptureView.Rotation = LookAt(CaptureDir, CaptureUp);
        CaptureView.FOV = 90.0f; // Cube face

        // Render scene from this view
        RenderSceneToTexture(CaptureView, TempCubemap, face, 0 /* mip 0 */);
    }

    // Generate mip chain (importance-sampled roughness filtering)
    for (int mip = 1; mip < NumMips; ++mip)
    {
        float roughness = MipToRoughness(mip);
        FilterCubemapForRoughness(TempCubemap, mip, roughness);
    }

    // Store in reflection capture array
    CopyToReflectionCaptureArray(Probe->CaptureIndex, TempCubemap);
}
```

### Roughness-Based Mip Selection

```hlsl
// From ReflectionEnvironmentShared.ush line 26
float ComputeReflectionCaptureMipFromRoughness(float Roughness, float CubemapMaxMip)
{
    // Constants for mip mapping
    const float REFLECTION_CAPTURE_ROUGHEST_MIP = 1.0;
    const float REFLECTION_CAPTURE_ROUGHNESS_MIP_SCALE = 1.2;

    // Logarithmic mapping: rougher surfaces use blurrier mips
    float LevelFrom1x1 = REFLECTION_CAPTURE_ROUGHEST_MIP
                       - REFLECTION_CAPTURE_ROUGHNESS_MIP_SCALE * log2(max(Roughness, 0.001));

    return CubemapMaxMip - 1.0 - LevelFrom1x1;
}

// Example: Roughness 0.0 → Mip 0 (sharpest)
//          Roughness 0.5 → Mip 3 (medium blur)
//          Roughness 1.0 → Mip 6 (blurriest)
```

### Cubemap Array Structure

```cpp
// All reflection captures stored in single TextureCubeArray
class FReflectionEnvironmentCubemapArray
{
    FTextureCubeArrayRHIRef CubemapArray;

    // Typically:
    // - Format: PF_FloatRGBA (HDR)
    // - Resolution: 128×128 per face
    // - Mips: 7 levels
    // - Max captures: ~5 per pixel (blended)

    int32 MaxCubemaps = 341; // Configurable
};
```

### Box Projection Correction

Standard cubemap sampling assumes infinite distance. Box projection corrects for room interiors:

```hlsl
// From ReflectionEnvironmentShared.ush line 136
float3 GetLookupVectorForBoxCapture(float3 reflectionVector,
                                    float3 worldPosition,
                                    float4 boxCapturePositionAndRadius,
                                    float4x4 boxTransform,
                                    float3 boxMinimum,
                                    float3 boxMaximum)
{
    // Transform ray to box local space
    float3 localPosition = mul(boxTransform, float4(worldPosition, 1.0)).xyz;
    float3 localReflection = mul((float3x3)boxTransform, reflectionVector);

    // Intersect ray with box bounds
    float3 invDir = 1.0 / localReflection;
    float3 firstPlane = (boxMinimum - localPosition) * invDir;
    float3 secondPlane = (boxMaximum - localPosition) * invDir;
    float3 furthestPlane = max(firstPlane, secondPlane);

    // Find nearest intersection
    float distance = min(min(furthestPlane.x, furthestPlane.y), furthestPlane.z);

    // Intersection point in local space
    float3 intersectPosition = localPosition + localReflection * distance;

    // Direction from capture center to intersection
    float3 captureVector = intersectPosition - boxCapturePositionAndRadius.xyz;

    return captureVector;
}
```

### Probe Blending

Multiple probes can influence a single pixel:

```hlsl
// Blend up to 3 reflection captures
float3 BlendReflectionCaptures(float3 worldPos, float3 normal, float3 reflectionVec,
                               float roughness, float3 specularColor)
{
    float3 totalReflection = 0;
    float totalWeight = 0;

    // Find nearest probes (sorted by distance)
    const int maxProbes = 3;
    ReflectionProbe probes[maxProbes];
    FindNearestProbes(worldPos, probes, maxProbes);

    for (int i = 0; i < maxProbes; ++i)
    {
        // Weight based on distance to probe
        float distance = length(worldPos - probes[i].position);
        float weight = 1.0 - saturate(distance / probes[i].influenceRadius);

        if (weight > 0.0)
        {
            // Box projection for parallax correction
            float3 lookupVec = BoxProjection(reflectionVec, worldPos, probes[i]);

            // Sample cubemap with roughness-based mip
            float mipLevel = RoughnessToMip(roughness, probes[i].numMips);
            float3 reflection = ReflectionCubemapArray.SampleLevel(samplerLinear,
                                                                   float4(lookupVec, probes[i].index),
                                                                   mipLevel).rgb;

            totalReflection += reflection * weight;
            totalWeight += weight;
        }
    }

    if (totalWeight > 0.0)
        totalReflection /= totalWeight;

    // Apply Fresnel
    float3 fresnel = EnvBRDFApprox(specularColor, roughness, saturate(dot(normal, -reflectionVec)));

    return totalReflection * fresnel;
}
```

### Best Practices

- **Placement**: Put probes at lighting transitions, room corners
- **Resolution**: 128×128 for most, 256×256 for important areas
- **Influence Radius**: Just large enough to cover area
- **Box vs Sphere**: Box for rooms (better parallax), Sphere for outdoor
- **Count**: Limit to 2-3 overlapping per pixel

---

## Planar Reflections

Perfect reflections for flat surfaces (mirrors, water, polished floors).

**Key Files:**
- `Engine/Source/Runtime/Renderer/Private/PlanarReflectionRendering.cpp`
- `Engine/Shaders/Private/PlanarReflectionShared.ush`

### Rendering Pipeline

```cpp
void RenderPlanarReflection(FPlanarReflectionSceneProxy* Proxy)
{
    // 1. Setup mirror view
    FVector PlaneOrigin = Proxy->ReflectionPlane.Origin;
    FVector PlaneNormal = Proxy->ReflectionPlane.Normal;

    // Mirror camera position across plane
    FVector MirroredCameraPos = MirrorPoint(CameraPosition, PlaneOrigin, PlaneNormal);

    // Mirror camera direction
    FVector MirroredCameraDir = MirrorVector(CameraDirection, PlaneNormal);

    // Flipped view matrix
    FMatrix MirrorViewMatrix = CreateViewMatrix(MirroredCameraPos, MirroredCameraDir);

    // Oblique projection (clip to plane)
    FMatrix ObliqueProjection = CreateObliqueProjection(ProjectionMatrix,
                                                        PlaneOrigin,
                                                        PlaneNormal,
                                                        MirrorViewMatrix);

    // 2. Render scene from mirror view
    FSceneRenderer::RenderReflectionView(MirrorViewMatrix,
                                        ObliqueProjection,
                                        PlanarReflectionRT);

    // 3. Optional: Prefilter for roughness
    if (bPrefilterRoughness)
    {
        FilterPlanarReflectionForRoughness(PlanarReflectionRT, Roughness);
    }
}
```

### Prefiltering for Roughness

```cpp
// Apply depth-aware blur based on material roughness
template<bool bEnablePlanarReflectionPrefilter>
class TPrefilterPlanarReflectionPS : public FGlobalShader
{
    void Execute(FRHICommandList& RHICmdList)
    {
        // Parameters
        float KernelRadiusY; // Blur kernel size
        float InvPrefilterRoughnessDistance; // Distance-based blur

        // For each pixel:
        for (int i = 0; i < numSamples; ++i)
        {
            float2 offset = GetSampleOffset(i);

            // Sample reflection
            float3 reflection = PlanarRT.Sample(uv + offset);

            // Sample depth
            float depth = DepthBuffer.Sample(uv + offset);

            // Depth weight (don't blur across depth discontinuities)
            float depthWeight = exp(-abs(depth - centerDepth) * depthSharpness);

            sum += reflection * depthWeight * sampleWeight;
            totalWeight += depthWeight * sampleWeight;
        }

        OutColor = sum / totalWeight;
    }
};
```

### Projection in Material

```hlsl
// Sample planar reflection in pixel shader
float3 SamplePlanarReflection(float3 worldPos, float3 normal, float roughness)
{
    // Transform world pos to planar reflection space
    float4 reflectionPos = mul(PlanarReflectionMatrix, float4(worldPos, 1.0));
    reflectionPos.xyz /= reflectionPos.w;

    // Convert to UV
    float2 reflectionUV;
    reflectionUV.x = reflectionPos.x * 0.5 + 0.5;
    reflectionUV.y = -reflectionPos.y * 0.5 + 0.5; // Flip Y

    // Fade at edges
    float2 edgeFade = smoothstep(0.0, 0.1, reflectionUV) *
                      smoothstep(1.0, 0.9, reflectionUV);
    float fade = edgeFade.x * edgeFade.y;

    // Sample with roughness-based mip
    float mipLevel = roughness * MaxMipLevel;
    float3 reflection = PlanarReflectionTexture.SampleLevel(LinearSampler,
                                                            reflectionUV,
                                                            mipLevel).rgb;

    return reflection * fade;
}
```

### Performance Considerations

Planar reflections are **expensive** (full scene render):

```cpp
// Optimization: Limit what's rendered
r.PlanarReflection.MaxDistanceFromPlane = 500.0  // Cull distant objects
r.PlanarReflection.ScreenPercentage = 50.0       // Render at half resolution
r.PlanarReflection.Quality = 1                   // 0=low, 1=medium, 2=high

// Typical use cases:
// - Water surfaces (hero feature)
// - Bathroom mirrors (if player can see face)
// - Polished floors in key areas
```

**Best Practice:** Use sparingly (1-2 per scene max), prefer SSR or Lumen for most reflections.

---

## Ambient Occlusion (SSAO/GTAO)

Adds depth perception and contact shadows to indirect lighting.

**Key Files:**
- `Engine/Source/Runtime/Renderer/Private/CompositionLighting/PostProcessAmbientOcclusion.cpp`
- `Engine/Shaders/Private/PostProcessAmbientOcclusion.usf`

### SSAO (Screen Space Ambient Occlusion)

Classic technique using random samples in hemisphere:

```hlsl
float SSAO(float2 screenUV, Texture2D depthBuffer, Texture2D normalBuffer)
{
    // Reconstruct position and normal
    float depth = depthBuffer.Sample(screenUV).r;
    float3 worldPos = ReconstructWorldPosition(screenUV, depth);
    float3 normal = normalBuffer.Sample(screenUV).rgb * 2.0 - 1.0;

    // Sample kernel (random points in hemisphere)
    const int numSamples = 16;
    float3 randomVec = RandomTexture.Sample(screenUV * NoiseScale).rgb;
    float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    float3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        // Sample point in hemisphere
        float3 sampleOffset = TBN * HemisphereSamples[i];
        float3 samplePos = worldPos + sampleOffset * SampleRadius;

        // Project to screen
        float4 screenSamplePos = mul(ViewProjection, float4(samplePos, 1.0));
        screenSamplePos.xy /= screenSamplePos.w;
        float2 sampleUV = screenSamplePos.xy * 0.5 + 0.5;

        // Sample depth at this point
        float sampleDepth = depthBuffer.Sample(sampleUV).r;
        float sampleWorldDepth = LinearizeDepth(sampleDepth);

        // Check if sample is occluded
        float rangeCheck = smoothstep(0.0, 1.0, SampleRadius / abs(sampleWorldDepth - depth));
        occlusion += (sampleWorldDepth >= depth + Bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / numSamples);

    return pow(occlusion, Power); // Enhance contrast
}
```

**Multi-level SSAO** (UE5 improvement):

```cpp
// Sample at multiple scales using HZB mip chain
int32 NumLevels = 3; // Up to 3 passes

float totalAO = 0.0;
for (int level = 0; level < NumLevels; ++level)
{
    float radius = BaseRadius * pow(2.0, level);
    int mipLevel = level;

    // Sample HZB at this mip (faster for large radii)
    float ao = SSAO_Level(screenUV, radius, mipLevel);

    totalAO += ao;
}

totalAO /= NumLevels;
```

### GTAO (Ground Truth Ambient Occlusion)

More accurate method based on horizon angles:

```hlsl
float GTAO(float2 screenUV, Texture2D depthBuffer, Texture2D normalBuffer)
{
    float depth = depthBuffer.Sample(screenUV).r;
    float3 viewPos = ReconstructViewPosition(screenUV, depth);
    float3 viewNormal = normalBuffer.Sample(screenUV).rgb;

    // GTAO parameters
    const int numDirections = 2; // Angular slices
    const int numSteps = 4;      // Samples per direction

    float totalAO = 0.0;

    for (int dir = 0; dir < numDirections; ++dir)
    {
        // Direction in screen space
        float angle = (PI / numDirections) * dir;
        float2 direction = float2(cos(angle), sin(angle));

        // Search for horizon angles
        float horizonAnglePos = -PI / 2.0; // Start at -90°
        float horizonAngleNeg = -PI / 2.0;

        for (int step = 1; step <= numSteps; ++step)
        {
            float stepSize = (FalloffEnd / numSteps) * step;
            float2 sampleUV = screenUV + direction * stepSize;

            // Sample depth
            float sampleDepth = depthBuffer.Sample(sampleUV).r;
            float3 samplePos = ReconstructViewPosition(sampleUV, sampleDepth);

            // Calculate angle to sample
            float3 horizonVec = samplePos - viewPos;
            float horizonAngle = atan2(horizonVec.z, length(horizonVec.xy));

            // Update maximum horizon angle
            horizonAnglePos = max(horizonAnglePos, horizonAngle);

            // Sample in negative direction too
            sampleUV = screenUV - direction * stepSize;
            sampleDepth = depthBuffer.Sample(sampleUV).r;
            samplePos = ReconstructViewPosition(sampleUV, sampleDepth);
            horizonVec = samplePos - viewPos;
            horizonAngle = atan2(horizonVec.z, length(horizonVec.xy));
            horizonAngleNeg = max(horizonAngleNeg, horizonAngle);
        }

        // Integrate AO from horizon angles
        // (involves integration over the hemisphere, see paper for details)
        float ao = IntegrateAO(horizonAnglePos, horizonAngleNeg, viewNormal, direction);
        totalAO += ao;
    }

    totalAO /= numDirections;

    return totalAO;
}
```

**GTAO Async Compute** (UE5 optimization):

```cpp
// Run GTAO on async compute queue
enum class EGTAOType
{
    EOff,
    EAsyncHorizonSearch,      // GTAO with async compute
    EAsyncCombinedSpatial,    // GTAO + spatial filter async
    ENonAsync                 // Graphics pipeline (fallback)
};

// Pass 1 (Async): Horizon search
// [GPU fence]
// Pass 2 (Graphics): Integration, spatial filter, temporal filter
// Pass 3 (Graphics): Upsample (if rendered at half-res)
```

**Console Variables:**
```cpp
r.AO.Method = 1                     // 0=SSAO, 1=GTAO
r.AO.Quality = 2                    // 0-4
r.AO.Radius = 100.0                 // World-space radius
r.AO.Power = 1.5                    // Contrast enhancement
r.GTAO.NumAngles = 2                // Angular samples (1-16)
r.GTAO.Upsample = 1                 // Enable upsampling
```

### Bilateral Filtering

Both SSAO and GTAO benefit from edge-preserving blur:

```hlsl
float BilateralFilter(float2 uv, Texture2D aoBuffer,
                      Texture2D depthBuffer, Texture2D normalBuffer)
{
    float centerDepth = depthBuffer.Sample(uv).r;
    float3 centerNormal = normalBuffer.Sample(uv).rgb;
    float centerAO = aoBuffer.Sample(uv).r;

    float totalAO = centerAO;
    float totalWeight = 1.0;

    // 5×5 kernel
    for (int y = -2; y <= 2; ++y)
    {
        for (int x = -2; x <= 2; ++x)
        {
            if (x == 0 && y == 0) continue;

            float2 offset = float2(x, y) * TexelSize;
            float2 sampleUV = uv + offset;

            float sampleDepth = depthBuffer.Sample(sampleUV).r;
            float3 sampleNormal = normalBuffer.Sample(sampleUV).rgb;
            float sampleAO = aoBuffer.Sample(sampleUV).r;

            // Depth weight (don't blur across edges)
            float depthDiff = abs(sampleDepth - centerDepth);
            float depthWeight = exp(-depthDiff * DepthSigma);

            // Normal weight
            float normalWeight = pow(max(0.0, dot(centerNormal, sampleNormal)), NormalPower);

            // Spatial weight (Gaussian)
            float spatialWeight = exp(-dot(offset, offset) * SpatialSigma);

            float weight = depthWeight * normalWeight * spatialWeight;

            totalAO += sampleAO * weight;
            totalWeight += weight;
        }
    }

    return totalAO / totalWeight;
}
```

---

## Lumen Global Illumination

UE5's flagship dynamic GI system. Complex but transformative.

**Key Files:**
- `Engine/Source/Runtime/Renderer/Private/Lumen/LumenScreenProbeGather.cpp` (4,100+ lines)
- `Engine/Source/Runtime/Renderer/Private/Lumen/LumenScene.cpp`
- `Engine/Source/Runtime/Renderer/Private/Lumen/LumenRadianceCache.cpp`

### Three Core Components

#### 1. Surface Cache (Scene Representation)

Lumen represents the scene as "cards" (simplified surfaces):

```cpp
// Scene stored as oriented cards
struct FLumenCard
{
    FVector Origin;
    FVector Orientation;    // Local axes
    FVector2D Size;        // Dimensions
    uint32 MeshIndex;      // Which mesh
    uint32 AtlasAllocation; // Where in atlas
};

// Cards rasterized into atlases
struct FLumenSurfaceCache
{
    // Virtual texture atlases
    FRDGTextureRef AlbedoAtlas;    // Base color
    FRDGTextureRef NormalAtlas;    // World-space normals
    FRDGTextureRef EmissiveAtlas;  // Emissive lighting
    FRDGTextureRef DepthAtlas;     // Depth for parallax

    // Each mesh gets 1-6 cards (like a simplified bounding box)
    // Cards are streamed based on importance
};
```

**Card generation:**
```cpp
void GenerateLumenCards(FStaticMeshSceneProxy* Mesh)
{
    // For each axis-aligned direction (±X, ±Y, ±Z):
    //   Create a card facing that direction
    //   Size = mesh projection onto that plane
    //   Store in card array

    // Typically 6 cards per mesh (box-like)
    // Complex meshes may use multiple cards per face
}
```

#### 2. Screen Probe Gather (Final Gather)

Distributes probes across screen, each tracing rays:

```cpp
// Probe placement
struct FScreenProbeGather
{
    // Uniform grid: One probe per 16×16 pixel tile
    int32 ProbeDownsampleFactor = 16;

    // Adaptive probes: Additional probes at detail areas
    int32 NumAdaptiveProbes = 8;
    float AdaptiveProbeFraction = 0.5;

    // Each probe traces ~64 rays (octahedral distribution)
    int32 OctahedronResolution = 8; // 8×8 = 64 rays
};

void PlaceScreenProbes(FRDGBuilder& GraphBuilder)
{
    // Uniform grid
    int probesX = ScreenWidth / DownsampleFactor;
    int probesY = ScreenHeight / DownsampleFactor;

    // Place adaptive probes at:
    // - Geometric discontinuities (depth edges)
    // - Shadow boundaries
    // - High variance areas (from previous frame)

    AddPass_PlaceAdaptiveProbes<<<...>>>();
}
```

**Probe tracing:**
```hlsl
// For each screen probe:
void TraceScreenProbe(uint2 probeCoord)
{
    // Get world position for this probe
    float depth = DepthBuffer[probeCoord * DownsampleFactor];
    float3 worldPos = ReconstructWorldPosition(probeCoord, depth);
    float3 normal = NormalBuffer[probeCoord * DownsampleFactor].rgb;

    // Octahedral ray distribution (uniform hemispherical coverage)
    const int resolution = 8;
    float3 radiance[resolution * resolution];

    for (int y = 0; y < resolution; ++y)
    {
        for (int x = 0; x < resolution; ++x)
        {
            // Octahedral UV to direction
            float2 octUV = float2(x, y) / float2(resolution);
            float3 rayDir = OctahedralToDirection(octUV, normal);

            // Trace ray against surface cache
            FLumenRayHit hit = TraceLumenRay(worldPos, rayDir, MaxTraceDistance);

            if (hit.bHit)
            {
                // Sample surface cache at hit point
                radiance[y * resolution + x] = SampleSurfaceCache(hit.CardCoord);
            }
            else
            {
                // Hit sky
                radiance[y * resolution + x] = SampleSkyLight(rayDir);
            }
        }
    }

    // Store in octahedral atlas
    WriteOctahedralProbe(probeCoord, radiance);
}
```

**Interpolation to pixels:**
```hlsl
// For each pixel:
float3 GatherIndirectLighting(float2 screenUV)
{
    // Find surrounding 4 probes
    float2 probeCoord = screenUV / ProbeDownsampleFactor;
    int2 probe00 = floor(probeCoord);
    int2 probe01 = probe00 + int2(0, 1);
    int2 probe10 = probe00 + int2(1, 0);
    int2 probe11 = probe00 + int2(1, 1);

    // Bilinear weights
    float2 frac = fract(probeCoord);

    // Get world position and normal
    float3 worldPos = ...;
    float3 normal = ...;

    // Sample each probe (with depth-aware weighting)
    float3 lighting = 0;
    float totalWeight = 0;

    // Probe 00
    float3 probe00Dir = normalize(GetProbePosition(probe00) - worldPos);
    float probe00Weight = max(0, dot(normal, probe00Dir)) *
                          DepthWeight(worldPos, probe00) *
                          (1 - frac.x) * (1 - frac.y);
    lighting += SampleProbe(probe00, normal) * probe00Weight;
    totalWeight += probe00Weight;

    // ... repeat for probe01, probe10, probe11

    return lighting / totalWeight;
}
```

#### 3. Radiance Cache (Far-Field GI)

World-space probe grid for far-field lighting:

```cpp
struct FLumenRadianceCache
{
    // 3D clipmap structure (like VSM but in 3D)
    int32 NumClipmapLevels = 4;

    struct FClipmapLevel
    {
        FVector CenterPosition;    // Centered on camera
        float ProbeSpacing;        // Distance between probes
        FVector3i GridResolution;  // e.g., 64×64×64

        // Each probe stores octahedral radiance
        FRDGTextureRef ProbeRadiance; // R16G16B16A16F, octahedral
        FRDGTextureRef ProbeDepth;    // For parallax
    };

    FClipmapLevel Levels[NumClipmapLevels];

    // Level 0: High detail, small area (2m spacing)
    // Level 1: Medium detail, medium area (4m spacing)
    // Level 2: Low detail, large area (8m spacing)
    // Level 3: Very low detail, very large area (16m spacing)
};
```

**Usage:**
```hlsl
// Sample radiance cache for rough reflections or far-field GI
float3 SampleRadianceCache(float3 worldPos, float3 direction)
{
    // Determine clipmap level based on distance from camera
    float distanceFromCamera = length(worldPos - CameraPosition);
    int level = log2(distanceFromCamera / BaseSpacing);
    level = clamp(level, 0, NumLevels - 1);

    // Find surrounding 8 probes (trilinear interpolation)
    float3 localPos = (worldPos - Levels[level].CenterPosition) / Levels[level].ProbeSpacing;
    int3 probe000 = floor(localPos);
    float3 frac = fract(localPos);

    // Sample all 8 probes
    float3 radiance = 0;
    for (int z = 0; z < 2; ++z)
    {
        for (int y = 0; y < 2; ++y)
        {
            for (int x = 0; x < 2; ++x)
            {
                int3 probeCoord = probe000 + int3(x, y, z);
                float weight = lerp(1 - frac.x, frac.x, x) *
                               lerp(1 - frac.y, frac.y, y) *
                               lerp(1 - frac.z, frac.z, z);

                // Sample octahedral radiance in direction
                float3 probeRadiance = SampleProbeOctahedral(level, probeCoord, direction);

                radiance += probeRadiance * weight;
            }
        }
    }

    return radiance;
}
```

### Ray Tracing Methods

**Software Ray Tracing (Default):**
```cpp
// Uses Signed Distance Fields (SDF)
FLumenRayHit TraceLumenRay(float3 origin, float3 direction, float maxDistance)
{
    // Two-phase tracing:
    // 1. Mesh SDF (accurate, short range)
    // 2. Global SDF (fast, long range)

    float t = 0;
    const int maxSteps = 64;

    // Phase 1: Mesh SDF (up to ~180 units)
    while (t < min(maxDistance, MeshSDFTraceDistance))
    {
        float3 pos = origin + direction * t;

        // Sample mesh SDF
        float sdf = SampleMeshSDF(pos);

        if (sdf < SurfaceThreshold)
        {
            // Hit! Find which card
            FLumenCard card = FindLumenCard(pos);
            return CreateHit(card, pos, t);
        }

        // Step by SDF distance (sphere tracing)
        t += max(sdf, MinStepSize);
    }

    // Phase 2: Global SDF (remaining distance)
    while (t < maxDistance)
    {
        float3 pos = origin + direction * t;

        float sdf = SampleGlobalSDF(pos);

        if (sdf < SurfaceThreshold)
        {
            // Hit coarse geometry
            return CreateApproximateHit(pos, t);
        }

        t += sdf;
    }

    return CreateMiss();
}
```

**Hardware Ray Tracing (Optional):**
```cpp
// If r.Lumen.HardwareRayTracing = 1
FLumenRayHit TraceLumenRay_HWRT(float3 origin, float3 direction, float maxDistance)
{
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.01;
    ray.TMax = maxDistance;

    FLumenRayPayload payload;

    TraceRay(SceneAccelerationStructure,
             RAY_FLAG_NONE,
             0xFF,
             0, 0, 0,
             ray,
             payload);

    if (payload.HitT > 0)
    {
        // Hit lighting: Evaluate materials at hit point
        // OR Surface cache: Sample pre-cached lighting
        return CreateHit(payload);
    }

    return CreateMiss();
}
```

### Temporal Accumulation

Critical for noise reduction:

```hlsl
// Current frame (noisy)
float3 currentGI = TraceAndGatherGI(worldPos, normal);

// Reproject previous frame
float2 velocity = VelocityBuffer.Sample(screenUV).xy;
float2 prevUV = screenUV - velocity;

// Sample history
float3 previousGI = PreviousGIBuffer.Sample(prevUV).rgb;

// History validation
bool validHistory = true;
validHistory &= (abs(DepthBuffer.Sample(screenUV) - DepthHistory.Sample(prevUV)) < DepthThreshold);
validHistory &= (dot(NormalBuffer.Sample(screenUV), NormalHistory.Sample(prevUV)) > NormalThreshold);
validHistory &= all(prevUV >= 0.0 && prevUV <= 1.0);

// Temporal blend
float historyWeight = validHistory ? 0.9 : 0.0;

// Clamp history to reduce ghosting
float3 currentNeighborMin, currentNeighborMax;
ComputeNeighborMinMax(screenUV, currentNeighborMin, currentNeighborMax);
previousGI = clamp(previousGI, currentNeighborMin, currentNeighborMax);

float3 finalGI = lerp(currentGI, previousGI, historyWeight);
```

### Performance Scalability

```cpp
// Screen probe settings
r.Lumen.ScreenProbeGather.DownsampleFactor = 16  // 8, 16, 32
r.Lumen.ScreenProbeGather.TracingOctahedronResolution = 8 // 6, 8, 12

// Trace distance
r.Lumen.MaxTraceDistance = 20000  // cm
r.Lumen.TraceMeshSDFs.TraceDistance = 180.0

// Quality
r.Lumen.ScreenProbeGather.ScreenTraceMaxIterations = 64
r.Lumen.ScreenProbeGather.SpatialFilterHalfKernelSize = 1

// Hardware RT (expensive)
r.Lumen.HardwareRayTracing = 0  // 0=software, 1=hardware

// Async compute
r.Lumen.DiffuseIndirect.AsyncCompute = 1
```

---

## Lumen Reflections

Lumen also handles reflections (separate from GI, different tracing).

**Key File:** `Engine/Source/Runtime/Renderer/Private/Lumen/LumenReflections.cpp` (3,600+ lines)

### Reflection vs. Diffuse GI Differences

```cpp
// Diffuse GI:
// - Hemispherical gathering (wide cone)
// - Cosine-weighted importance
// - Lower resolution acceptable

// Reflections:
// - Narrow cone (based on roughness)
// - Mirror direction
// - Needs higher resolution for sharp reflections
```

### Roughness-Based Dispatch

```cpp
// Smooth surfaces: Full ray-traced reflections
// Rough surfaces: Use radiance cache (cheaper)

float MaxRoughnessForTracing = 0.35; // Configurable

if (Roughness < MaxRoughnessForTracing)
{
    // Full reflection ray tracing
    float3 reflection = TraceLumenReflection(worldPos, reflectDir, Roughness);
}
else
{
    // Sample radiance cache instead
    float3 reflection = SampleRadianceCache(worldPos, reflectDir);
}
```

### GGX Importance Sampling

```hlsl
// Sample reflection direction based on material roughness
float3 SampleReflectionDirection(float3 normal, float3 view, float roughness, float2 random)
{
    // GGX distribution sampling
    float a = roughness * roughness;
    float a2 = a * a;

    float phi = 2.0 * PI * random.x;
    float cosTheta = sqrt((1.0 - random.y) / (1.0 + (a2 - 1.0) * random.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // Spherical to Cartesian
    float3 H;
    H.x = sinTheta * cos(phi);
    H.y = sinTheta * sin(phi);
    H.z = cosTheta;

    // Transform to world space
    float3 up = abs(normal.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);

    H = tangent * H.x + bitangent * H.y + normal * H.z;

    // Reflect view around H
    float3 reflectDir = reflect(-view, H);

    return reflectDir;
}
```

### Reflection Denoiser

```cpp
// Four-stage denoiser for Lumen reflections:

// 1. Screen Space Reconstruction (spatial filter, BRDF-aware)
float3 ScreenSpaceReconstruction(float2 uv, float3 currentReflection)
{
    // 5-tap spatial filter
    // Weights based on BRDF similarity (roughness, normal)
    // Bilateral filtering
}

// 2. Temporal Filter
float3 TemporalFilter(float2 uv, float3 currentReflection, float3 previousReflection)
{
    // High temporal weight (up to 12 frames accumulation)
    // History rejection based on depth/normal/reflection direction changes
}

// 3. Bilateral Filter
float3 BilateralFilter(float2 uv, float3 reflection)
{
    // Edge-preserving blur
    // Kernel radius: ~8 pixels
    // Depth and normal weights
}

// 4. Firefly Suppression
float3 FireflySuppression(float3 reflection)
{
    // Clamp extreme values (bright reflections)
    // Tonemap-based range compression
    float MaxIntensity = 40.0; // Configurable
    return min(reflection, MaxIntensity);
}
```

---

## Implementation Strategy for Custom Engines

Building a full Lumen system takes years. Here's a practical roadmap:

### Phase 1: Foundation (Months 1-3)

**Start with the basics:**

1. **Reflection Probes** (Week 1-2)
   - Cubemap capture at editor time
   - Roughness-based mip selection
   - Simple blending (nearest probe or 2-3 blended)

2. **SSAO** (Week 3)
   - Basic hemisphere sampling
   - Bilateral blur
   - Good enough for most games

3. **SSR** (Week 4-6)
   - Linear ray marching
   - Basic PCF
   - Temporal accumulation
   - Tile-based optimization

**Result:** 80% of commercial games stop here. This gives you:
- Indoor reflections (SSR)
- Outdoor reflections (probes)
- Depth perception (SSAO)

### Phase 2: Intermediate (Months 4-6)

**Improve quality:**

4. **GTAO** (Week 7-8)
   - Better AO than SSAO
   - Async compute implementation
   - Upsampling for performance

5. **Better SSR** (Week 9-10)
   - HZB ray marching
   - Better filtering (BRDF-aware)
   - Improved temporal

6. **Probe Volumes** (Week 11-12)
   - 3D grid of probes (simplified radiance cache)
   - Manual placement or auto-generation
   - Trilinear interpolation
   - Good enough GI for many games

**Result:** AA/AAA quality reflections and basic GI.

### Phase 3: Advanced (Months 7-12)

**If you need dynamic GI:**

7. **Signed Distance Fields** (Month 7-8)
   - Mesh SDF generation
   - Global SDF building
   - Ray marching against SDFs
   - This is hard but essential for Lumen-like GI

8. **Screen Probe GI** (Month 9-10)
   - Probe placement (uniform + adaptive)
   - Ray tracing against SDFs
   - Octahedral storage
   - Interpolation to pixels

9. **Temporal Stability** (Month 11-12)
   - Robust history validation
   - Variance estimation
   - Adaptive sample counts
   - This makes or breaks the system

**Result:** Lumen-like dynamic GI.

### Phase 4: Optional (Year 2+)

10. **Hardware Ray Tracing**
    - DXR/Vulkan RT integration
    - BVH building
    - Hybrid software/hardware

11. **Surface Cache**
    - Card generation
    - Virtual texture atlases
    - Progressive streaming

12. **Radiance Cache**
    - Clipmap structure
    - Probe update heuristics
    - Parallax-corrected lookup

---

## bgfx Implementation Examples

### Reflection Probe System

```cpp
class ReflectionProbeSystem
{
    struct Probe
    {
        vec3 position;
        float influenceRadius;
        bgfx::TextureHandle cubemap; // PF_RGBA16F with mips
        mat4 boxTransform; // For box projection
        vec3 boxMin, boxMax;
    };

    std::vector<Probe> probes;

    bgfx::TextureHandle CaptureCubemap(vec3 position)
    {
        const int size = 128;
        const int numMips = 7;

        // Create cubemap
        bgfx::TextureHandle cubemap = bgfx::createTextureCube(
            size, false, numMips,
            bgfx::TextureFormat::RGBA16F,
            BGFX_TEXTURE_RT
        );

        // Render 6 faces
        vec3 dirs[6] = { {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1} };
        vec3 ups[6] = { {0,1,0}, {0,1,0}, {0,0,1}, {0,0,1}, {0,1,0}, {0,1,0} };

        for (int face = 0; face < 6; ++face)
        {
            // Setup view
            mat4 view = lookAt(position, position + dirs[face], ups[face]);
            mat4 proj = perspective(90.0f, 1.0f, 0.1f, 1000.0f);

            // Create FBO for this face
            bgfx::Attachment attachment;
            attachment.init(cubemap, bgfx::Access::Write, 0, 0, face);
            bgfx::FrameBufferHandle fbo = bgfx::createFrameBuffer(1, &attachment);

            // Render scene
            bgfx::setViewFrameBuffer(VIEW_PROBE_CAPTURE, fbo);
            bgfx::setViewTransform(VIEW_PROBE_CAPTURE, &view, &proj);
            RenderScene(VIEW_PROBE_CAPTURE);

            bgfx::destroy(fbo);
        }

        // Generate mips (importance-sampled for roughness)
        GenerateRoughnessMips(cubemap);

        return cubemap;
    }

    void Bind(uint8_t stage, bgfx::UniformHandle sampler)
    {
        // For now, bind single nearest probe
        // (Full implementation would blend multiple)
        bgfx::setTexture(stage, sampler, probes[0].cubemap);
    }
};
```

### Screen Space AO (Compute)

```cpp
// SSAO compute shader
class SSAORenderer
{
    bgfx::ProgramHandle ssaoCS;
    bgfx::TextureHandle aoTexture;
    bgfx::TextureHandle randomTexture;

    void Init(uint16_t width, uint16_t height)
    {
        // AO output texture
        aoTexture = bgfx::createTexture2D(
            width, height, false, 1,
            bgfx::TextureFormat::R8,
            BGFX_TEXTURE_COMPUTE_WRITE
        );

        // 4×4 random vectors
        randomTexture = bgfx::createTexture2D(
            4, 4, false, 1,
            bgfx::TextureFormat::RGBA8,
            BGFX_TEXTURE_NONE,
            GenerateRandomNoise()
        );

        // Load compute shader
        ssaoCS = bgfx::createProgram(
            bgfx::createShader(loadMemory("cs_ssao.bin")),
            true
        );
    }

    void Render(bgfx::TextureHandle depthBuffer, bgfx::TextureHandle normalBuffer)
    {
        // Bind inputs
        bgfx::setImage(0, depthBuffer, 0, bgfx::Access::Read);
        bgfx::setImage(1, normalBuffer, 0, bgfx::Access::Read);
        bgfx::setImage(2, randomTexture, 0, bgfx::Access::Read);
        bgfx::setImage(3, aoTexture, 0, bgfx::Access::Write);

        // Dispatch (8×8 thread groups)
        bgfx::dispatch(VIEW_SSAO,
                      ssaoCS,
                      (width + 7) / 8,
                      (height + 7) / 8,
                      1);
    }
};
```

**GLSL Compute Shader:**
```glsl
// cs_ssao.sc
layout(local_size_x = 8, local_size_y = 8) in;

IMAGE2D_RO(s_depth, r32f, 0);
IMAGE2D_RO(s_normal, rgba8, 1);
SAMPLER2D(s_random, 2);
IMAGE2D_WR(s_aoOutput, r8, 3);

uniform vec4 u_ssaoParams; // radius, bias, power, intensity

const int numSamples = 16;
const vec3 sampleKernel[16] = ...; // Precomputed hemisphere samples

void main()
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);

    float depth = imageLoad(s_depth, coord).r;
    vec3 normal = imageLoad(s_normal, coord).rgb * 2.0 - 1.0;

    // Reconstruct position
    vec3 worldPos = reconstructPosition(coord, depth);

    // Random rotation
    vec2 noiseUV = vec2(coord) / 4.0; // 4×4 tile
    vec3 randomVec = texture2D(s_random, noiseUV).rgb * 2.0 - 1.0;

    // TBN matrix
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    // Sample AO
    float occlusion = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        vec3 samplePos = worldPos + TBN * sampleKernel[i] * u_ssaoParams.x;

        // Project and sample depth
        vec4 offset = mul(u_viewProj, vec4(samplePos, 1.0));
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;

        float sampleDepth = imageLoad(s_depth, ivec2(offset.xy * imageSize(s_depth))).r;

        // Range check
        float rangeCheck = smoothstep(0.0, 1.0, u_ssaoParams.x / abs(worldPos.z - sampleDepth));

        occlusion += (sampleDepth >= depth + u_ssaoParams.y ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / float(numSamples));
    occlusion = pow(occlusion, u_ssaoParams.z); // Power

    imageStore(s_aoOutput, coord, vec4(occlusion));
}
```

### Simple SSR

```glsl
// fs_ssr.sc (Screen Space Reflections)
$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_sceneColor, 0);
SAMPLER2D(s_depth, 1);
SAMPLER2D(s_normal, 2);
SAMPLER2D(s_roughness, 3);

uniform vec4 u_ssrParams; // maxDistance, stepSize, numSteps, falloff

vec3 TraceSSR(vec3 worldPos, vec3 reflectDir)
{
    const int numSteps = int(u_ssrParams.z);

    vec3 rayPos = worldPos;
    float stepSize = u_ssrParams.y;

    for (int i = 0; i < numSteps; ++i)
    {
        rayPos += reflectDir * stepSize;

        // Project to screen
        vec4 screenPos = mul(u_viewProj, vec4(rayPos, 1.0));
        screenPos.xyz /= screenPos.w;
        vec2 screenUV = screenPos.xy * 0.5 + 0.5;

        // Out of bounds?
        if (any(lessThan(screenUV, vec2_splat(0.0))) ||
            any(greaterThan(screenUV, vec2_splat(1.0))))
            return vec3_splat(0.0);

        // Sample depth
        float sceneDepth = texture2D(s_depth, screenUV).r;
        float rayDepth = LinearizeDepth(screenPos.z);

        // Hit?
        if (rayDepth > sceneDepth)
        {
            // Fade at edges
            vec2 edgeFade = smoothstep(0.0, 0.1, screenUV) *
                            smoothstep(1.0, 0.9, screenUV);
            float fade = edgeFade.x * edgeFade.y;

            // Sample scene color
            vec3 reflection = texture2D(s_sceneColor, screenUV).rgb;

            return reflection * fade;
        }
    }

    return vec3_splat(0.0);
}

void main()
{
    vec2 uv = v_texcoord0;

    float depth = texture2D(s_depth, uv).r;
    vec3 worldPos = reconstructWorldPosition(uv, depth);

    vec3 normal = texture2D(s_normal, uv).rgb * 2.0 - 1.0;
    float roughness = texture2D(s_roughness, uv).r;

    // Skip rough surfaces
    if (roughness > 0.5)
    {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Reflection vector
    vec3 viewDir = normalize(worldPos - u_cameraPos);
    vec3 reflectDir = reflect(viewDir, normal);

    // Trace
    vec3 reflection = TraceSSR(worldPos, reflectDir);

    gl_FragColor = vec4(reflection, 1.0);
}
```

---

## Key Files Reference

### C++ Source Files

**Screen Space Reflections:**
- `Engine/Source/Runtime/Renderer/Private/ScreenSpaceReflectionTiles.cpp`
  - Tiled SSR optimization

**Reflection Probes:**
- `Engine/Source/Runtime/Renderer/Private/ReflectionEnvironmentCapture.cpp`
  - Probe capture and filtering
- `Engine/Source/Runtime/Renderer/Private/ReflectionEnvironment.cpp`
  - Runtime probe sampling and blending

**Planar Reflections:**
- `Engine/Source/Runtime/Renderer/Private/PlanarReflectionRendering.cpp`
  - Mirror view rendering

**Ambient Occlusion:**
- `Engine/Source/Runtime/Renderer/Private/CompositionLighting/PostProcessAmbientOcclusion.cpp`
  - SSAO and GTAO implementation

**Lumen GI:**
- `Engine/Source/Runtime/Renderer/Private/Lumen/LumenScreenProbeGather.cpp` (4,100+ lines)
  - Screen probe tracing and gathering
- `Engine/Source/Runtime/Renderer/Private/Lumen/LumenScene.cpp`
  - Surface cache (card system)
- `Engine/Source/Runtime/Renderer/Private/Lumen/LumenRadianceCache.cpp`
  - Radiance cache (clipmap probes)
- `Engine/Source/Runtime/Renderer/Private/Lumen/LumenDiffuseIndirect.cpp`
  - Diffuse GI integration

**Lumen Reflections:**
- `Engine/Source/Runtime/Renderer/Private/Lumen/LumenReflections.cpp` (3,600+ lines)
  - Reflection ray tracing
- `Engine/Source/Runtime/Renderer/Private/Lumen/LumenReflectionTracing.cpp`
  - Tracing backend

### Shader Files

**Reflections:**
- `Engine/Shaders/Private/ScreenSpaceReflectionTileCommons.ush`
  - SSR utilities
- `Engine/Shaders/Private/ReflectionEnvironmentShared.ush`
  - Probe sampling

**AO:**
- `Engine/Shaders/Private/PostProcessAmbientOcclusion.usf`
  - SSAO and GTAO shaders

**Lumen:**
- `Engine/Shaders/Private/Lumen/LumenReflections.usf`
  - Reflection tracing
- `Engine/Shaders/Private/Lumen/LumenScreenProbeGather.usf`
  - Probe gathering
- `Engine/Shaders/Private/Lumen/LumenRadianceCache.usf`
  - Radiance cache sampling

---

## Summary

**Recommended Implementation Order for Your Custom Engine:**

1. **Month 1:** Reflection probes + basic SSAO
2. **Month 2:** SSR with temporal accumulation
3. **Month 3:** GTAO (better AO)
4. **Month 4-6:** Probe volumes (basic GI)
5. **Month 7+:** Full Lumen (if you need dynamic GI)

**80/20 Rule:** Probes + SSR + GTAO gives you 80% of the visual quality for 20% of the implementation effort. Only pursue Lumen if your game absolutely needs dynamic GI (destructible environments, dynamic time of day, etc.).

UE5's reflection and GI systems represent the cutting edge of real-time rendering. Even implementing a subset of these techniques will put your engine in AAA territory.
