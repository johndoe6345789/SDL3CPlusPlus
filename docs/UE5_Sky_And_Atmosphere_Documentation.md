# Unreal Engine 5 Sky and Atmosphere System Documentation
## Physically-Based Sky Rendering for Game Engines

This documentation explains UE5's sky and atmosphere rendering, from simple skyboxes to physically-accurate atmospheric scattering.

---

## Table of Contents
1. [Sky System Overview](#sky-system-overview)
2. [Skybox Types](#skybox-types)
3. [Sky Atmosphere (Physically-Based)](#sky-atmosphere-physically-based)
4. [Volumetric Clouds](#volumetric-clouds)
5. [Sky Light](#sky-light)
6. [Implementation Guide](#implementation-guide)
7. [bgfx Implementation Examples](#bgfx-implementation-examples)
8. [Key Files Reference](#key-files-reference)

---

## Sky System Overview

UE5 offers multiple sky rendering approaches:

```
Sky Rendering Stack (from simplest to most advanced):
┌──────────────────────────────────────────────┐
│ 1. Static Skybox (Cubemap)                  │
│    - Pre-rendered HDRI                       │
│    - Instant, zero cost                      │
│    - No time-of-day changes                  │
└──────────────────────────────────────────────┘
                   ↓
┌──────────────────────────────────────────────┐
│ 2. Sky Dome (Procedural Material)           │
│    - Shader-based gradients                  │
│    - Simple, customizable                    │
│    - Limited realism                         │
└──────────────────────────────────────────────┘
                   ↓
┌──────────────────────────────────────────────┐
│ 3. Sky Atmosphere (Physical)                │
│    - Rayleigh scattering (blue sky)          │
│    - Mie scattering (haze)                   │
│    - Ozone absorption                        │
│    - Sun disk rendering                      │
│    - Aerial perspective                      │
└──────────────────────────────────────────────┘
                   ↓ (optional)
┌──────────────────────────────────────────────┐
│ 4. Volumetric Clouds                        │
│    - Ray-marched 3D clouds                   │
│    - Dynamic lighting                        │
│    - Weather systems                         │
└──────────────────────────────────────────────┘
```

---

## Skybox Types

### 1. HDRI Cubemap Skybox

**Key File:** `Engine/Source/Runtime/Engine/Classes/Components/SkyLightComponent.h`

**Simple and effective:**

```cpp
class USkyLightComponent : public ULightComponentBase
{
    // Source type
    ESkyLightSourceType SourceType;  // Captured or Specified

    // If specified cubemap:
    UTextureCube* Cubemap;          // HDRI texture
    float SourceCubemapAngle;       // Rotation (0-360°)

    // Hemisphere split
    bool bLowerHemisphereIsBlack;   // Solid ground below?
    FLinearColor LowerHemisphereColor;
};

enum ESkyLightSourceType
{
    SLS_CapturedScene,     // Capture from scene
    SLS_SpecifiedCubemap   // Use provided HDRI
};
```

**Typical workflow:**
1. Load HDRI texture (`.hdr` or `.exr` format)
2. Create cubemap texture from equirectangular
3. Assign to Sky Light component
4. Sky Light provides ambient lighting to scene

**Advantages:**
- Zero runtime cost (static texture)
- High quality (real-world HDRIs)
- Perfect for indoor scenes or fixed outdoor

**Limitations:**
- Cannot change time of day
- No dynamic sun
- No atmospheric effects

### 2. Procedural Sky Dome

**Simple gradient-based sky:**

```hlsl
// Sky dome vertex shader
float4 SkyDomeVS(float3 position : POSITION) : SV_Position
{
    // Just pass through, no transforms needed
    return float4(position, 1.0);
}

// Sky dome pixel shader
float4 SkyDomePS(float4 position : SV_Position) : SV_Target
{
    float3 viewDir = normalize(position.xyz);

    // Vertical gradient
    float t = viewDir.z * 0.5 + 0.5;  // 0 at horizon, 1 at zenith

    // Lerp between horizon and zenith colors
    float3 horizonColor = float3(0.8, 0.9, 1.0);  // Light blue
    float3 zenithColor = float3(0.2, 0.4, 0.8);   // Dark blue

    float3 skyColor = lerp(horizonColor, zenithColor, t);

    return float4(skyColor, 1.0);
}
```

**Common approach for stylized games:**
- Mobile games
- Low-spec targets
- Artistic control

---

## Sky Atmosphere (Physically-Based)

UE5's flagship sky system - physically accurate atmospheric scattering.

**Key Files:**
- `Engine/Source/Runtime/Engine/Classes/Components/SkyAtmosphereComponent.h`
- `Engine/Shaders/Private/SkyAtmosphere.usf`
- `Engine/Shaders/Private/SkyAtmosphereCommon.ush`

### Core Parameters

```cpp
class USkyAtmosphereComponent : public USceneComponent
{
    // Planet properties
    float BottomRadius;          // Ground radius (km), default: 6360
    float AtmosphereHeight;      // Atmosphere layer (km), default: 60
    FColor GroundAlbedo;         // Ground reflectance
    float MultiScatteringFactor; // Dual scattering approximation

    // Transform mode
    ESkyAtmosphereTransformMode TransformMode;
};

enum ESkyAtmosphereTransformMode
{
    PlanetTopAtAbsoluteWorldOrigin,    // Planet surface at world 0,0,0
    PlanetTopAtComponentTransform,     // Planet surface at component location
    PlanetCenterAtComponentTransform   // Planet center at component location
};
```

### Physical Model

The atmosphere is modeled using **participating media** with three components:

#### 1. Rayleigh Scattering (Blue Sky)

**Why the sky is blue:**

```cpp
// Rayleigh scattering parameters
struct FRayleighScattering
{
    float ScatteringScale;              // Overall intensity (0-2)
    FLinearColor Scattering;            // RGB coefficients (1/km)
    float ExponentialDistribution;       // Altitude falloff (km)
};

// Default values (Earth-like):
// Scattering = (0.0331, 0.0697, 0.1649) // Blue scatters most
// ExponentialDistribution = 8.0 km      // Scale height
```

**Density calculation:**
```hlsl
// Atmosphere density decreases exponentially with altitude
float DensityRay = exp(-SampleHeight / ScaleHeight);

// Scattering coefficient
float3 ScatteringRay = DensityRay * Atmosphere.RayleighScattering.rgb;
```

**Phase function** (angular distribution):
```hlsl
float RayleighPhase(float cosTheta)
{
    // Symmetric scattering
    const float Factor = 3.0 / (16.0 * PI);
    return Factor * (1.0 + cosTheta * cosTheta);
}
```

**Physics:**
- Short wavelengths (blue) scatter more than long (red)
- Factor: λ^-4 (inverse fourth power)
- Results in blue sky during day, red sunset

#### 2. Mie Scattering (Haze, Fog)

**Aerosols and particles:**

```cpp
struct FMieScattering
{
    float ScatteringScale;           // Overall intensity (0-5)
    FLinearColor Scattering;         // RGB coefficients (1/km)
    float AbsorptionScale;           // Absorption intensity (0-5)
    FLinearColor Absorption;         // RGB absorption (1/km)
    float Anisotropy;                // Phase function g (-0.999 to 0.999)
    float ExponentialDistribution;   // Altitude falloff (km)
};

// Default values:
// Scattering = (0.0021, 0.0021, 0.0021)  // Grayscale
// Absorption = (0.0045, 0.0045, 0.0045)
// Anisotropy = 0.8                        // Forward scattering
// ExponentialDistribution = 1.2 km        // Low altitude
```

**Density calculation:**
```hlsl
float DensityMie = exp(-SampleHeight / ScaleHeight);

float3 ScatteringMie = DensityMie * Atmosphere.MieScattering.rgb;
float3 AbsorptionMie = DensityMie * Atmosphere.MieAbsorption.rgb;
float3 ExtinctionMie = ScatteringMie + AbsorptionMie;
```

**Henyey-Greenstein phase function:**
```hlsl
float HenyeyGreensteinPhase(float g, float cosTheta)
{
    float numer = 1.0 - g * g;
    float denom = 1.0 + g * g + 2.0 * g * cosTheta;
    return numer / (4.0 * PI * denom * sqrt(denom));
}

// g > 0: Forward scattering (haze around sun)
// g = 0: Isotropic (uniform)
// g < 0: Backward scattering (rare)
```

**Physics:**
- Larger particles (dust, water droplets)
- Wavelength-independent (white/gray)
- Strong forward scattering (bright halo around sun)
- Responsible for haze, fog, sunsets

#### 3. Ozone Absorption

**Stratospheric ozone layer:**

```cpp
struct FOzoneAbsorption
{
    float AbsorptionScale;           // Overall intensity (0-0.2)
    FLinearColor Absorption;         // RGB absorption (1/km)
    FTentDistribution Distribution;  // Altitude distribution
};

struct FTentDistribution
{
    float TipAltitude;   // Peak altitude (km), default: 25
    float TipValue;      // Peak density, default: 1.0
    float Width;         // Distribution width (km), default: 15
};

// Default absorption:
// Absorption = (0.065, 0.1881, 0.0085)  // Absorbs red/green, not blue
```

**Tent distribution:**
```hlsl
float TentDistribution(float altitude, FTentDistribution tent)
{
    float distance = abs(altitude - tent.TipAltitude);
    float value = saturate(1.0 - distance / tent.Width);
    return value * tent.TipValue;
}
```

**Physics:**
- Ozone (O₃) in stratosphere (15-35 km)
- Absorbs UV and red/green light
- Contributes to orange/red sunsets
- Blue sky at horizon vs overhead

### Rendering Pipeline

UE5 uses **Look-Up Tables (LUTs)** for performance:

```
Precomputed LUTs:
┌────────────────────────────────┐
│ 1. Transmittance LUT (256×64) │
│    - View height × zenith angle │
│    - Transmittance through atm  │
└────────────────────────────────┘
           ↓
┌────────────────────────────────┐
│ 2. Multi-Scattering LUT (32×32)│
│    - Approximates 2+ bounces    │
│    - Energy conservation        │
└────────────────────────────────┘
           ↓
┌────────────────────────────────┐
│ 3. Sky View LUT (192×104)      │
│    - Pre-rendered sky directions│
│    - Fast sky lookup            │
└────────────────────────────────┘
           ↓
┌────────────────────────────────┐
│ 4. Aerial Perspective (32×32×16)│
│    - 3D volume for fog          │
│    - Applied to opaque geometry │
└────────────────────────────────┘
```

#### 1. Transmittance LUT

**What it stores:** Transmittance from any height through atmosphere

```hlsl
// Compute shader: RenderTransmittanceLutCS
[numthreads(8, 8, 1)]
void TransmittanceLutCS(uint2 ThreadId : SV_DispatchThreadID)
{
    // UV to view height and zenith angle
    float2 uv = (ThreadId + 0.5) / float2(256, 64);

    float viewHeight, viewZenithCosAngle;
    UvToLutTransmittanceParams(uv, viewHeight, viewZenithCosAngle);

    // Ray march from viewHeight to atmosphere top
    float3 worldPos = float3(0, 0, viewHeight);
    float3 worldDir = float3(
        sqrt(1.0 - viewZenithCosAngle * viewZenithCosAngle),
        0,
        viewZenithCosAngle
    );

    // Find intersection with atmosphere top
    float tMax = RaySphereIntersection(worldPos, worldDir, AtmosphereTopRadius);

    // Ray march and accumulate optical depth
    const int numSamples = 10;
    float dt = tMax / numSamples;

    float3 opticalDepth = 0;

    for (int i = 0; i < numSamples; ++i)
    {
        float t = (i + 0.5) * dt;
        float3 samplePos = worldPos + worldDir * t;
        float sampleHeight = length(samplePos) - BottomRadius;

        // Sample medium
        FMediumSample medium = SampleMedium(sampleHeight);

        // Accumulate extinction
        opticalDepth += medium.Extinction * dt;
    }

    // Transmittance = exp(-optical depth)
    float3 transmittance = exp(-opticalDepth);

    TransmittanceLUT[ThreadId] = float4(transmittance, 1.0);
}
```

**UV Parameterization:**
```hlsl
void UvToLutTransmittanceParams(float2 uv, out float viewHeight, out float viewZenithCosAngle)
{
    float xMu = uv.x;
    float xR = uv.y;

    float H = sqrt(AtmosphereTopRadius^2 - BottomRadius^2);
    float rho = xR * H;

    viewHeight = sqrt(rho^2 + BottomRadius^2);

    float dMin = AtmosphereTopRadius - viewHeight;
    float dMax = rho + H;
    float d = dMin + xMu * (dMax - dMin);

    viewZenithCosAngle = (H^2 - rho^2 - d^2) / (2.0 * viewHeight * d);
}
```

#### 2. Multi-Scattering LUT

**Approximates multiple bounces of light:**

```hlsl
// Simplified multi-scattering
[numthreads(8, 8, 1)]
void MultiScatteringLutCS(uint2 ThreadId : SV_DispatchThreadID)
{
    float2 uv = (ThreadId + 0.5) / 32.0;

    // Sample in spherical coordinates
    float cosSunZenith = uv.x * 2.0 - 1.0;  // -1 to 1
    float viewHeight = lerp(BottomRadius, AtmosphereTopRadius, uv.y);

    // Integrate over all view directions (hemisphere)
    float3 multiScattering = 0;

    const int numDirs = 64;  // Sample directions

    for (int i = 0; i < numDirs; ++i)
    {
        float3 viewDir = FibonacciHemisphere(i, numDirs);

        // Compute single scattering for this direction
        SingleScatteringResult ss = IntegrateSingleScattering(
            viewHeight, viewDir, cosSunZenith
        );

        multiScattering += ss.Scattering;
    }

    multiScattering /= numDirs;
    multiScattering *= MultiScatteringFactor;

    MultiScatteringLUT[ThreadId] = float4(multiScattering, 1.0);
}
```

#### 3. Sky View LUT (Fast Sky)

**Pre-rendered sky for fast lookups:**

```hlsl
[numthreads(8, 8, 1)]
void SkyViewLutCS(uint2 ThreadId : SV_DispatchThreadID)
{
    float2 uv = (ThreadId + 0.5) / float2(192, 104);

    // UV to spherical direction (with horizon compression)
    float3 viewDir = SkViewLutParamsToDirection(uv);

    // Camera position
    float3 cameraPos = float3(0, 0, BottomRadius + CameraAltitude);

    // Ray march sky
    SingleScatteringResult sky = IntegrateSingleScattering(
        cameraPos,
        viewDir,
        SunDirection,
        MaxDistance
    );

    // Store radiance
    SkyViewLUT[ThreadId] = float4(sky.L, 1.0);
}
```

**Usage:**
```hlsl
// Sample sky instead of ray marching
float3 GetSkyColor(float3 viewDir)
{
    float2 uv = DirectionToSkyViewLutParams(viewDir);
    return SkyViewLUT.SampleLevel(LinearSampler, uv, 0).rgb;
}
```

#### 4. Aerial Perspective Volume

**3D fog volume applied to opaque geometry:**

```hlsl
[numthreads(4, 4, 4)]
void AerialPerspectiveCS(uint3 ThreadId : SV_DispatchThreadID)
{
    float3 uvw = (ThreadId + 0.5) / float3(32, 32, 16);

    // UV to world position
    float2 screenUV = uvw.xy;
    float depth = DepthFromSlice(uvw.z);  // Non-linear distribution

    float3 worldPos = ScreenToWorld(screenUV, depth);
    float3 cameraPos = View.WorldCameraOrigin;

    // Ray march from camera to world position
    SingleScatteringResult aerial = IntegrateSingleScattering(
        cameraPos,
        normalize(worldPos - cameraPos),
        SunDirection,
        length(worldPos - cameraPos)
    );

    // Store scattering and transmittance
    AerialPerspectiveVolume[ThreadId] = float4(aerial.L, aerial.Transmittance);
}
```

**Application:**
```hlsl
// In deferred lighting or forward pass
float3 sceneColor = ...; // Lit scene color

// Sample aerial perspective
float3 uvw = float3(screenUV, DepthToSlice(depth));
float4 aerialPerspective = AerialPerspectiveVolume.SampleLevel(LinearSampler, uvw, 0);

// Apply fog
float3 finalColor = sceneColor * aerialPerspective.a + aerialPerspective.rgb;
```

### Sun Disk Rendering

**Add bright sun disk:**

```hlsl
float3 GetSunDisk(float3 viewDir, float3 sunDir, float sunAngularRadius)
{
    float ViewDotSun = dot(viewDir, sunDir);
    float CosSunRadius = cos(sunAngularRadius);

    if (ViewDotSun > CosSunRadius)
    {
        // Inside sun disk

        // Get transmittance to sun
        float3 transmittance = GetAtmosphereTransmittance(
            ViewHeight,
            ViewDotSun
        );

        // Soft edge to prevent bloom artifacts
        float EdgeSoftness = saturate(2.0 * (ViewDotSun - CosSunRadius) / (1.0 - CosSunRadius));

        // Sun luminance (in lux or nits)
        float3 sunLuminance = SunIlluminance * EdgeSoftness;

        return transmittance * sunLuminance;
    }

    return 0;
}
```

### Console Variables

```cpp
// Quality
r.SkyAtmosphere.SampleCountMin = 2
r.SkyAtmosphere.SampleCountMax = 32
r.SkyAtmosphere.DistanceToSampleCountMaxInv = 150  // km

// Fast sky (LUT-based)
r.SkyAtmosphere.FastSkyLUT = 1
r.SkyAtmosphere.FastSkyLUT.SampleCountMax = 32

// Aerial perspective
r.SkyAtmosphere.AerialPerspectiveLUT.Width = 32
r.SkyAtmosphere.AerialPerspectiveLUT.DepthResolution = 16
r.SkyAtmosphere.AerialPerspectiveLUT.Depth = 96  // km

// Async compute
r.SkyAtmosphereAsyncCompute = 0
```

---

## Volumetric Clouds

3D ray-marched clouds with realistic lighting.

**Key Files:**
- `Engine/Source/Runtime/Engine/Classes/Components/VolumetricCloudComponent.h`
- `Engine/Shaders/Private/VolumetricCloud.usf`

### Cloud Parameters

```cpp
class UVolumetricCloudComponent : public USceneComponent
{
    // Layer configuration
    float LayerBottomAltitude;          // Cloud base (km)
    float LayerHeight;                  // Cloud thickness (km)

    // Trace distances
    float TracingStartMaxDistance;      // Max start distance (km)
    float TracingMaxDistance;           // Max trace distance (km)

    // Quality
    float ViewSampleCountScale;         // Primary view samples (0.05-8)
    float ReflectionViewSampleCountScaleValue;  // Reflection samples
    float ShadowViewSampleCountScale;   // Shadow samples
    float ShadowTracingDistance;        // Shadow trace distance (km)

    // Lighting
    bool bUsePerSampleAtmosphericLightTransmittance;
    float SkyLightCloudBottomOcclusion;  // Sky light AO (0-1)

    // Material
    UMaterialInterface* CloudMaterial;   // Volume domain material
};
```

### Cloud Material (Volume Domain)

```hlsl
// Cloud material outputs
struct FCloudMaterialOutput
{
    float3 Extinction;   // How much light is blocked (RGB)
    float3 Albedo;       // Scattering color (RGB)
    float3 Emissive;     // Self-illumination
    float AmbientOcclusion;  // Sky light shadowing
};
```

**Typical cloud material:**
- Sample 3D noise textures (Perlin, Worley)
- Combine at multiple frequencies (detail)
- Remap density based on altitude
- Add weather patterns

### Ray Marching

```hlsl
// Simplified cloud ray marching
float4 RayMarchClouds(float3 rayOrigin, float3 rayDir, float maxDistance)
{
    const int maxSteps = 128;  // Configurable quality

    float3 accumulatedLight = 0;
    float accumulatedTransmittance = 1.0;

    float t = 0;
    float dt = maxDistance / maxSteps;

    for (int step = 0; step < maxSteps; ++step)
    {
        float3 samplePos = rayOrigin + rayDir * t;

        // Sample cloud density from material
        FCloudMaterialOutput cloud = EvaluateCloudMaterial(samplePos);

        if (cloud.Extinction.r > 0.0)
        {
            // In cloud

            // Beer's law transmittance
            float sampleTransmittance = exp(-cloud.Extinction.r * dt);

            // Light contribution
            float3 lighting = 0;

            // Sun lighting (shadow ray march)
            float3 shadowRayDir = SunDirection;
            float shadowOpticalDepth = MarchShadowRay(samplePos, shadowRayDir);
            float sunTransmittance = exp(-shadowOpticalDepth);

            lighting += SunColor * sunTransmittance * cloud.Albedo;

            // Sky light (ambient)
            lighting += SkyLightColor * (1.0 - cloud.AmbientOcclusion) * cloud.Albedo;

            // Emissive
            lighting += cloud.Emissive;

            // Accumulate
            accumulatedLight += lighting * accumulatedTransmittance * (1.0 - sampleTransmittance);
            accumulatedTransmittance *= sampleTransmittance;

            // Early termination
            if (accumulatedTransmittance < 0.01)
                break;
        }

        t += dt;
    }

    return float4(accumulatedLight, accumulatedTransmittance);
}
```

### Performance Optimization

**Temporal reprojection:**
```hlsl
// Current frame (noisy, few samples)
float4 currentClouds = RayMarchClouds(rayOrigin, rayDir, maxDistance);

// Reproject previous frame
float2 velocity = VelocityBuffer.Sample(screenUV).xy;
float2 prevUV = screenUV - velocity;

float4 previousClouds = PreviousCloudBuffer.Sample(prevUV);

// Temporal blend
float blendWeight = 0.95;  // High persistence

// Validate history
if (DepthChanged(prevUV) || OutOfBounds(prevUV))
    blendWeight = 0.0;

float4 finalClouds = lerp(currentClouds, previousClouds, blendWeight);
```

**Adaptive sampling:**
```cpp
// Fewer samples in clear sky, more in clouds
int numSamples = baseSamples;

if (hitCloud)
    numSamples *= 4;  // Increase detail in clouds
```

---

## Sky Light

Provides ambient environment lighting.

**Key File:** `Engine/Source/Runtime/Engine/Classes/Components/SkyLightComponent.h`

### Capture Modes

```cpp
// Static capture
bRealTimeCapture = false;
SourceType = SLS_SpecifiedCubemap;
Cubemap = MyHDRICubemap;

// Dynamic capture
bRealTimeCapture = true;
SourceType = SLS_CapturedScene;
SkyDistanceThreshold = 10000;  // cm, capture beyond this distance
```

### Cubemap Processing

**Convolution for diffuse:**
```hlsl
// Irradiance map (diffuse)
float3 GenerateIrradianceMap(float3 normal, TextureCube envMap)
{
    float3 irradiance = 0;

    // Sample hemisphere around normal
    const int numSamples = 1024;

    for (int i = 0; i < numSamples; ++i)
    {
        float2 xi = Hammersley(i, numSamples);
        float3 sampleDir = UniformSampleHemisphere(xi, normal);

        float3 envColor = envMap.SampleLevel(LinearSampler, sampleDir, 0).rgb;

        irradiance += envColor * dot(normal, sampleDir);
    }

    irradiance *= PI / numSamples;

    return irradiance;
}
```

**Prefiltered specular:**
```hlsl
// Prefilter for roughness (mip level = roughness)
float3 PrefilterEnvMap(float3 reflectDir, float roughness, TextureCube envMap)
{
    float3 prefilteredColor = 0;
    float totalWeight = 0;

    const int numSamples = 256;

    for (int i = 0; i < numSamples; ++i)
    {
        float2 xi = Hammersley(i, numSamples);
        float3 H = ImportanceSampleGGX(xi, roughness, reflectDir);
        float3 L = 2.0 * dot(reflectDir, H) * H - reflectDir;

        float NoL = max(dot(reflectDir, L), 0.0);

        if (NoL > 0.0)
        {
            float3 envColor = envMap.SampleLevel(LinearSampler, L, 0).rgb;

            prefilteredColor += envColor * NoL;
            totalWeight += NoL;
        }
    }

    return prefilteredColor / totalWeight;
}
```

### Spherical Harmonics (SH)

**Efficient diffuse representation:**

```cpp
struct FSHVectorRGB3
{
    float V[3][9];  // RGB × 9 SH coefficients
};

// Project cubemap to SH (L=2, 9 coefficients)
FSHVectorRGB3 ProjectToSH(TextureCube envMap)
{
    FSHVectorRGB3 sh = {};

    // Sample cubemap
    for each face
        for each pixel
            float3 direction = PixelToDirection(face, pixel);
            float3 color = envMap.Load(face, pixel).rgb;

            // Evaluate SH basis functions
            float basis[9] = EvaluateSHBasis(direction);

            // Accumulate
            for (int i = 0; i < 9; ++i)
                sh.V[0][i] += color.r * basis[i];
                sh.V[1][i] += color.g * basis[i];
                sh.V[2][i] += color.b * basis[i];

    // Normalize
    sh /= totalPixels;

    return sh;
}
```

**Reconstruction:**
```hlsl
float3 SampleSH(FSHVectorRGB3 sh, float3 normal)
{
    // Evaluate SH basis
    float basis[9] = EvaluateSHBasis(normal);

    // Dot product with coefficients
    float3 irradiance = 0;

    for (int i = 0; i < 9; ++i)
    {
        irradiance.r += sh.V[0][i] * basis[i];
        irradiance.g += sh.V[1][i] * basis[i];
        irradiance.b += sh.V[2][i] * basis[i];
    }

    return max(0, irradiance);
}
```

---

## Implementation Guide

### Minimum Viable Sky System

**Week 1: Static Skybox**
1. Load HDRI cubemap
2. Render skybox (inverted sphere or cube)
3. Sky Light with SH for ambient

**Week 2: Basic Atmosphere**
4. Transmittance LUT (compute shader)
5. Simple ray marching (no multi-scattering)
6. Sun disk rendering

**Week 3-4: Full Atmosphere**
7. Multi-scattering LUT
8. Mie scattering and ozone
9. Aerial perspective volume
10. Sky View LUT for performance

**Week 5-8: Clouds (Optional)**
11. 3D noise generation
12. Cloud ray marching
13. Cloud lighting
14. Temporal reprojection

### Key Algorithms

**Ray-Sphere Intersection:**
```hlsl
float RaySphereIntersection(float3 rayOrigin, float3 rayDir, float sphereRadius)
{
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(rayOrigin, rayDir);
    float c = dot(rayOrigin, rayOrigin) - sphereRadius * sphereRadius;

    float discriminant = b * b - 4.0 * a * c;

    if (discriminant < 0.0)
        return -1.0;  // No intersection

    float t = (-b + sqrt(discriminant)) / (2.0 * a);

    return t;
}
```

**Analytical Integration (Better than dt):**
```hlsl
// Instead of: L += Throughput * Scattering * dt
// Use: Analytical integration of exponential

float3 IntegrateScatteringOverSegment(float3 scattering, float3 extinction, float dt)
{
    float3 safeExtinction = max(extinction, 0.0001);

    // ∫ e^(-extinction*t) dt from 0 to dt
    float3 transmittance = exp(-extinction * dt);

    // Analytical: (scattering / extinction) * (1 - transmittance)
    float3 integrated = (scattering / safeExtinction) * (1.0 - transmittance);

    return integrated;
}
```

---

## bgfx Implementation Examples

### Simple Skybox

```cpp
class SkyboxRenderer
{
    bgfx::TextureHandle skyboxCubemap;
    bgfx::ProgramHandle skyboxProgram;
    bgfx::VertexBufferHandle cubeVB;
    bgfx::IndexBufferHandle cubeIB;

    void RenderSkybox()
    {
        // Disable depth writes, always pass depth test
        uint64_t state = BGFX_STATE_WRITE_RGB
                       | BGFX_STATE_DEPTH_TEST_LEQUAL;

        bgfx::setState(state);

        // Remove translation from view matrix
        mat4 skyboxView = view;
        skyboxView[3] = vec4(0, 0, 0, 1);  // Clear translation

        bgfx::setTransform(identity);
        bgfx::setViewTransform(VIEW_SKY, &skyboxView, &projection);

        // Bind cubemap
        bgfx::setTexture(0, s_skybox, skyboxCubemap);

        // Render cube
        bgfx::setVertexBuffer(0, cubeVB);
        bgfx::setIndexBuffer(cubeIB);

        bgfx::submit(VIEW_SKY, skyboxProgram);
    }
};
```

**Skybox shader:**
```glsl
// vs_skybox.sc
$input a_position
$output v_dir

#include <bgfx_shader.sh>

void main()
{
    // Output direction (no transformation)
    v_dir = a_position;

    // Project position
    gl_Position = mul(u_viewProj, vec4(a_position, 1.0));

    // Force maximum depth
    gl_Position.z = gl_Position.w;
}

// fs_skybox.sc
$input v_dir

#include <bgfx_shader.sh>

SAMPLERCUBE(s_skybox, 0);

void main()
{
    vec3 dir = normalize(v_dir);
    gl_FragColor = textureCube(s_skybox, dir);
}
```

### Atmosphere Transmittance LUT

```cpp
// Compute shader to generate transmittance LUT
class AtmosphereRenderer
{
    bgfx::TextureHandle transmittanceLUT;
    bgfx::ProgramHandle transmittanceCS;

    void Init()
    {
        // Create transmittance LUT texture
        transmittanceLUT = bgfx::createTexture2D(
            256, 64, false, 1,
            bgfx::TextureFormat::RGBA16F,
            BGFX_TEXTURE_COMPUTE_WRITE | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP
        );

        // Load compute shader
        transmittanceCS = bgfx::createProgram(
            bgfx::createShader(loadMemory("cs_transmittance_lut.bin")),
            true
        );
    }

    void GenerateTransmittanceLUT()
    {
        // Bind output texture
        bgfx::setImage(0, transmittanceLUT, 0, bgfx::Access::Write);

        // Set atmosphere parameters
        bgfx::setUniform(u_atmosphereParams, &atmosphereParams);

        // Dispatch (256/8 = 32, 64/8 = 8)
        bgfx::dispatch(VIEW_COMPUTE, transmittanceCS, 32, 8, 1);
    }
};
```

**Transmittance compute shader:**
```glsl
// cs_transmittance_lut.sc
#include <bgfx_compute.sh>

IMAGE2D_WR(s_transmittanceLUT, rgba16f, 0);

uniform vec4 u_atmosphereParams; // x=bottomRadius, y=topRadius, z=scaleHeight

float RaySphereIntersection(vec3 ro, vec3 rd, float r)
{
    float a = dot(rd, rd);
    float b = 2.0 * dot(ro, rd);
    float c = dot(ro, ro) - r * r;
    float discriminant = b * b - 4.0 * a * c;

    if (discriminant < 0.0)
        return -1.0;

    return (-b + sqrt(discriminant)) / (2.0 * a);
}

NUM_THREADS(8, 8, 1)
void main()
{
    vec2 uv = (gl_GlobalInvocationID.xy + 0.5) / vec2(256.0, 64.0);

    float bottomRadius = u_atmosphereParams.x;
    float topRadius = u_atmosphereParams.y;
    float scaleHeight = u_atmosphereParams.z;

    // UV to view parameters
    float viewHeight = mix(bottomRadius, topRadius, uv.y);
    float viewZenithCos = uv.x * 2.0 - 1.0;

    // Ray march
    vec3 worldPos = vec3(0, 0, viewHeight);
    vec3 worldDir = vec3(sqrt(1.0 - viewZenithCos * viewZenithCos), 0, viewZenithCos);

    float tMax = RaySphereIntersection(worldPos, worldDir, topRadius);

    const int numSamples = 10;
    float dt = tMax / float(numSamples);

    vec3 opticalDepth = vec3_splat(0.0);

    for (int i = 0; i < numSamples; ++i)
    {
        float t = (float(i) + 0.5) * dt;
        vec3 samplePos = worldPos + worldDir * t;
        float sampleHeight = length(samplePos) - bottomRadius;

        // Rayleigh density
        float density = exp(-sampleHeight / scaleHeight);

        // Scattering coefficient (wavelength dependent)
        vec3 scattering = vec3(0.0331, 0.0697, 0.1649) * density;

        opticalDepth += scattering * dt;
    }

    vec3 transmittance = exp(-opticalDepth);

    imageStore(s_transmittanceLUT, ivec2(gl_GlobalInvocationID.xy), vec4(transmittance, 1.0));
}
```

### Sky Rendering with Atmosphere

```glsl
// fs_sky_atmosphere.sc
$input v_viewDir

#include <bgfx_shader.sh>

SAMPLER2D(s_transmittanceLUT, 0);

uniform vec4 u_sunDirection;
uniform vec4 u_atmosphereParams;

vec3 IntegrateSkyAtmosphere(vec3 viewDir)
{
    float bottomRadius = u_atmosphereParams.x;
    float topRadius = u_atmosphereParams.y;

    vec3 cameraPos = vec3(0, 0, bottomRadius + 0.001);  // 1m above ground

    // Find intersection with atmosphere
    float tMax = RaySphereIntersection(cameraPos, viewDir, topRadius);

    const int numSamples = 16;
    float dt = tMax / float(numSamples);

    vec3 L = vec3_splat(0.0);
    vec3 throughput = vec3_splat(1.0);

    for (int i = 0; i < numSamples; ++i)
    {
        float t = (float(i) + 0.5) * dt;
        vec3 samplePos = cameraPos + viewDir * t;
        float sampleHeight = length(samplePos) - bottomRadius;

        // Medium properties
        float density = exp(-sampleHeight / 8.0);
        vec3 scattering = vec3(0.0331, 0.0697, 0.1649) * density;

        // Phase function (Rayleigh)
        float cosTheta = dot(viewDir, u_sunDirection.xyz);
        float phase = 0.75 * (1.0 + cosTheta * cosTheta);

        // Sun transmittance (sample LUT)
        float sunHeight = length(samplePos);
        float sunZenithCos = dot(normalize(samplePos), u_sunDirection.xyz);
        vec2 transmittanceUV = vec2((sunZenithCos + 1.0) * 0.5, (sunHeight - bottomRadius) / (topRadius - bottomRadius));
        vec3 sunTransmittance = texture2D(s_transmittanceLUT, transmittanceUV).rgb;

        // Scattering contribution
        vec3 S = scattering * phase * sunTransmittance;

        // Integrate
        vec3 sampleTransmittance = exp(-scattering * dt);
        vec3 Sint = (S - S * sampleTransmittance) / scattering;
        L += throughput * Sint;
        throughput *= sampleTransmittance;
    }

    return L;
}

void main()
{
    vec3 skyColor = IntegrateSkyAtmosphere(normalize(v_viewDir));
    gl_FragColor = vec4(skyColor, 1.0);
}
```

---

## Key Files Reference

### C++ Source Files

**Components:**
- `Engine/Source/Runtime/Engine/Classes/Components/SkyAtmosphereComponent.h`
- `Engine/Source/Runtime/Engine/Classes/Components/SkyLightComponent.h`
- `Engine/Source/Runtime/Engine/Classes/Components/VolumetricCloudComponent.h`

**Rendering:**
- `Engine/Source/Runtime/Renderer/Private/SkyAtmosphereRendering.cpp`
  - Main atmosphere rendering logic
- `Engine/Source/Runtime/Renderer/Private/VolumetricCloudRendering.cpp`
  - Cloud rendering

**Common Data:**
- `Engine/Source/Runtime/Engine/Public/Rendering/SkyAtmosphereCommonData.h`
  - Shared data structures

### Shader Files

**Atmosphere:**
- `Engine/Shaders/Private/SkyAtmosphere.usf`
  - Main atmosphere shaders
- `Engine/Shaders/Private/SkyAtmosphereCommon.ush`
  - Common utilities and functions
- `Engine/Shaders/Private/ParticipatingMediaCommon.ush`
  - Phase functions, scattering math

**Clouds:**
- `Engine/Shaders/Private/VolumetricCloud.usf`
  - Cloud ray marching
- `Engine/Shaders/Private/VolumetricCloudCommon.ush`
  - Cloud utilities

---

## Summary

**For Your Custom Engine:**

1. **Start simple**: Static HDRI skybox + sky light (Week 1)
2. **Add atmosphere**: Transmittance LUT + basic ray marching (Week 2-3)
3. **Optimize**: Multi-scattering, sky view LUT, aerial perspective (Week 4-6)
4. **Advanced**: Volumetric clouds (Weeks 7-12, optional)

**Key Insights:**
- **LUTs are essential**: Precompute expensive calculations
- **Analytical integration**: More accurate than simple dt multiplication
- **Temporal reprojection**: Critical for cloud performance
- **Physical units**: Use real-world values (km, wavelengths)

**Typical Performance Budget:**
- Skybox: <0.1ms (trivial)
- Atmosphere (with LUTs): 0.5-1ms
- Clouds (full quality): 2-8ms (very expensive)

UE5's sky and atmosphere system provides cinematic-quality outdoor environments. Even implementing basic atmosphere dramatically improves visual realism compared to simple skyboxes.
