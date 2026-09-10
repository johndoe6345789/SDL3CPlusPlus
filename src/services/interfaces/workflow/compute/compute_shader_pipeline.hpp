#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL_gpu.h>
#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Expands a leading `~` against $HOME, as shell paths conventionally allow.
std::string ExpandComputeShaderPath(const std::string& path);

/**
 * @brief Reads a compiled compute shader binary from disk.
 * @throws std::runtime_error if the file cannot be opened.
 */
std::vector<uint8_t> LoadComputeShaderBinary(const std::string& path);

/// The entrypoint/format pair the GPU device's active backend expects.
struct ComputeShaderFormat {
    SDL_GPUShaderFormat format;
    const char* entrypoint;
};

/// SPIR-V/"main" on Vulkan and every other backend; MSL/"main0" on Metal.
ComputeShaderFormat DetectComputeShaderFormat(SDL_GPUDevice* device);

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

/// compute.pipeline.create's resource-count parameters plus the context key
/// it publishes the pipeline under.
struct ComputePipelineCreateParams {
    ComputePipelineResourceCounts counts;
    std::string pipelineKey = "compute_pipeline";
};

ComputePipelineCreateParams ReadComputePipelineCreateParams(
    const WorkflowStepDefinition& step);

}  // namespace sdl3cpp::services::impl
