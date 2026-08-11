# Unreal Engine 5 Shadow Rendering Documentation
## For bgfx Implementation

This documentation explains how UE5's shadow rendering system works, from basic shadow mapping to advanced Virtual Shadow Maps.

---

## Table of Contents
1. [Shadow System Overview](#shadow-system-overview)
2. [Traditional Shadow Maps](#traditional-shadow-maps)
3. [Virtual Shadow Maps (VSM)](#virtual-shadow-maps-vsm)
4. [Shadow Map Filtering](#shadow-map-filtering)
5. [Contact Shadows](#contact-shadows)
6. [Ray-Traced Shadows](#ray-traced-shadows)
7. [Shadow Bias and Artifacts](#shadow-bias-and-artifacts)
8. [Performance Optimization](#performance-optimization)
9. [bgfx Implementation Guide](#bgfx-implementation-guide)
10. [Key Files Reference](#key-files-reference)

---

## Shadow System Overview

UE5 uses multiple shadowing techniques simultaneously:

```
Shadow System Hierarchy:
┌────────────────────────────────────────────────┐
│ Primary: Virtual Shadow Maps (UE5 flagship)   │
│ - Page-based virtual texturing                │
│ - 16k virtual resolution per light            │
│ - Dynamic caching for static geometry         │
└────────────────────────────────────────────────┘
                    ↓ (fallback)
┌────────────────────────────────────────────────┐
│ Traditional: Cascaded Shadow Maps (CSM)       │
│ - Multiple cascades for directional lights    │
│ - Cube maps for point lights                  │
│ - Single frustum for spot lights              │
└────────────────────────────────────────────────┘
                    ↓ (detail layer)
┌────────────────────────────────────────────────┐
│ Contact Shadows (Screen Space)                │
│ - Fine detail near surfaces                   │
│ - Ray marching in screen space                │
└────────────────────────────────────────────────┘
                    ↓ (optional, high-end)
┌────────────────────────────────────────────────┐
│ Ray-Traced Shadows (Hardware RT)              │
│ - Accurate soft shadows                       │
│ - Translucent shadows                         │
└────────────────────────────────────────────────┘
```

### Light Type Support

| Light Type | Traditional | VSM | Contact | Ray-Traced |
|------------|-------------|-----|---------|------------|
| Directional | CSM (2-4 cascades) | Clipmap | ✓ | ✓ |
| Spot | Single shadow map | Single page | ✓ | ✓ |
| Point | Cube map | 6 pages | ✓ | ✓ |
| Rect | Single shadow map | Single page | ✓ | ✓ |

**Key Files:**
- `Engine/Source/Runtime/Renderer/Private/ShadowRendering.cpp` (2,613 lines)
- `Engine/Source/Runtime/Renderer/Private/ShadowDepthRendering.cpp` (2,400 lines)

---

## Traditional Shadow Maps

Traditional shadow mapping is the baseline technique supported by all hardware.

### Cascaded Shadow Maps (CSM) for Directional Lights

**Key File:** `Engine/Source/Runtime/Renderer/Private/ShadowSetup.cpp`

#### Cascade Setup

```cpp
// UE5 CSM configuration
int32 NumCascades = 4; // Typically 2-4 cascades
float CascadeDistribution = 0.8; // Exponential distribution

// Cascade distances are calculated based on view frustum
float CascadeSplits[NumCascades + 1];
CascadeSplits[0] = NearPlane;
CascadeSplits[NumCascades] = FarPlane;

// Calculate intermediate splits (exponential + linear blend)
for (int i = 1; i < NumCascades; ++i)
{
    float fraction = (float)i / NumCascades;

    // Logarithmic split
    float logSplit = NearPlane * pow(FarPlane / NearPlane, fraction);

    // Linear split
    float linearSplit = NearPlane + (FarPlane - NearPlane) * fraction;

    // Blend between log and linear
    CascadeSplits[i] = lerp(linearSplit, logSplit, CascadeDistribution);
}
```

**Console Variables:**
```cpp
r.Shadow.CSM.MaxCascades = 4              // Number of cascades (1-10)
r.Shadow.DistanceScale = 1.0              // Scale all shadow distances
r.Shadow.CSMSplitPenumbraScale = 0.5      // Softness between cascades
r.Shadow.MaxResolution = 2048             // Per-cascade resolution
```

#### Shadow Map Rendering

```cpp
// For each cascade:
struct FShadowCascade
{
    FMatrix ViewMatrix;           // Look from light direction
    FMatrix ProjectionMatrix;     // Orthographic projection
    FBox CasterBounds;            // AABB of shadow casters
    float SplitNear, SplitFar;    // Depth range
    FVector2D Resolution;         // Shadow map size
};

// Rendering process:
void RenderCascadeShadowMap(FShadowCascade& Cascade)
{
    // 1. Setup view from light
    SetViewMatrix(Cascade.ViewMatrix);
    SetProjectionMatrix(Cascade.ProjectionMatrix);

    // 2. Cull shadow casters
    TArray<FPrimitiveSceneProxy*> Casters;
    CullShadowCasters(Cascade.CasterBounds, Casters);

    // 3. Render depth only
    for (auto Caster : Casters)
    {
        RenderDepthOnly(Caster);
    }
}
```

### Point Light Shadows (Cube Maps)

**Traditional Approach:**
```cpp
// Render 6 faces of cube map
enum ECubeFace
{
    Face_PosX, Face_NegX,
    Face_PosY, Face_NegY,
    Face_PosZ, Face_NegZ
};

// For each face:
FMatrix ViewMatrices[6];
ViewMatrices[Face_PosX] = LookAt(LightPos, LightPos + FVector(1,0,0), FVector(0,1,0));
ViewMatrices[Face_NegX] = LookAt(LightPos, LightPos + FVector(-1,0,0), FVector(0,1,0));
// ... etc

// Perspective projection (90° FOV)
FMatrix ProjectionMatrix = PerspectiveFov(PI/2, 1.0, NearPlane, FarPlane);

// Render all 6 faces to cube map
for (int face = 0; face < 6; ++face)
{
    SetRenderTarget(CubeMap, face);
    SetViewMatrix(ViewMatrices[face]);
    RenderDepthOnly(ShadowCasters);
}
```

**One-Pass Point Light Shadows (Optimized):**
```cpp
// Use geometry shader to render all 6 faces in one pass
// Geometry shader outputs to different cube faces

// Vertex shader
struct VS_Output
{
    float4 Position : SV_Position;
    float3 WorldPos : WORLDPOS;
};

// Geometry shader
[maxvertexcount(18)] // 6 faces × 3 vertices
void GS_PointShadow(triangle VS_Output input[3],
                    inout TriangleStream<GS_Output> outStream)
{
    for (int face = 0; face < 6; ++face)
    {
        GS_Output output;
        output.RTIndex = face; // Which cube face

        for (int v = 0; v < 3; ++v)
        {
            output.Position = mul(FaceViewProj[face], input[v].WorldPos);
            output.Depth = length(input[v].WorldPos - LightPosition);
            outStream.Append(output);
        }
        outStream.RestartStrip();
    }
}

// Pixel shader
float PS_PointShadow(GS_Output input) : SV_Depth
{
    // Write linear depth
    return input.Depth / FarPlane;
}
```

### Spot Light Shadows

Simplest case - single shadow map with perspective projection:

```cpp
// Setup view/projection
FVector LightPos = SpotLight.Position;
FVector LightDir = SpotLight.Direction;
FVector Up = abs(LightDir.Z) < 0.999 ? FVector(0,0,1) : FVector(1,0,0);

FMatrix ViewMatrix = LookAt(LightPos, LightPos + LightDir, Up);

float FOV = SpotLight.OuterConeAngle * 2.0;
FMatrix ProjectionMatrix = PerspectiveFov(FOV, 1.0, NearPlane, FarPlane);

// Render single shadow map
SetRenderTarget(ShadowMap);
RenderDepthOnly(ShadowCasters);
```

### Shadow Projection

**In lighting pixel shader:**
```hlsl
// Sample shadow map
float SampleShadowMap(float3 worldPos, Texture2D shadowMap, SamplerState shadowSampler,
                      float4x4 shadowViewProj)
{
    // Transform to light clip space
    float4 shadowPos = mul(shadowViewProj, float4(worldPos, 1.0));

    // Perspective divide
    shadowPos.xyz /= shadowPos.w;

    // Transform to [0, 1] texture space
    float2 shadowUV = shadowPos.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y; // Flip Y

    // Sample depth
    float shadowDepth = shadowMap.Sample(shadowSampler, shadowUV).r;

    // Compare
    float currentDepth = shadowPos.z;
    float shadow = currentDepth > shadowDepth + Bias ? 0.0 : 1.0;

    return shadow;
}
```

**For CSM (multiple cascades):**
```hlsl
float SampleCSM(float3 worldPos, float viewDepth)
{
    // Determine which cascade
    int cascadeIndex = 0;
    for (int i = 0; i < NumCascades; ++i)
    {
        if (viewDepth < CascadeSplits[i + 1])
        {
            cascadeIndex = i;
            break;
        }
    }

    // Sample that cascade's shadow map
    return SampleShadowMap(worldPos,
                          CascadeShadowMaps[cascadeIndex],
                          ShadowSampler,
                          CascadeViewProj[cascadeIndex]);
}
```

**Key File:** `Engine/Shaders/Private/ShadowProjectionCommon.ush`

---

## Virtual Shadow Maps (VSM)

UE5's flagship shadow technology - dramatically improves quality and performance for complex scenes.

**Key Files:**
- `Engine/Source/Runtime/Renderer/Private/VirtualShadowMaps/VirtualShadowMapArray.cpp` (5,342 lines)
- `Engine/Shaders/Shared/VirtualShadowMapDefinitions.h`

### Core Concept

Instead of fixed-resolution shadow maps, VSM uses:
- **Virtual Address Space**: 16k × 16k per light (128×128 pages of 128×128 pixels each)
- **Physical Page Pool**: Dynamically allocated based on visibility
- **Caching**: Static geometry cached between frames
- **Mip Levels**: 8 levels for LOD

```
Virtual Shadow Map (16,384 × 16,384)
┌─────────────────────────────────────┐
│ ┌─┬─┬─┬─┐                          │
│ ├─┼─┼─┼─┤ Each square = 128×128 px │
│ ├─┼─┼─┼─┤ (a "page")               │
│ └─┴─┴─┴─┘                          │
│                                     │
│  Only visible pages are allocated  │
│  and rendered                       │
└─────────────────────────────────────┘
                ↓
Physical Page Pool (e.g., 2048 pages)
┌────┬────┬────┬────┬────┐
│Page│Page│Page│Page│... │ Only allocated pages
├────┼────┼────┼────┼────┤ stored in memory
│ 0  │ 1  │ 2  │ 3  │    │
└────┴────┴────┴────┴────┘
```

### VSM Constants

```cpp
// From VirtualShadowMapDefinitions.h

#define VSM_PAGE_SIZE 128                // Each page is 128x128 pixels
#define VSM_LEVEL0_DIM_PAGES_XY 128      // 128x128 pages at level 0
#define VSM_VIRTUAL_MAX_RESOLUTION_XY (VSM_PAGE_SIZE * VSM_LEVEL0_DIM_PAGES_XY) // 16,384
#define VSM_MAX_MIP_LEVELS 8             // 8 mip levels

// Physical pool
r.Shadow.Virtual.MaxPhysicalPages = 2048 // Max pages in memory
```

### Page Table

```cpp
// Virtual → Physical mapping
struct FVirtualShadowMapPageTable
{
    // For each virtual page (128×128 array):
    //   0xFFFFFFFF = not allocated
    //   otherwise = physical page index
    TArray<uint32> PageTable; // 128*128 = 16,384 entries per mip

    // Hierarchical mip levels (for faster lookups)
    TArray<uint32> MipTables[VSM_MAX_MIP_LEVELS];
};
```

### VSM Rendering Pipeline

```cpp
// 1. Mark pages based on screen pixels
void MarkRequiredPages(FRDGBuilder& GraphBuilder)
{
    // For each pixel on screen:
    //   Project to virtual shadow map
    //   Mark the page as needed
    //   Mark coarser mip pages for smooth transitions

    // Uses compute shader
    AddPass_MarkPixelPages<<<...>>>();
}

// 2. Allocate physical pages
void AllocatePages(FRDGBuilder& GraphBuilder)
{
    // For each marked page:
    //   Check cache (if static geometry)
    //   If not cached, allocate new physical page
    //   Update page table

    // LRU eviction if pool is full
}

// 3. Render shadow depth into allocated pages
void RenderVirtualShadowDepth(FRDGBuilder& GraphBuilder)
{
    // For each allocated page:
    //   Setup viewport (128×128 sub-region)
    //   Render only geometry visible to that page
    //   Write to physical page location in atlas

    // Nanite integration: GPU-driven culling per page
    RenderNaniteShadowDepth(AllocatedPages);
    RenderNonNaniteShadowDepth(AllocatedPages);
}

// 4. Build HZB (Hierarchical Z-Buffer) for occlusion culling
void BuildHZB(FRDGBuilder& GraphBuilder)
{
    // Create mip chain of depth buffer
    // Used for next frame's culling
}

// 5. Project and sample during lighting
float SampleVirtualShadowMap(float3 worldPos)
{
    // Project to virtual shadow space
    float2 virtualUV = ProjectToVirtualShadow(worldPos);

    // Determine mip level (based on screen coverage)
    float mipLevel = CalculateMipLevel(virtualUV);

    // Look up page table
    uint pageIndex = PageTable.Sample(virtualUV, mipLevel);

    if (pageIndex == 0xFFFFFFFF)
        return 1.0; // Not shadowed (page not allocated)

    // Convert to physical UV
    float2 physicalUV = VirtualToPhysicalUV(virtualUV, pageIndex);

    // Sample physical shadow map
    return ShadowAtlas.Sample(physicalUV);
}
```

### VSM Caching

```cpp
// Static geometry caching
struct FVirtualShadowMapCacheEntry
{
    uint32 PhysicalPageIndex;
    uint32 LastRequestFrame;
    bool bStatic;
};

// Cache invalidation
void InvalidateMovedPrimitives(TArray<FPrimitiveSceneProxy*> MovedPrimitives)
{
    for (auto Primitive : MovedPrimitives)
    {
        // Mark all pages overlapping this primitive as invalid
        TArray<uint32> AffectedPages = FindAffectedPages(Primitive->Bounds);

        for (uint32 pageIndex : AffectedPages)
        {
            Cache[pageIndex].bStatic = false;
            Cache[pageIndex].LastRequestFrame = 0;
        }
    }
}

// Benefits:
// - Static geometry rendered once, cached indefinitely
// - Only dynamic geometry re-rendered each frame
// - 10-100x performance improvement for static scenes
```

### VSM for Different Light Types

**Directional Light (Sun):**
```cpp
// Uses clipmap structure (centered on camera)
struct FVirtualShadowMapClipmap
{
    int32 NumLevels = 8; // 8 clipmap levels

    // Each level is a 128×128 page grid centered on camera
    // Level 0: Highest detail, small area
    // Level 7: Lowest detail, large area

    float LevelRadii[NumLevels]; // Computed based on view distance
};
```

**Spot/Point Lights:**
```cpp
// Single virtual shadow map (or 6 for point lights)
// Uses full 128×128 page grid
// Pages allocated based on screen coverage
```

**Console Variables:**
```cpp
r.Shadow.Virtual = 1                     // Enable VSM
r.Shadow.Virtual.Cache = 1               // Enable caching
r.Shadow.Virtual.MaxPhysicalPages = 2048 // Memory budget
r.Shadow.Virtual.ResolutionLodBiasDirectional = 0.0 // Quality bias
r.Shadow.Virtual.ResolutionLodBiasLocal = 0.0       // For local lights
```

---

## Shadow Map Filtering

Shadow map filtering reduces aliasing and creates soft shadows.

**Key File:** `Engine/Shaders/Private/ShadowFilteringCommon.ush`

### PCF (Percentage Closer Filtering)

Basic technique - sample shadow map multiple times and average results.

```hlsl
// 1×1 PCF (4 samples with bilinear)
float PCF_1x1(Texture2D shadowMap, SamplerState shadowSampler,
              float2 uv, float compareDepth)
{
    // Hardware PCF (built into sampler)
    return shadowMap.SampleCmpLevelZero(shadowSampler, uv, compareDepth);
}

// 3×3 PCF (16 samples using Gather4)
float PCF_3x3(Texture2D shadowMap, SamplerState shadowSampler,
              float2 uv, float compareDepth, float2 texelSize)
{
    float shadow = 0.0;

    // Use Gather4 for 2×2 texel samples
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            float2 offset = float2(x, y) * texelSize;
            float4 depths = shadowMap.Gather(shadowSampler, uv + offset);

            // Compare each of the 4 samples
            shadow += (depths.x > compareDepth) ? 0.25 : 0.0;
            shadow += (depths.y > compareDepth) ? 0.25 : 0.0;
            shadow += (depths.z > compareDepth) ? 0.25 : 0.0;
            shadow += (depths.w > compareDepth) ? 0.25 : 0.0;
        }
    }

    return shadow / 9.0;
}

// 5×5 PCF (36 samples)
float PCF_5x5(Texture2D shadowMap, SamplerState shadowSampler,
              float2 uv, float compareDepth, float2 texelSize)
{
    // Similar but with -2 to +2 range
    // Uses Gather4 for efficiency
}
```

**Quality Levels:**
```cpp
r.ShadowQuality = 5  // 0-5 (0=off, 5=max)

// Maps to:
// 0: No filtering (hard shadows)
// 1: 1×1 PCF (bilinear)
// 2: 3×3 PCF (16 samples)
// 3: 5×5 PCF (36 samples)
// 4+: 5×5 PCF + optimizations
```

### PCSS (Percentage Closer Soft Shadows)

Physically-based soft shadows that vary based on occluder distance.

**Key File:** `Engine/Shaders/Private/ShadowPercentageCloserFiltering.ush`

```hlsl
// Two-phase algorithm:

// Phase 1: Blocker search
float FindBlockerDistance(Texture2D shadowMap, float2 uv,
                          float receiverDepth, float searchRadius)
{
    const int numSamples = 16; // Poisson disk samples

    float blockerSum = 0.0;
    float blockerCount = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        float2 offset = PoissonDisk[i] * searchRadius;
        float shadowDepth = shadowMap.Sample(shadowSampler, uv + offset).r;

        if (shadowDepth < receiverDepth)
        {
            blockerSum += shadowDepth;
            blockerCount += 1.0;
        }
    }

    if (blockerCount == 0.0)
        return -1.0; // No blockers, fully lit

    return blockerSum / blockerCount; // Average blocker depth
}

// Phase 2: Penumbra estimation and PCF
float PCSS(Texture2D shadowMap, float2 uv, float receiverDepth)
{
    // Find average blocker distance
    float blockerDistance = FindBlockerDistance(shadowMap, uv, receiverDepth, searchWidth);

    if (blockerDistance < 0.0)
        return 1.0; // Fully lit

    // Estimate penumbra size based on geometry
    // penumbra = (receiver - blocker) / blocker * lightSize
    float penumbra = (receiverDepth - blockerDistance) / blockerDistance * lightSize;

    // PCF with variable kernel size
    float shadow = PCF_Variable(shadowMap, uv, receiverDepth, penumbra);

    return shadow;
}
```

**Parameters:**
```cpp
r.Shadow.FilterMethod = 1                // 0=PCF, 1=PCSS
r.Shadow.MaxSoftKernelSize = 40          // Max kernel radius (pixels)
```

### SMRT (Shadow Map Ray Tracing) - VSM Only

Novel filtering technique for Virtual Shadow Maps.

**Key File:** `Engine/Shaders/Private/VirtualShadowMaps/VirtualShadowMapSMRTCommon.ush`

```hlsl
// Traces rays through virtual shadow map hierarchy
struct FSMRTSettings
{
    int RayCount;              // Rays per pixel (4-16)
    int SamplesPerRay;         // Samples along each ray (8-16)
    float TexelDitherScale;    // Noise reduction
    float ExtrapolateMaxSlope; // Early termination
};

float SMRT_TraceShadow(float3 worldPos, float3 normal, float3 lightDir)
{
    float shadow = 0.0;

    // Cast multiple rays for soft shadows
    for (int ray = 0; ray < RayCount; ++ray)
    {
        // Jitter ray direction (for soft shadows)
        float3 rayDir = JitterRayDirection(lightDir, ray);

        // March ray through VSM
        float rayOcclusion = 0.0;
        float3 rayPos = worldPos + normal * SurfaceBias;

        for (int step = 0; step < SamplesPerRay; ++step)
        {
            // Sample VSM at this position
            float vsmDepth = SampleVirtualShadowMap(rayPos);

            if (vsmDepth < GetDepth(rayPos))
            {
                rayOcclusion = 1.0;
                break; // Hit occluder
            }

            rayPos += rayDir * StepSize;
        }

        shadow += rayOcclusion;
    }

    return 1.0 - (shadow / RayCount);
}
```

**Benefits:**
- Adaptive sample count (extrapolation in fully lit/shadowed regions)
- Better soft shadows than PCF
- Leverages VSM's mip hierarchy

**Console Variables:**
```cpp
r.Shadow.Virtual.SMRT.RayCount = 8           // Rays per pixel
r.Shadow.Virtual.SMRT.SamplesPerRay = 12     // Samples per ray
r.Shadow.Virtual.SMRT.TexelDitherScale = 2.0 // Dithering
```

---

## Contact Shadows

Screen-space shadows for fine detail near surfaces.

**Key File:** `Engine/Source/Runtime/Renderer/Private/Shadows/ScreenSpaceShadows.cpp`

### Why Contact Shadows?

Shadow maps miss fine detail (e.g., character feet on ground). Contact shadows add this detail cheaply.

### Ray Marching Approach

```hlsl
// Compute shader (one thread per pixel)
float ContactShadow(float3 worldPos, float3 lightDir, float maxDistance)
{
    const int numSteps = 8;
    const float stepScale = 2.0; // Exponential steps

    float3 rayPos = worldPos;
    float stepSize = maxDistance / numSteps;

    for (int i = 0; i < numSteps; ++i)
    {
        rayPos += lightDir * stepSize;

        // Project to screen space
        float4 screenPos = mul(ViewProjection, float4(rayPos, 1.0));
        screenPos.xyz /= screenPos.w;
        float2 screenUV = screenPos.xy * 0.5 + 0.5;

        // Sample depth buffer
        float sceneDepth = DepthBuffer.SampleLevel(PointSampler, screenUV, 0).r;
        float rayDepth = screenPos.z;

        // Check if ray is behind surface
        if (rayDepth > sceneDepth)
        {
            return 0.0; // In shadow
        }

        // Exponential step size increase
        stepSize *= stepScale;
    }

    return 1.0; // Not in shadow
}
```

### Stochastic Jittering (UE5 Default)

```cpp
// Parameters:
GLumenScreenProbeTracingOctahedronResolution = 8;  // Ray directions
float Dither = InterleavedGradientNoise(PixelPos);  // Noise per pixel

// Jitter start position to reduce banding
float3 startPos = worldPos + lightDir * (Dither * ContactShadowLength / numSteps);
```

### Bend Screen-Space Shadows (Wave Algorithm)

More advanced, better performance on modern GPUs:

```hlsl
// Uses GPU waves (64 threads)
// Threads collaborate to share ray marching work

[numthreads(8, 8, 1)]
void ContactShadowCS_Wave(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    // 60 samples per pixel using wave intrinsics
    // Threads in wave share intermediate results

    float shadow = 0.0;
    const int samplesPerThread = 60 / WaveGetLaneCount();

    for (int i = 0; i < samplesPerThread; ++i)
    {
        float3 samplePos = ComputeSamplePosition(i);

        // Sample and share via wave intrinsics
        float depth = SampleDepth(samplePos);
        bool occluded = WaveActiveAnyTrue(depth > samplePos.z);

        if (occluded)
        {
            shadow = 0.0;
            break;
        }
    }

    OutShadow[dispatchThreadId.xy] = shadow;
}
```

**Console Variables:**
```cpp
r.ContactShadows = 1                      // Enable contact shadows
r.ContactShadows.OverrideLength = -1      // Override length (-1 = per-light)
r.ContactShadows.Standalone.Method = 0    // 0=Jitter, 1=Bend
```

---

## Ray-Traced Shadows

Hardware ray tracing for maximum quality (DXR/Vulkan RT).

**Key File:** `Engine/Source/Runtime/Renderer/Private/RayTracing/RayTracingShadows.cpp`

### Ray Tracing Shadow Pipeline

```hlsl
// Ray generation shader
[shader("raygeneration")]
void RayTracedShadowsRGS()
{
    uint2 pixelPos = DispatchRaysIndex().xy;

    // Reconstruct world position from GBuffer
    float depth = DepthTexture[pixelPos].r;
    float3 worldPos = ReconstructWorldPosition(pixelPos, depth);

    // Get normal from GBuffer
    float3 normal = NormalTexture[pixelPos].xyz;

    // Ray direction (to light)
    float3 rayDir = normalize(LightPosition - worldPos);

    // Offset start position to avoid self-intersection
    float3 rayOrigin = worldPos + normal * NormalBias;

    // Trace shadow ray
    RayDesc ray;
    ray.Origin = rayOrigin;
    ray.Direction = rayDir;
    ray.TMin = 0.001;
    ray.TMax = LightDistance;

    ShadowPayload payload;
    payload.Shadowed = false;

    TraceRay(SceneAccelerationStructure,
             RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,  // Optimize for shadows
             0xFF, // Instance mask
             0,    // Ray contribution to hit group index
             0,    // Multiplier for geometry contribution
             0,    // Miss shader index
             ray,
             payload);

    // Write result
    float shadow = payload.Shadowed ? 0.0 : 1.0;
    OutShadowMask[pixelPos] = shadow;
}

// Miss shader (ray didn't hit anything = lit)
[shader("miss")]
void RayTracedShadowsMS(inout ShadowPayload payload)
{
    payload.Shadowed = false;
}

// Any-hit shader (for alpha-tested materials)
[shader("anyhit")]
void RayTracedShadowsAHS(inout ShadowPayload payload, BuiltInTriangleIntersectionAttributes attr)
{
    // Sample material opacity
    float opacity = SampleMaterialOpacity(attr);

    if (opacity < 0.5)
        IgnoreHit(); // Continue ray
    else
        AcceptHitAndEndSearch(); // Shadow ray hit
}

// Closest-hit shader
[shader("closesthit")]
void RayTracedShadowsCHS(inout ShadowPayload payload, BuiltInTriangleIntersectionAttributes attr)
{
    payload.Shadowed = true;
}
```

### Soft Shadows with Ray Tracing

```hlsl
// Multiple rays for area light soft shadows
const int NumShadowRays = 16;

float shadow = 0.0;
for (int i = 0; i < NumShadowRays; ++i)
{
    // Sample random point on light area
    float2 lightUV = Hammersley(i, NumShadowRays); // Low-discrepancy sequence
    float3 lightPoint = SampleAreaLight(lightUV);

    // Trace ray to that point
    float3 rayDir = normalize(lightPoint - worldPos);
    float rayDist = length(lightPoint - worldPos);

    // Trace...
    bool hit = TraceRay(...);

    shadow += hit ? 0.0 : 1.0;
}

shadow /= NumShadowRays;
```

**Console Variables:**
```cpp
r.RayTracing.Shadows = 1                  // Enable RT shadows
r.RayTracing.Shadows.SamplesPerPixel = 1  // Rays per pixel
r.RayTracing.NormalBias = 0.1             // Surface offset
```

---

## Shadow Bias and Artifacts

Shadow artifacts are caused by precision issues. Bias mitigates them.

### Types of Bias

**1. Depth Bias (Constant):**
```cpp
// Pushes shadow depth away from surface
float BiasedDepth = ShadowDepth + DepthBias;
```

**2. Slope-Scale Depth Bias:**
```cpp
// Scales with surface slope
float slope = length(ddx(depth), ddy(depth));
float BiasedDepth = ShadowDepth + DepthBias + slope * SlopeScaleBias;
```

**3. Normal Bias:**
```cpp
// Offset ray origin along normal
float3 BiasedPos = WorldPos + Normal * NormalBias;
```

**4. Receiver Bias:**
```cpp
// Applied during shadow projection (UE5 specific)
float CompareDepth = ShadowSpaceDepth - ReceiverBias;
```

### Bias Configuration per Light Type

```cpp
// Directional/CSM
r.Shadow.CSMDepthBias = 10.0              // Constant bias
r.Shadow.CSMSlopeScaleDepthBias = 3.0     // Slope bias
r.Shadow.CSMReceiverBias = 0.9            // Receiver bias (0-1)

// Spot lights
r.Shadow.SpotLightDepthBias = 3.0
r.Shadow.SpotLightSlopeScaleDepthBias = 3.0
r.Shadow.SpotLightReceiverBias = 0.5

// Point lights
r.Shadow.PointLightDepthBias = 0.02       // Much smaller (in [0,1] space)
r.Shadow.PointLightSlopeScaleDepthBias = 3.0

// Rect lights
r.Shadow.RectLightDepthBias = 0.025
r.Shadow.RectLightReceiverBias = 0.3
```

### Common Shadow Artifacts

| Artifact | Cause | Solution |
|----------|-------|----------|
| **Shadow Acne** | Self-shadowing from precision | Increase depth bias |
| **Peter Panning** | Object appears detached from shadow | Decrease bias, use normal offset |
| **Aliasing** | Insufficient shadow map resolution | Increase resolution, use filtering |
| **Cascade Transition** | Visible seams between CSM cascades | Blend cascades, match bias |
| **Light Bleeding** | VSM/ESM artifact | Use traditional shadow maps |

### Transition Scale

```cpp
r.Shadow.TransitionScale = 60.0  // Higher = sharper, more artifacts

// Controls soft transition at shadow edges
float shadowFade = saturate((CompareDepth - BiasedDepth) * TransitionScale);
```

---

## Performance Optimization

### Culling Optimizations

**1. CPU Culling:**
```cpp
r.Shadow.UseOctreeForCulling = 1  // Spatial acceleration structure
r.Shadow.RadiusThreshold = 0.01   // Cull small shadow casters

// Frustum culling per cascade/light
TArray<FPrimitiveSceneProxy*> CullShadowCasters(FConvexVolume ShadowFrustum)
{
    TArray<FPrimitiveSceneProxy*> Casters;

    // Test each primitive against shadow frustum
    for (auto Primitive : Scene->Primitives)
    {
        if (ShadowFrustum.IntersectBox(Primitive->Bounds))
        {
            Casters.Add(Primitive);
        }
    }

    return Casters;
}
```

**2. GPU Culling (VSM):**
```cpp
// HZB (Hierarchical Z-Buffer) occlusion culling
// Cull objects occluded in previous frame's depth

r.Shadow.Virtual.UseHZB = 1  // Enable HZB culling

// Test object bounds against HZB
bool IsOccluded = TestBoundsAgainstHZB(ObjectBounds, PreviousFrameHZB);
```

### Caching Strategies

**VSM Caching:**
```cpp
r.Shadow.Virtual.Cache = 1                   // Enable caching
r.Shadow.Virtual.Cache.StaticSeparate = 1    // Separate static/dynamic pools

// Cache invalidation strategies:
// 1. Per-primitive tracking (precise, expensive)
// 2. Spatial hashing (fast, conservative)
// 3. Dirty regions (balance)
```

**Traditional Shadow Caching:**
```cpp
r.Shadow.CacheWholeSceneShadows = 1   // Cache static directional shadows
r.Shadow.WholeSceneShadowCacheMb = 150 // Memory budget
```

### Resolution and LOD

**Adaptive Resolution (VSM):**
```cpp
r.Shadow.Virtual.ResolutionLodBiasDirectional = 0.0  // Quality bias (-2 to +2)
r.Shadow.Virtual.ResolutionLodBiasLocal = 0.0        // For spot/point lights

// Mip selection based on screen coverage
// Distant objects get coarser shadow pages
```

**Traditional Resolution:**
```cpp
r.Shadow.TexelsPerPixel = 1.27324           // Shadow texels per screen pixel
r.Shadow.MinResolution = 32                 // Minimum shadow map size
r.Shadow.MaxResolution = 2048               // Maximum size
r.Shadow.FadeResolution = 64                // Start fading out
```

### Parallel Execution

```cpp
r.ParallelGatherShadowPrimitives = 1  // Multi-threaded culling
r.ParallelInitDynamicShadows = 1      // Parallel shadow setup
```

### Stencil and Depth Bounds Optimization

```cpp
r.Shadow.StencilOptimization = 1      // Avoid stencil buffer clears
r.Shadow.CSMDepthBoundsTest = 1       // Use depth bounds test (faster than stencil)
```

### One-Pass Projection (VSM)

```cpp
r.Shadow.Virtual.OnePassProjection.MaxLightsPerPixel = 16

// Projects multiple lights in single pass
// Reduces draw calls for scenes with many lights
```

---

## bgfx Implementation Guide

### Basic Shadow Map Setup

```cpp
class ShadowMapRenderer
{
    bgfx::FrameBufferHandle shadowMapFB;
    bgfx::TextureHandle shadowMapDepth;
    bgfx::ProgramHandle shadowDepthProgram;

    void Init(uint16_t resolution)
    {
        // Create depth texture
        shadowMapDepth = bgfx::createTexture2D(
            resolution, resolution,
            false, 1,
            bgfx::TextureFormat::D24S8,
            BGFX_TEXTURE_RT | BGFX_SAMPLER_COMPARE_LEQUAL
        );

        // Create framebuffer
        bgfx::Attachment attachment;
        attachment.init(shadowMapDepth);
        shadowMapFB = bgfx::createFrameBuffer(1, &attachment, false);

        // Load shadow depth shader
        shadowDepthProgram = loadProgram("vs_shadow_depth", "fs_shadow_depth");
    }

    void RenderShadowMap(const Light& light, const std::vector<Mesh>& objects)
    {
        // Calculate light view/projection
        float viewMtx[16], projMtx[16];
        CalculateLightMatrices(light, viewMtx, projMtx);

        // Set as render target
        bgfx::setViewFrameBuffer(VIEW_SHADOW, shadowMapFB);
        bgfx::setViewRect(VIEW_SHADOW, 0, 0, resolution, resolution);
        bgfx::setViewTransform(VIEW_SHADOW, viewMtx, projMtx);
        bgfx::setViewClear(VIEW_SHADOW, BGFX_CLEAR_DEPTH, 0, 1.0f, 0);

        // Render depth only
        for (const auto& mesh : objects)
        {
            bgfx::setTransform(mesh.worldMatrix);
            bgfx::setVertexBuffer(0, mesh.vb);
            bgfx::setIndexBuffer(mesh.ib);

            uint64_t state = BGFX_STATE_WRITE_Z
                           | BGFX_STATE_DEPTH_TEST_LESS
                           | BGFX_STATE_CULL_CCW;

            bgfx::setState(state);
            bgfx::submit(VIEW_SHADOW, shadowDepthProgram);
        }
    }
};
```

### Shadow Sampling in Lighting Pass

```cpp
// Create sampler with comparison
bgfx::UniformHandle s_shadowMap = bgfx::createUniform(
    "s_shadowMap",
    bgfx::UniformType::Sampler,
    1
);

// In lighting pass
bgfx::setTexture(4, s_shadowMap, shadowMapDepth,
                BGFX_SAMPLER_COMPARE_LEQUAL);  // Hardware PCF
```

**GLSL Shader:**
```glsl
// fs_lighting.sc
SAMPLER2DSHADOW(s_shadowMap, 4);  // Comparison sampler

uniform mat4 u_lightViewProj;

void main()
{
    // Reconstruct world position
    vec3 worldPos = ...;

    // Transform to light clip space
    vec4 shadowPos = mul(u_lightViewProj, vec4(worldPos, 1.0));
    shadowPos.xyz /= shadowPos.w;

    // Transform to [0,1]
    vec3 shadowUV;
    shadowUV.xy = shadowPos.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y;
    shadowUV.z = shadowPos.z;

    // Sample with hardware PCF
    float shadow = shadow2D(s_shadowMap, shadowUV);

    // Apply shadow to lighting
    vec3 lighting = directLighting * shadow;

    gl_FragColor = vec4(lighting, 1.0);
}
```

### Cascaded Shadow Maps with bgfx

```cpp
class CSMRenderer
{
    static const int NUM_CASCADES = 4;

    bgfx::FrameBufferHandle cascadeFB[NUM_CASCADES];
    bgfx::TextureHandle cascadeTextures[NUM_CASCADES];

    struct Cascade
    {
        float viewMtx[16];
        float projMtx[16];
        float splitDistance;
    };
    Cascade cascades[NUM_CASCADES];

    void RenderCSM(const Camera& camera, const Light& light, const Scene& scene)
    {
        // Calculate cascade splits
        CalculateCascadeSplits(camera, cascades);

        for (int i = 0; i < NUM_CASCADES; ++i)
        {
            // Calculate light matrices for this cascade
            CalculateCascadeMatrices(camera, light, cascades[i]);

            // Render to cascade shadow map
            bgfx::setViewFrameBuffer(VIEW_SHADOW + i, cascadeFB[i]);
            bgfx::setViewTransform(VIEW_SHADOW + i,
                                  cascades[i].viewMtx,
                                  cascades[i].projMtx);

            // Cull and render
            RenderDepthOnly(scene, cascades[i].frustum);
        }
    }
};
```

### Contact Shadows with Compute

```cpp
// bgfx compute shader for contact shadows
bgfx::ProgramHandle contactShadowCS = bgfx::createProgram(
    bgfx::createShader(loadMemory("cs_contact_shadow.bin")),
    true
);

// Dispatch compute
bgfx::setImage(0, s_depthBuffer, 0, bgfx::Access::Read);
bgfx::setImage(1, s_shadowOutput, 0, bgfx::Access::Write);
bgfx::setUniform(u_lightDirection, &lightDir);

bgfx::dispatch(VIEW_COMPUTE,
              contactShadowCS,
              (width + 7) / 8,
              (height + 7) / 8,
              1);
```

---

## Key Files Reference

### C++ Source Files

**Core Shadow Rendering:**
- `Engine/Source/Runtime/Renderer/Private/ShadowRendering.cpp` (2,613 lines)
  - Main shadow projection and rendering
- `Engine/Source/Runtime/Renderer/Private/ShadowDepthRendering.cpp` (2,400 lines)
  - Shadow depth pass
- `Engine/Source/Runtime/Renderer/Private/ShadowSetup.cpp`
  - Shadow frustum calculation and cascade setup

**Virtual Shadow Maps:**
- `Engine/Source/Runtime/Renderer/Private/VirtualShadowMaps/VirtualShadowMapArray.cpp` (5,342 lines)
  - VSM system implementation
- `Engine/Source/Runtime/Renderer/Private/VirtualShadowMaps/VirtualShadowMapCache.cpp`
  - Caching system

**Contact and Ray-Traced Shadows:**
- `Engine/Source/Runtime/Renderer/Private/Shadows/ScreenSpaceShadows.cpp`
  - Contact shadows
- `Engine/Source/Runtime/Renderer/Private/RayTracing/RayTracingShadows.cpp`
  - Hardware ray-traced shadows

### Shader Files

**Shadow Filtering:**
- `Engine/Shaders/Private/ShadowFilteringCommon.ush`
  - PCF implementations
- `Engine/Shaders/Private/ShadowPercentageCloserFiltering.ush`
  - PCSS implementation

**Virtual Shadow Maps:**
- `Engine/Shaders/Shared/VirtualShadowMapDefinitions.h`
  - VSM constants
- `Engine/Shaders/Private/VirtualShadowMaps/VirtualShadowMapSMRTCommon.ush`
  - SMRT filtering

**Shadow Projection:**
- `Engine/Shaders/Private/ShadowProjectionCommon.ush`
  - Shadow sampling utilities
- `Engine/Shaders/Private/ShadowProjectionPixelShader.usf`
  - Shadow projection shaders

---

## Summary

**For Your bgfx Engine:**

1. **Start Simple**: Implement basic shadow maps (directional CSM, spot single map)
2. **Add Filtering**: PCF is essential, PCSS is great for quality
3. **Contact Shadows**: Cheap detail boost, implement via compute shader
4. **Optimization**: Culling, caching, resolution management
5. **Advanced (Optional)**: VSM is complex but provides best quality/performance for large scenes

**Implementation Priority:**
1. Basic shadow maps (week 1-2)
2. PCF filtering (week 2)
3. CSM for directional lights (week 3)
4. Contact shadows (week 4)
5. PCSS (week 5-6)
6. VSM (months, optional)

UE5's shadow system is production-proven across hundreds of shipped titles. Following this architecture will give you robust, high-quality shadows.
