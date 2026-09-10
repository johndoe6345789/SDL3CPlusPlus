#pragma once

#include <SDL3/SDL_gpu.h>

#include <vector>

namespace sdl3cpp::services::impl {

/// Clamp-to-edge linear sampler, for HDR/color texture sampling.
SDL_GPUSampler* CreatePostfxLinearSampler(SDL_GPUDevice* device);

/// Clamp-to-edge nearest sampler, for depth texture sampling (SSAO).
SDL_GPUSampler* CreatePostfxNearestSampler(SDL_GPUDevice* device);

/**
 * @brief Generates an SSAO hemisphere kernel.
 *
 * Each sample is a deterministically-hashed unit vector in the +Z
 * hemisphere, scaled quadratically (0.1 to 1.0) so more samples land
 * near the surface. Returns `sampleCount` vec4s (xyz + unused w),
 * flattened to `sampleCount * 4` floats.
 */
std::vector<float> GenerateSsaoKernel(int sampleCount);

}  // namespace sdl3cpp::services::impl
