# Unreal Engine 5 Lighting System Documentation
## Complete Guide for Game Engine Developers

This documentation explains how UE5's lighting system works - from light types to deferred rendering and light culling.

---

## Table of Contents
1. [Lighting System Overview](#lighting-system-overview)
2. [Light Types](#light-types)
3. [Light Parameters](#light-parameters)
4. [Light Mobility](#light-mobility)
5. [Deferred Lighting Pipeline](#deferred-lighting-pipeline)
6. [Light Culling (Clustered Deferred)](#light-culling-clustered-deferred)
7. [Forward Lighting](#forward-lighting)
8. [bgfx Implementation Guide](#bgfx-implementation-guide)
9. [Key Files Reference](#key-files-reference)

---

## Lighting System Overview

UE5 uses a **hybrid lighting system** that combines:

```
Lighting Architecture:
┌────────────────────────────────────────────┐
│ 1. Direct Lighting                         │
│    - Directional (Sun)                     │
│    - Point (Light bulbs, street lights)    │
│    - Spot (Flashlights, car headlights)    │
│    - Rect (Area lights, windows)           │
└────────────────────────────────────────────┘
              ↓
┌────────────────────────────────────────────┐
│ 2. Indirect Lighting                       │
│    - Sky Light (Environment)               │
│    - Reflection Probes                     │
│    - Lumen GI (Dynamic)                    │
│    - Lightmaps (Baked)                     │
└────────────────────────────────────────────┘
              ↓
┌────────────────────────────────────────────┐
│ 3. Rendering Path                          │
│    - Deferred (Most lights)                │
│    - Forward (Translucent, mobile)         │
│    - Clustered Deferred (Many lights)      │
└────────────────────────────────────────────┘
```

**Key File:** `Engine/Source/Runtime/Engine/Public/SceneTypes.h`

```cpp
enum ELightComponentType
{
    LightType_Directional,  // Parallel rays (sun, moon)
    LightType_Point,        // Omnidirectional (bulb)
    LightType_Spot,         // Cone-shaped (flashlight)
    LightType_Rect,         // Area light (window)
    LightType_MAX
};
```

---

## Light Types

### 1. Directional Light (Sun, Moon)

**Key File:** `Engine/Source/Runtime/Engine/Classes/Components/DirectionalLightComponent.h`

**Characteristics:**
- **Infinite distance**: All rays are parallel
- **No attenuation**: Same intensity everywhere
- **Covers entire scene**: Always evaluated for every pixel
- **Typical use**: Sun, moon, ambient global lighting

```cpp
class UDirectionalLightComponent : public ULightComponent
{
    // Angular diameter of light source (affects shadow softness)
    float LightSourceAngle;          // Default: 0.5357° (matches sun)
    float LightSourceSoftAngle;      // Additional soft shadow angle

    // Shadow parameters
    int32 DynamicShadowCascades;     // Number of CSM cascades (1-4)
    float CascadeDistributionExponent; // 0.0-1.0, controls cascade spacing
    float CascadeTransitionFraction;   // 0.0-1.0, smooth blending

    // Advanced features
    bool bAtmosphereSunLight;        // Integrate with sky atmosphere
    int32 AtmosphereSunLightIndex;   // Which sun (0 or 1)
    bool bCastCloudShadows;          // Cast shadows from volumetric clouds
    float CloudShadowStrength;       // Cloud shadow opacity
    float CloudShadowOnAtmosphereStrength;
    float CloudShadowOnSurfaceStrength;
};
```

**Shadow System:**
- Uses Cascaded Shadow Maps (CSM)
- Typically 2-4 cascades
- Each cascade covers progressively larger area with lower resolution

**Console Variables:**
```cpp
r.Shadow.CSM.MaxCascades = 4
r.Shadow.DistanceScale = 1.0
r.Shadow.CSMSplitPenumbraScale = 0.5
r.Shadow.MaxResolution = 2048
```

### 2. Point Light (Light Bulbs, Street Lights, Lamps)

**Key File:** `Engine/Source/Runtime/Engine/Classes/Components/PointLightComponent.h`

**Characteristics:**
- **Omnidirectional**: Light radiates in all directions
- **Inverse-square falloff**: Physically accurate attenuation
- **Finite radius**: `AttenuationRadius` defines hard cutoff
- **Typical use**: Indoor lights, street lamps, fire, explosions

```cpp
class UPointLightComponent : public ULocalLightComponent
{
    // Attenuation
    bool bUseInverseSquaredFalloff;   // Physically accurate vs artistic
    float LightFalloffExponent;       // When not inverse-squared (2-16)

    // Soft shadows (source size)
    float SourceRadius;               // Physical light source size (cm)
    float SoftSourceRadius;           // Additional softening
    float SourceLength;               // For capsule-shaped lights
};
```

**Attenuation Formula:**

**Inverse-Squared (Physical):**
```cpp
float GetDistanceAttenuation(float Distance, float AttenuationRadius)
{
    float NormalizedDistance = saturate(Distance / AttenuationRadius);
    float Attenuation = 1.0 / (Distance * Distance + 1.0);

    // Smooth cutoff at radius
    float WindowFunction = Square(1.0 - Square(NormalizedDistance));

    return Attenuation * WindowFunction;
}
```

**Exponential (Artistic):**
```cpp
float GetDistanceAttenuation(float Distance, float AttenuationRadius, float Exponent)
{
    float NormalizedDistance = saturate(Distance / AttenuationRadius);
    float BaseFalloff = saturate(1.0 - pow(NormalizedDistance, 4.0));

    return pow(BaseFalloff, Exponent);
}
```

**Shadow System:**
- Cubemap (6 faces) for omnidirectional shadows
- Can use single-pass geometry shader for efficiency
- Virtual Shadow Maps (VSM) support with 6 pages

### 3. Spot Light (Flashlights, Car Headlights, Stage Lights)

**Key File:** `Engine/Source/Runtime/Engine/Classes/Components/SpotLightComponent.h`

**Characteristics:**
- **Cone-shaped**: Directional with angular falloff
- **Inherits from Point**: All point light features + cone
- **Two angles**: Inner cone (full brightness) and outer cone (falloff)
- **Typical use**: Flashlights, headlights, street lamps, spotlights

```cpp
class USpotLightComponent : public UPointLightComponent
{
    float InnerConeAngle;   // Full brightness angle (degrees)
    float OuterConeAngle;   // Cutoff angle (degrees)
};
```

**Cone Attenuation:**
```cpp
float GetSpotAngleAttenuation(float3 LightDirection, float3 ToPixel,
                              float InnerCos, float OuterCos)
{
    float CosAngle = dot(normalize(ToPixel), LightDirection);

    // Smooth transition from outer to inner cone
    float AngleAttenuation = saturate((CosAngle - OuterCos) / (InnerCos - OuterCos));

    return Square(AngleAttenuation); // Squared for smoother falloff
}
```

**Shadow System:**
- Single shadow map (perspective projection)
- Matches light cone frustum
- VSM support with single page

### 4. Rect Light (Area Lights, Windows, Panels)

**Key File:** `Engine/Source/Runtime/Engine/Classes/Components/RectLightComponent.h`

**Characteristics:**
- **True area light**: Rectangular emissive surface
- **Accurate soft shadows**: Based on actual light size
- **Barn doors**: Light shaping attachments
- **Texture projection**: Can project images
- **Typical use**: Softboxes, windows, LED panels, TV screens

```cpp
class URectLightComponent : public ULocalLightComponent
{
    float SourceWidth;              // Width in cm
    float SourceHeight;             // Height in cm

    // Barn doors (light shaping)
    float BarnDoorAngle;            // Barn door angle (0-90°)
    float BarnDoorLength;           // Barn door length (cm)

    // Texture projection
    UTexture* SourceTexture;        // Emissive texture
};
```

**Area Light Integration:**
```hlsl
// Simplified rect light evaluation
float3 RectLightBRDF(float3 N, float3 V, float3 Points[4], float roughness)
{
    // Representative point on rect (approximation)
    float3 L = ClosestPointOnRect(Points, V);
    float3 H = normalize(V + L);

    // Solid angle calculation
    float solidAngle = RectSolidAngle(Points, worldPos);

    // Standard BRDF with area normalization
    float D = D_GGX(roughness, saturate(dot(N, H)));
    float Vis = Vis_SmithJointApprox(roughness, NoV, NoL);
    float3 F = F_Schlick(specularColor, VoH);

    return (D * Vis * F) * solidAngle;
}
```

**Shadow System:**
- Single shadow map (perspective or orthographic)
- Soft shadows from accurate area calculation
- Contact shadows for detail

### 5. Sky Light (Environment Lighting)

**Key File:** `Engine/Source/Runtime/Engine/Classes/Components/SkyLightComponent.h`

**Characteristics:**
- **Hemispherical**: Surrounds entire scene
- **Image-based**: Uses cubemap (HDRI or captured)
- **Distant lighting**: Treated as infinitely far
- **Typical use**: Sky, ambient environment, IBL

```cpp
class USkyLightComponent : public ULightComponentBase
{
    // Source configuration
    ESkyLightSourceType SourceType;  // Captured scene or specified cubemap
    bool bRealTimeCapture;           // Update each frame?
    UTextureCube* Cubemap;           // Source cubemap (if specified)

    // Capture settings
    int32 CubemapResolution;         // Power of 2 (128-2048)
    float SkyDistanceThreshold;      // Capture distance cutoff (cm)
    float SourceCubemapAngle;        // Rotation (0-360°)

    // Hemisphere control
    bool bLowerHemisphereIsBlack;    // Solid ground below?
    FLinearColor LowerHemisphereColor;

    // Advanced
    float OcclusionMaxDistance;      // AO distance (200-1500 cm)
    float Contrast;                  // AO contrast (0-1)
    float MinOcclusion;              // Min AO value (0-1)
};
```

**Storage:**
```cpp
// Spherical harmonics for diffuse
FSHVectorRGB3 IrradianceEnvironmentMap;

// Prefiltered cubemap for specular
FTextureCubeRHIRef ProcessedSkyTexture;
```

**Sampling in Shader:**
```hlsl
// Diffuse contribution
float3 DiffuseSkyLight(float3 normal)
{
    // Sample SH for fast diffuse lookup
    return SampleSH(IrradianceSH, normal);
}

// Specular contribution
float3 SpecularSkyLight(float3 reflectDir, float roughness)
{
    // Sample prefiltered cubemap
    float mipLevel = RoughnessToMip(roughness);
    return SkyLightCubemap.SampleLevel(samplerLinear, reflectDir, mipLevel).rgb;
}
```

---

## Light Parameters

### Intensity and Units

**Key File:** `Engine/Source/Runtime/Engine/Classes/Engine/Scene.h`

```cpp
enum class ELightUnits : uint8
{
    Unitless,    // Legacy, arbitrary scale
    Candelas,    // Luminous intensity (normalized)
    Lumens,      // Luminous power (normalized)
    EV,          // Exposure value at ISO 100 (normalized)
    Nits         // Luminance (non-normalized, depends on source size)
};
```

**Conversion to Luminous Power:**

```cpp
// From LightComponent.cpp
float GetLuminousIntensity() const
{
    switch (IntensityUnits)
    {
        case ELightUnits::Candelas:
            return Intensity;

        case ELightUnits::Lumens:
            // Point light: Lumens to candelas
            return Intensity / (4.0 * PI);

        case ELightUnits::Unitless:
            return Intensity * 16.0; // Legacy conversion

        case ELightUnits::EV:
            return pow(2.0, Intensity - 3.0);

        case ELightUnits::Nits:
            // Depends on source area
            return Intensity * SourceArea * PI;
    }
}
```

**Typical Values:**
```
Candle: 1 candela
60W Incandescent Bulb: ~850 lumens (68 candelas for point)
100W Incandescent Bulb: ~1700 lumens (135 candelas)
Flashlight: 100-1000 lumens
Car Headlight: 1000-2000 lumens
Sunlight: ~120,000 lux (EV ~15)
```

### Color Temperature

**Blackbody Radiation:**

```cpp
// From LightComponent.h
float Temperature;           // Kelvin (1700-12000)
bool bUseTemperature;        // Enable/disable

// Preset temperatures:
// - Candle flame: 1850K (orange)
// - Tungsten bulb: 2700K (warm yellow)
// - Halogen: 3200K (yellow-white)
// - Noon sunlight: 5500K (white)
// - D65 white point: 6500K (standard white)
// - Overcast sky: 7000K (cool white)
// - Blue sky: 10000K (blue)
```

**Temperature to RGB Conversion:**
```cpp
FLinearColor GetColorTemperature(float Temp)
{
    // Approximation of blackbody radiation
    float u = (0.860117757f + 1.54118254e-4f * Temp + 1.28641212e-7f * Temp * Temp) /
              (1.0f + 8.42420235e-4f * Temp + 7.08145163e-7f * Temp * Temp);

    float v = (0.317398726f + 4.22806245e-5f * Temp + 4.20481691e-8f * Temp * Temp) /
              (1.0f - 2.89741816e-5f * Temp + 1.61456053e-7f * Temp * Temp);

    // Convert CIE uv to RGB (simplified)
    return UVToLinearRGB(u, v);
}
```

### IES Light Profiles

**Real-world photometric data:**

**Key File:** `Engine/Source/Runtime/Engine/Classes/Engine/TextureLightProfile.h`

```cpp
class UTextureLightProfile : public UTexture2D
{
    float Brightness;        // Multiplier
    float TextureMultiplier; // Additional scale
};
```

**Usage in Shader:**
```hlsl
// IES profile stored as 1D or 2D texture
Texture2D IESTexture;
SamplerState IESSampler;

float GetIESAttenuation(float3 worldPos, float3 lightPos, float3 lightDir)
{
    float3 toLightDir = normalize(worldPos - lightPos);

    // Horizontal angle (azimuth)
    float phi = atan2(toLightDir.y, toLightDir.x);

    // Vertical angle (elevation)
    float theta = acos(dot(toLightDir, lightDir));

    // Sample IES texture
    float2 uv = float2(theta / PI, (phi + PI) / (2.0 * PI));
    float iesValue = IESTexture.SampleLevel(IESSampler, uv, 0).r;

    return iesValue * IESBrightnessScale;
}
```

**Common uses:**
- Street lights (specific beam patterns)
- Stage/theatrical lighting
- Automotive headlights
- Architectural lighting

### Attenuation Radius and Falloff

```cpp
// From LocalLightComponent.h
float AttenuationRadius;      // Hard cutoff distance (cm)
float LightFalloffExponent;   // Falloff curve (when not inverse-squared)
```

**Radius Estimation:**
```cpp
// Auto-calculate radius from intensity (when radius is 0)
float GetDefaultLightRadius() const
{
    const float MinLightAttenuation = 0.01;  // 1% threshold

    if (bUseInverseSquaredFalloff)
    {
        // Inverse-squared: I / (d^2) = threshold
        return sqrt(Intensity / MinLightAttenuation);
    }
    else
    {
        // Exponential: estimate based on exponent
        return Intensity * SomeArtisticScale;
    }
}
```

### Source Size (Soft Shadows)

**Point/Spot Lights:**
```cpp
float SourceRadius;        // Physical light source size (cm)
float SoftSourceRadius;    // Additional artistic softening (cm)
float SourceLength;        // For capsule-shaped lights (cm)
```

**Directional Lights:**
```cpp
float LightSourceAngle;      // Angular diameter (degrees)
float LightSourceSoftAngle;  // Additional soft angle (degrees)

// Sun: 0.5357° (0.93° with atmosphere)
// Moon: 0.52°
```

**Effect on Shadows:**
- Larger source → softer shadows (wider penumbra)
- Used in ray-traced shadows for cone angle
- Used in PCSS for kernel size
- VSM uses this for shadow softness

---

## Light Mobility

**Key File:** `Engine/Source/Runtime/Engine/Classes/Components/SceneComponent.h`

```cpp
enum class EComponentMobility : uint8
{
    Static,      // Baked at build time
    Stationary,  // Hybrid (baked indirect + dynamic direct)
    Movable      // Fully dynamic
};
```

### Static Lights

**Characteristics:**
- **Completely baked** into lightmaps
- **Zero runtime cost** (texture reads only)
- **Best quality**: Unlimited bounces, full GI
- **Cannot change**: Position, color, intensity locked
- **Build required**: Must rebuild lighting

**Use cases:**
- Architectural lighting in static buildings
- Environmental lighting (sun in static scenes)
- Any light that never changes

**Lightmap Resolution:**
```cpp
// Per-static-mesh setting
int32 OverriddenLightMapRes;  // Lightmap texels per unit

// Project-wide
r.LightMap.DefaultLightMapRes = 64
```

### Stationary Lights

**Hybrid approach** - most complex:

**What's Baked:**
- **Indirect lighting**: All bounced light in lightmaps
- **Static shadows**: Shadows from static geometry

**What's Dynamic:**
- **Direct lighting**: Calculated per-frame (allows intensity/color changes)
- **Dynamic shadows**: Shadows from movable objects
- **Specular**: Real-time specular highlights

**Key Limitation:**
```cpp
// Maximum 4 overlapping stationary lights per pixel
// Uses 4 shadow map channels
int32 ShadowMapChannel;  // 0-3, or -1 if no static shadows
```

**Channel Assignment:**
```cpp
// From LightComponent.h
uint32 bHasStaticLighting : 1;
uint32 bHasStaticShadowing : 1;
int32 PreviewShadowMapChannel;  // Editor visualization
```

**Console Variables:**
```cpp
r.AllowStaticLighting = 1
r.Shadow.Virtual.Cache = 1  // Cache static shadows in VSM
```

**Use cases:**
- Sun in mostly-static scenes with dynamic characters
- Indoor lights where only direct light changes
- Most common for outdoor environments

### Movable Lights

**Fully dynamic:**
- **Everything runtime**: Lighting and shadows computed per-frame
- **Can move/change**: Position, color, intensity, radius
- **Higher cost**: Full shadow rendering every frame
- **No baking**: Instant iteration

**Shadow Options:**
```cpp
bool bCastDynamicShadow;     // Enable shadow rendering
bool bCastStaticShadow;      // Ignored for movable (always false)
bool bAffectTranslucentLighting;
```

**Use cases:**
- Flashlights, muzzle flashes
- Vehicle headlights
- Destructible environments
- Any light that moves or changes

**Performance:**
```cpp
// Rough cost estimate (depends on scene complexity):
// - Directional: 2-5 ms (CSM rendering)
// - Spot: 0.5-2 ms (single shadow map)
// - Point: 1-4 ms (cubemap)
// - Rect: 1-3 ms (area light)
```

---

## Deferred Lighting Pipeline

UE5's primary rendering path for most lights.

**Key Files:**
- `Engine/Source/Runtime/Renderer/Private/DeferredShadingRenderer.cpp`
- `Engine/Source/Runtime/Renderer/Private/LightRendering.cpp`
- `Engine/Shaders/Private/DeferredLightPixelShaders.usf`

### Pipeline Overview

```
1. GBuffer Pass
   ↓
[GBuffer Textures: Normal, BaseColor, Roughness, Metallic, etc.]
   ↓
2. Lighting Accumulation Pass (for each light)
   ↓
[Scene Color += Light Contribution]
   ↓
3. Post Processing
```

### GBuffer Layout

**From:** `Engine/Shaders/Private/DeferredShadingCommon.ush`

```hlsl
struct FGBufferData
{
    // Geometric
    float3 WorldNormal;
    float3 WorldTangent;
    float Depth;

    // Material PBR properties
    float3 BaseColor;
    float3 DiffuseColor;    // Derived: BaseColor * (1 - Metallic)
    float3 SpecularColor;   // Derived: lerp(0.04, BaseColor, Metallic)
    float Metallic;
    float Specular;
    float Roughness;
    float Anisotropy;

    // Lighting modifiers
    float GBufferAO;

    // Shading model
    uint ShadingModelID;

    // Custom data (shading model specific)
    float4 CustomData;

    // Baked lighting
    float4 PrecomputedShadowFactors;  // 4 shadow channels
};
```

**Render Targets:**
```
RT0: SceneColor (accumulation target)
RT1: GBufferA - WorldNormal (RGB), PerObjectData (A)
RT2: GBufferB - Metallic (R), Specular (G), Roughness (B), ShadingModelID (A)
RT3: GBufferC - BaseColor (RGB), AO (A)
RT4: GBufferD - CustomData (shading model specific)
RT5: GBufferE - PrecomputedShadowFactors (RGBA)
RT6: GBufferF - WorldTangent (RGB), Anisotropy (A)
```

### Deferred Light Shader

**From:** `Engine/Shaders/Private/DeferredLightPixelShaders.usf`

```hlsl
void DeferredLightPixelMain(
    float4 SVPos : SV_POSITION,
    out float4 OutColor : SV_Target0)
{
    // 1. Calculate screen UV
    float2 ScreenUV = SvPositionToScreenUV(SVPos);

    // 2. Read GBuffer
    FGBufferData GBuffer = GetGBufferData(ScreenUV);

    // 3. Reconstruct world position from depth
    float SceneDepth = CalcSceneDepth(ScreenUV);
    float3 WorldPosition = ScreenToWorld(ScreenUV, SceneDepth);

    // 4. Get light data (from uniform buffer)
    FDeferredLightData LightData = InitDeferredLightFromUniforms();

    // 5. Calculate light attenuation (distance + angle)
    float LightMask = GetLocalLightAttenuation(
        WorldPosition,
        LightData,
        LightData.Normal,  // For rect lights
        LightData.Direction // For spot lights
    );

    if (LightMask <= 0.0)
    {
        OutColor = 0;
        return;  // Early out if outside light range
    }

    // 6. Get shadow attenuation
    float4 LightAttenuation = GetLightAttenuation(ScreenUV);

    // 7. Calculate camera vector
    float3 CameraVector = normalize(View.WorldCameraOrigin - WorldPosition);

    // 8. Evaluate BRDF lighting
    FDeferredLightingSplit Lighting = GetDynamicLighting(
        WorldPosition,
        CameraVector,
        GBuffer,
        1.0,  // AO
        GBuffer.ShadingModelID,
        LightData,
        LightAttenuation,
        uint2(SVPos.xy)
    );

    // 9. Output (additive blend)
    OutColor = float4(Lighting.SpecularLighting + Lighting.DiffuseLighting, 0);
}
```

### BRDF Evaluation

**From:** `Engine/Shaders/Private/DeferredLightingCommon.ush`

```hlsl
FDeferredLightingSplit GetDynamicLighting(...)
{
    float3 L = LightData.Direction;  // Light direction
    float3 V = CameraVector;         // View direction
    float3 N = GBuffer.WorldNormal;
    float3 H = normalize(V + L);

    float NoL = saturate(dot(N, L));
    float NoV = saturate(dot(N, V));
    float VoH = saturate(dot(V, H));
    float NoH = saturate(dot(N, H));

    // Shadow term
    float Shadow = LightAttenuation.r;

    // Diffuse
    float3 Diffuse = Diffuse_Burley(GBuffer.DiffuseColor, GBuffer.Roughness, NoV, NoL, VoH);

    // Specular (GGX)
    float D = D_GGX(GBuffer.Roughness, NoH);
    float Vis = Vis_SmithJointApprox(GBuffer.Roughness, NoV, NoL);
    float3 F = F_Schlick(GBuffer.SpecularColor, VoH);

    float3 Specular = (D * Vis) * F;

    // Apply light color and shadow
    Lighting.DiffuseLighting = Diffuse * LightData.Color * NoL * Shadow;
    Lighting.SpecularLighting = Specular * LightData.Color * NoL * Shadow;

    return Lighting;
}
```

### Blend State (Additive)

Lights accumulate using additive blending:

```cpp
// From LightRendering.cpp
TStaticBlendState<
    CW_RGBA,        // Write all channels
    BO_Add,         // RGB: Add
    BF_One,         // Src factor: 1
    BF_One,         // Dst factor: 1
    BO_Add,         // Alpha: Add
    BF_One,         // Src alpha factor: 1
    BF_One          // Dst alpha factor: 1
>::GetRHI()

// Result: SceneColor += LightContribution
```

### Light Volume Rendering

For point and spot lights, render light bounds geometry:

```cpp
// Point light: Render sphere
// Spot light: Render cone

// Depth test configuration:
// - If camera inside light volume: Disable depth test
// - If camera outside: Use depth test (cull pixels behind geometry)

uint64 DepthState = bCameraInsideLightGeometry ?
    BGFX_STATE_DEPTH_TEST_ALWAYS : BGFX_STATE_DEPTH_TEST_GREATER;
```

---

## Light Culling (Clustered Deferred)

For scenes with many lights (100+), use clustered deferred shading.

**Key Files:**
- `Engine/Source/Runtime/Renderer/Private/ClusteredDeferredShadingPass.cpp`
- `Engine/Source/Runtime/Renderer/Private/LightGridInjection.cpp`
- `Engine/Shaders/Private/ClusteredDeferredShadingPixelShader.usf`

### 3D Grid Structure

```cpp
// Grid parameters
int32 GridPixelSize = 64;    // Tile size (64×64 pixels)
int32 GridSizeZ = 32;        // Depth slices

// Total cells
int32 GridSizeX = (ScreenWidth + GridPixelSize - 1) / GridPixelSize;
int32 GridSizeY = (ScreenHeight + GridPixelSize - 1) / GridPixelSize;
int32 TotalCells = GridSizeX * GridSizeY * GridSizeZ;
```

### Light Grid Injection

**Build light grid (compute shader):**

```hlsl
// cs_light_grid_injection.hlsl
[numthreads(4, 4, 4)]  // Process 4x4x4 cells per thread group
void BuildLightGridCS(uint3 GroupId : SV_GroupID, uint3 ThreadId : SV_GroupThreadID)
{
    uint3 GridCoord = GroupId * 4 + ThreadId;

    if (any(GridCoord >= GridDimensions))
        return;

    // Calculate cell bounds in world space
    float3 MinBounds, MaxBounds;
    GetCellBounds(GridCoord, MinBounds, MaxBounds);

    // Test each light
    uint NumLightsInCell = 0;
    uint LightIndices[MAX_LIGHTS_PER_CELL];

    for (uint lightIndex = 0; lightIndex < NumLights; ++lightIndex)
    {
        FLightData light = Lights[lightIndex];

        // Test light bounds vs cell bounds
        if (IntersectLightWithCell(light, MinBounds, MaxBounds))
        {
            LightIndices[NumLightsInCell++] = lightIndex;

            if (NumLightsInCell >= MAX_LIGHTS_PER_CELL)
                break;  // Cell full
        }
    }

    // Write to light grid
    uint cellIndex = GridCoordToIndex(GridCoord);
    LightGrid[cellIndex].NumLights = NumLightsInCell;
    LightGrid[cellIndex].LightIndexStart = AtomicAdd(GlobalLightIndexCounter, NumLightsInCell);

    // Write light indices
    for (uint i = 0; i < NumLightsInCell; ++i)
    {
        uint writeIndex = LightGrid[cellIndex].LightIndexStart + i;
        GlobalLightIndexList[writeIndex] = LightIndices[i];
    }
}
```

### Clustered Shading Pass

**Single fullscreen pass evaluates all lights:**

```hlsl
// fs_clustered_deferred.hlsl
void ClusteredDeferredPS(
    float4 SvPosition : SV_Position,
    out float4 OutColor : SV_Target0)
{
    float2 ScreenUV = SvPositionToScreenUV(SvPosition);

    // Read GBuffer
    FGBufferData GBuffer = GetGBufferData(ScreenUV);
    float3 WorldPos = ReconstructWorldPosition(ScreenUV, GBuffer.Depth);
    float3 V = normalize(CameraPos - WorldPos);

    // Find grid cell
    uint3 GridCoord = WorldToGridCoord(WorldPos, SvPosition.xy);
    uint CellIndex = GridCoordToIndex(GridCoord);

    // Get lights in this cell
    uint NumLights = LightGrid[CellIndex].NumLights;
    uint LightIndexStart = LightGrid[CellIndex].LightIndexStart;

    // Accumulate lighting
    float3 AccumulatedLighting = 0;

    for (uint i = 0; i < NumLights; ++i)
    {
        uint lightIndex = GlobalLightIndexList[LightIndexStart + i];
        FLightData light = Lights[lightIndex];

        // Evaluate light
        float3 L = normalize(light.Position - WorldPos);
        float attenuation = CalculateAttenuation(WorldPos, light);

        if (attenuation > 0.0)
        {
            // BRDF evaluation
            float3 lighting = EvaluateBRDF(GBuffer, V, L) * attenuation * light.Color;
            AccumulatedLighting += lighting;
        }
    }

    OutColor = float4(AccumulatedLighting, 1.0);
}
```

### Performance Benefits

**Traditional deferred:**
- O(Lights × Pixels) - Each light = separate pass
- Example: 100 lights × 1920×1080 = ~200M pixel shades

**Clustered deferred:**
- O(Pixels + Lights × Cells) - Single pass
- Example: 1920×1080 + 100 lights × (30×17×32) = ~2M + 1.6M = 3.6M

**Savings:** ~98% reduction in overdraw

**Console Variables:**
```cpp
r.LightCulling.Quality = 1             // 0=off, 1=on
r.LightGridPixelSize = 64              // Tile size
r.LightGridSizeZ = 32                  // Z slices
r.LightGridMaxCulledLights = 256       // Max lights per cell
```

---

## Forward Lighting

Alternative rendering path, used for translucency and mobile.

**Key File:** `Engine/Shaders/Private/ForwardLightingCommon.ush`

### When Forward is Used

1. **Translucent materials** (always forward)
2. **Mobile rendering** (primary path)
3. **VR forward renderer** (optional)
4. **Hair/fur** (forward+ with visibility buffer)

### Forward Lighting Evaluation

```hlsl
// From ForwardLightingCommon.ush
FDeferredLightingSplit GetForwardDirectLighting(
    uint GridIndex,
    float3 WorldPosition,
    FGBufferData GBuffer,
    ...)
{
    FDeferredLightingSplit DirectLighting = (FDeferredLightingSplit)0;

    // 1. Directional light (sun)
    const FDirectionalLightData DirectionalLight = GetDirectionalLightData(0);
    {
        float3 L = DirectionalLight.Direction;
        float Shadow = GetDirectionalLightShadow(ScreenUV);

        DirectLighting += EvaluateLight(GBuffer, V, L, DirectionalLight.Color, Shadow);
    }

    // 2. Local lights (from light grid)
    const FCulledLightsGridData GridData = GetCulledLightsGrid(GridIndex);

    for (uint LocalLightIndex = 0; LocalLightIndex < GridData.NumLocalLights; ++LocalLightIndex)
    {
        uint LightIndex = ForwardLightData.CulledLightDataGrid[GridData.DataStartIndex + LocalLightIndex];
        FLocalLightData LocalLight = GetLocalLightData(LightIndex);

        float3 L = normalize(LocalLight.Position - WorldPosition);
        float Attenuation = CalculateAttenuation(WorldPosition, LocalLight);
        float Shadow = GetLocalLightShadow(ScreenUV, LightIndex);

        DirectLighting += EvaluateLight(GBuffer, V, L, LocalLight.Color * Attenuation, Shadow);
    }

    return DirectLighting;
}
```

### Forward vs Deferred Comparison

| Feature | Deferred | Forward |
|---------|----------|---------|
| **MSAA Support** | No (GBuffer incompatible) | Yes |
| **Translucency** | Separate pass | Native |
| **Memory** | High (GBuffer) | Lower |
| **Light Count** | Excellent (many lights) | Good (moderate lights) |
| **Material Variations** | Single shader | Many variants |
| **Bandwidth** | High (GBuffer reads/writes) | Lower |

---

## bgfx Implementation Guide

### Basic Deferred Lighting

```cpp
class DeferredLightRenderer
{
    // Light uniform buffer
    struct LightUniform
    {
        vec4 position;       // xyz = position, w = radius
        vec4 color;          // rgb = color, a = intensity
        vec4 direction;      // xyz = direction (spot/directional)
        vec4 params;         // x = inner cone, y = outer cone, z = falloff exp
    };

    std::vector<LightUniform> lights;
    bgfx::UniformHandle u_lightData;
    bgfx::ProgramHandle directionalLightShader;
    bgfx::ProgramHandle pointLightShader;
    bgfx::ProgramHandle spotLightShader;

    void RenderLights(const GBuffer& gbuffer)
    {
        // Bind GBuffer textures
        bgfx::setTexture(0, s_gbufferNormal, gbuffer.normalTexture);
        bgfx::setTexture(1, s_gbufferBaseColor, gbuffer.baseColorTexture);
        bgfx::setTexture(2, s_gbufferMaterial, gbuffer.materialTexture);
        bgfx::setTexture(3, s_gbufferDepth, gbuffer.depthTexture);

        // Additive blend state
        uint64_t state = BGFX_STATE_WRITE_RGB
                       | BGFX_STATE_WRITE_A
                       | BGFX_STATE_BLEND_ADD
                       | BGFX_STATE_DEPTH_TEST_EQUAL;  // Don't write depth

        bgfx::setState(state);

        // Render each light
        for (const auto& light : lights)
        {
            bgfx::setUniform(u_lightData, &light);

            if (light.type == LIGHT_DIRECTIONAL)
            {
                // Fullscreen quad
                DrawFullscreenQuad();
                bgfx::submit(VIEW_LIGHTING, directionalLightShader);
            }
            else if (light.type == LIGHT_POINT)
            {
                // Render light sphere volume
                DrawLightSphere(light.position, light.radius);
                bgfx::submit(VIEW_LIGHTING, pointLightShader);
            }
            else if (light.type == LIGHT_SPOT)
            {
                // Render light cone volume
                DrawLightCone(light);
                bgfx::submit(VIEW_LIGHTING, spotLightShader);
            }
        }
    }
};
```

### Deferred Light Shader (GLSL)

```glsl
// fs_deferred_point_light.sc
$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_gbufferNormal, 0);
SAMPLER2D(s_gbufferBaseColor, 1);
SAMPLER2D(s_gbufferMaterial, 2);  // r=roughness, g=metallic, b=specular
SAMPLER2D(s_gbufferDepth, 3);

uniform vec4 u_lightPosRadius;    // xyz = position, w = radius
uniform vec4 u_lightColor;        // rgb = color, a = intensity
uniform vec4 u_lightParams;       // x = falloff exponent

vec3 ReconstructWorldPosition(vec2 uv, float depth)
{
    // Reconstruct from depth
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 viewPos = mul(u_invProj, clipPos);
    viewPos /= viewPos.w;
    vec4 worldPos = mul(u_invView, viewPos);
    return worldPos.xyz;
}

float GetAttenuation(float distance, float radius, float exponent)
{
    float normalizedDist = saturate(distance / radius);
    float baseFalloff = saturate(1.0 - pow(normalizedDist, 4.0));
    return pow(baseFalloff, exponent);
}

void main()
{
    vec2 uv = v_texcoord0;

    // Read GBuffer
    vec3 normal = texture2D(s_gbufferNormal, uv).rgb * 2.0 - 1.0;
    vec4 baseColorAO = texture2D(s_gbufferBaseColor, uv);
    vec3 baseColor = baseColorAO.rgb;
    float ao = baseColorAO.a;

    vec3 material = texture2D(s_gbufferMaterial, uv).rgb;
    float roughness = material.r;
    float metallic = material.g;
    float specular = material.b;

    float depth = texture2D(s_gbufferDepth, uv).r;

    // Reconstruct position
    vec3 worldPos = ReconstructWorldPosition(uv, depth);

    // Light calculation
    vec3 lightPos = u_lightPosRadius.xyz;
    float lightRadius = u_lightPosRadius.w;

    vec3 L = lightPos - worldPos;
    float distance = length(L);
    L /= distance;  // Normalize

    // Attenuation
    float attenuation = GetAttenuation(distance, lightRadius, u_lightParams.x);

    if (attenuation <= 0.0)
    {
        discard;  // Outside light range
    }

    // View vector
    vec3 V = normalize(u_cameraPos - worldPos);

    // BRDF (simplified)
    vec3 H = normalize(V + L);
    float NoL = max(dot(normal, L), 0.0);
    float NoV = max(dot(normal, V), 0.0);
    float NoH = max(dot(normal, H), 0.0);
    float VoH = max(dot(V, H), 0.0);

    // Derived colors
    vec3 diffuseColor = baseColor * (1.0 - metallic);
    vec3 specularColor = mix(vec3_splat(0.04), baseColor, metallic);

    // Diffuse (Burley)
    float FD90 = 0.5 + 2.0 * VoH * VoH * roughness;
    float FdV = 1.0 + (FD90 - 1.0) * pow(1.0 - NoV, 5.0);
    float FdL = 1.0 + (FD90 - 1.0) * pow(1.0 - NoL, 5.0);
    vec3 diffuse = diffuseColor * (1.0 / 3.14159) * FdV * FdL;

    // Specular (GGX)
    float a = roughness * roughness;
    float a2 = a * a;

    // D (GGX)
    float denom = (NoH * a2 - NoH) * NoH + 1.0;
    float D = a2 / (3.14159 * denom * denom);

    // Vis (Smith)
    float k = a * 0.5;
    float vis = 0.5 / ((NoL * (NoV * (1.0 - k) + k) + NoV * (NoL * (1.0 - k) + k)));

    // F (Schlick)
    float Fc = pow(1.0 - VoH, 5.0);
    vec3 F = specularColor + (vec3_splat(1.0) - specularColor) * Fc;

    vec3 spec = D * vis * F;

    // Combine
    vec3 lighting = (diffuse + spec) * u_lightColor.rgb * u_lightColor.a * NoL * attenuation * ao;

    gl_FragColor = vec4(lighting, 1.0);
}
```

### Clustered Lighting (Simplified)

```cpp
// Build light grid
class ClusteredLightCuller
{
    static const int GRID_SIZE_X = 30;   // 1920 / 64
    static const int GRID_SIZE_Y = 17;   // 1080 / 64
    static const int GRID_SIZE_Z = 32;

    struct GridCell
    {
        uint32_t lightIndexStart;
        uint32_t numLights;
    };

    GridCell cells[GRID_SIZE_X * GRID_SIZE_Y * GRID_SIZE_Z];
    std::vector<uint32_t> lightIndices;

    bgfx::DynamicIndexBufferHandle lightIndexBuffer;
    bgfx::DynamicVertexBufferHandle cellDataBuffer;

    void BuildLightGrid(const std::vector<Light>& lights)
    {
        lightIndices.clear();

        for (int z = 0; z < GRID_SIZE_Z; ++z)
        {
            for (int y = 0; y < GRID_SIZE_Y; ++y)
            {
                for (int x = 0; x < GRID_SIZE_X; ++x)
                {
                    int cellIdx = x + y * GRID_SIZE_X + z * GRID_SIZE_X * GRID_SIZE_Y;

                    // Calculate cell bounds
                    AABB cellBounds = GetCellBounds(x, y, z);

                    // Test each light
                    uint32_t startIdx = lightIndices.size();

                    for (uint32_t i = 0; i < lights.size(); ++i)
                    {
                        if (LightIntersectsCell(lights[i], cellBounds))
                        {
                            lightIndices.push_back(i);
                        }
                    }

                    cells[cellIdx].lightIndexStart = startIdx;
                    cells[cellIdx].numLights = lightIndices.size() - startIdx;
                }
            }
        }

        // Upload to GPU
        bgfx::update(lightIndexBuffer, 0, bgfx::copy(lightIndices.data(),
                                                      lightIndices.size() * sizeof(uint32_t)));
        bgfx::update(cellDataBuffer, 0, bgfx::copy(cells, sizeof(cells)));
    }
};
```

---

## Key Files Reference

### C++ Source Files

**Light Components:**
- `Engine/Source/Runtime/Engine/Classes/Components/LightComponent.h`
- `Engine/Source/Runtime/Engine/Classes/Components/DirectionalLightComponent.h`
- `Engine/Source/Runtime/Engine/Classes/Components/PointLightComponent.h`
- `Engine/Source/Runtime/Engine/Classes/Components/SpotLightComponent.h`
- `Engine/Source/Runtime/Engine/Classes/Components/RectLightComponent.h`
- `Engine/Source/Runtime/Engine/Classes/Components/SkyLightComponent.h`

**Rendering:**
- `Engine/Source/Runtime/Renderer/Private/DeferredShadingRenderer.cpp`
  - Main render loop
- `Engine/Source/Runtime/Renderer/Private/LightRendering.cpp`
  - Deferred light rendering
- `Engine/Source/Runtime/Renderer/Private/ClusteredDeferredShadingPass.cpp`
  - Clustered deferred shading
- `Engine/Source/Runtime/Renderer/Private/LightGridInjection.cpp`
  - Light culling and grid building

**Data Structures:**
- `Engine/Source/Runtime/Engine/Public/SceneTypes.h`
  - Light type enums
- `Engine/Source/Runtime/Engine/Classes/Engine/Scene.h`
  - Light units enum

### Shader Files

**Deferred Lighting:**
- `Engine/Shaders/Private/DeferredLightingCommon.ush`
  - BRDF evaluation
- `Engine/Shaders/Private/DeferredLightPixelShaders.usf`
  - Per-light pixel shaders
- `Engine/Shaders/Private/DeferredShadingCommon.ush`
  - GBuffer utilities

**Forward Lighting:**
- `Engine/Shaders/Private/ForwardLightingCommon.ush`
  - Forward lighting evaluation

**Clustered:**
- `Engine/Shaders/Private/ClusteredDeferredShadingPixelShader.usf`
  - Clustered shading pass
- `Engine/Shaders/Private/LightGridInjection.usf`
  - Light grid building compute shader

---

## Summary

**For Your Custom Engine:**

1. **Start with basic deferred lighting**
   - Directional, point, spot lights
   - Simple attenuation and BRDF
   - Additive accumulation

2. **Add light parameters**
   - Intensity units (lumens/candelas)
   - Color temperature
   - Attenuation curves

3. **Implement light culling**
   - Tiled deferred (8×8 tiles)
   - Or clustered deferred (3D grid)
   - Dramatically improves multi-light performance

4. **Optimize**
   - Light volumes (sphere/cone rendering)
   - Depth bounds test
   - Stencil masking

**Reference Values:**
- Indoor bulb: 800-1700 lumens
- Car headlight: 1000-2000 lumens
- Sunlight: EV 15, ~6500K
- Attenuation radius: Auto-calculate from intensity

UE5's lighting system is battle-tested across hundreds of shipped titles. Following this architecture gives you physically-based, scalable lighting that looks great and performs well.
