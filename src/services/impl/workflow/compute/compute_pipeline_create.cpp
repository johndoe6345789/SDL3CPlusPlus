#include "services/interfaces/workflow/compute/compute_pipeline_create.hpp"
#include "services/interfaces/workflow/compute/compute_shader_format.hpp"

#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

SDL_GPUComputePipeline* CreateComputePipelineFromBinary(
    SDL_GPUDevice* device, const std::vector<uint8_t>& shaderData,
    const ComputePipelineResourceCounts& counts, const char* pluginId) {
    const ComputeShaderFormat shaderFormat = DetectComputeShaderFormat(device);

    SDL_GPUComputePipelineCreateInfo info = {};
    info.code                             = shaderData.data();
    info.code_size                        = shaderData.size();
    info.entrypoint                       = shaderFormat.entrypoint;
    info.format                           = shaderFormat.format;
    info.num_samplers = static_cast<Uint32>(counts.numSamplers);
    info.num_readwrite_storage_buffers =
        static_cast<Uint32>(counts.numReadWriteStorageBuffers);
    info.num_uniform_buffers = static_cast<Uint32>(counts.numUniformBuffers);
    info.threadcount_x       = static_cast<Uint32>(counts.threadcountX);
    info.threadcount_y       = static_cast<Uint32>(counts.threadcountY);
    info.threadcount_z       = static_cast<Uint32>(counts.threadcountZ);

    auto* pipeline = SDL_CreateGPUComputePipeline(device, &info);
    if (!pipeline) {
        throw std::runtime_error(std::string(pluginId) +
                                 ": Failed to create compute pipeline: " +
                                 std::string(SDL_GetError()));
    }
    return pipeline;
}

}  // namespace sdl3cpp::services::impl
