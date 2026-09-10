#include "services/interfaces/workflow/rendering/postfx_samplers.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

float HashFloat(int i, int seed) {
    int h = i * 374761393 + seed * 668265263;
    h     = (h ^ (h >> 13)) * 1274126177;
    return static_cast<float>(h & 0x7FFFFFFF) / static_cast<float>(0x7FFFFFFF);
}

}  // namespace

SDL_GPUSampler* CreatePostfxLinearSampler(SDL_GPUDevice* device) {
    SDL_GPUSamplerCreateInfo info = {};
    info.min_filter               = SDL_GPU_FILTER_LINEAR;
    info.mag_filter               = SDL_GPU_FILTER_LINEAR;
    info.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    return SDL_CreateGPUSampler(device, &info);
}

SDL_GPUSampler* CreatePostfxNearestSampler(SDL_GPUDevice* device) {
    SDL_GPUSamplerCreateInfo info = {};
    info.min_filter               = SDL_GPU_FILTER_NEAREST;
    info.mag_filter               = SDL_GPU_FILTER_NEAREST;
    info.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    return SDL_CreateGPUSampler(device, &info);
}

std::vector<float> GenerateSsaoKernel(int sampleCount) {
    std::vector<float> kernel;
    kernel.reserve(static_cast<size_t>(sampleCount) * 4);
    for (int i = 0; i < sampleCount; ++i) {
        float x = HashFloat(i, 0) * 2.0f - 1.0f;
        float y = HashFloat(i, 1) * 2.0f - 1.0f;
        float z = HashFloat(i, 2);  // hemisphere: z >= 0

        float len = std::sqrt(x * x + y * y + z * z);
        if (len < 0.001f) {
            x   = 0;
            y   = 0;
            z   = 1;
            len = 1;
        }
        x /= len;
        y /= len;
        z /= len;

        // Quadratic scale: more samples near the surface
        float scale = static_cast<float>(i) / static_cast<float>(sampleCount);
        scale       = 0.1f + scale * scale * 0.9f;

        kernel.push_back(x * scale);
        kernel.push_back(y * scale);
        kernel.push_back(z * scale);
        kernel.push_back(0.0f);
    }
    return kernel;
}

}  // namespace sdl3cpp::services::impl
