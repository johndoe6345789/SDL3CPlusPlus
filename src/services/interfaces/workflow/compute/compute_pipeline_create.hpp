#pragma once

#include <SDL3/SDL_gpu.h>
#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Resource counts a compute pipeline declares it will bind.
struct ComputePipelineResourceCounts {
    int numSamplers                = 1;
    int numReadWriteStorageBuffers = 1;
    int numUniformBuffers          = 1;
    int threadcountX               = 8;
    int threadcountY               = 8;
    int threadcountZ               = 1;
};

/**
 * @brief Creates a compute pipeline from a shader binary already in memory.
 * @throws std::runtime_error (naming `pluginId`) if pipeline creation fails.
 */
SDL_GPUComputePipeline* CreateComputePipelineFromBinary(
    SDL_GPUDevice* device, const std::vector<uint8_t>& shaderData,
    const ComputePipelineResourceCounts& counts, const char* pluginId);

}  // namespace sdl3cpp::services::impl
