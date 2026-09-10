#pragma once

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// The entrypoint/format pair the GPU device's active backend expects.
struct ComputeShaderFormat {
    SDL_GPUShaderFormat format;
    const char* entrypoint;
};

/// SPIR-V/"main" on Vulkan and every other backend; MSL/"main0" on Metal.
ComputeShaderFormat DetectComputeShaderFormat(SDL_GPUDevice* device);

}  // namespace sdl3cpp::services::impl
