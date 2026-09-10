#include "services/interfaces/workflow/compute/compute_shader_pipeline.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <cstdlib>
#include <fstream>
#include <stdexcept>

namespace sdl3cpp::services::impl {
namespace {

int IntParameter(const WorkflowStepParameterResolver& params,
                 const WorkflowStepDefinition& step, const char* key,
                 int fallback) {
    const auto* p       = params.FindParameter(step, key);
    const bool isNumber = p && p->type == WorkflowParameterValue::Type::Number;
    return isNumber ? static_cast<int>(p->numberValue) : fallback;
}

}  // namespace

std::string ExpandComputeShaderPath(const std::string& path) {
    if (path.empty() || path[0] != '~') {
        return path;
    }
    const char* home = std::getenv("HOME");
    return home ? std::string(home) + path.substr(1) : path;
}

std::vector<uint8_t> LoadComputeShaderBinary(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open compute shader: " + path);
    }
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> data(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

ComputeShaderFormat DetectComputeShaderFormat(SDL_GPUDevice* device) {
    const char* driver = SDL_GetGPUDeviceDriver(device);
    const bool isMetal = driver && std::string(driver) == "metal";
    return {isMetal ? SDL_GPU_SHADERFORMAT_MSL : SDL_GPU_SHADERFORMAT_SPIRV,
            isMetal ? "main0" : "main"};
}

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

ComputePipelineCreateParams ReadComputePipelineCreateParams(
    const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;
    ComputePipelineCreateParams p;
    auto& counts = p.counts;
    counts.numSamplers =
        IntParameter(params, step, "num_samplers", counts.numSamplers);
    counts.numReadWriteStorageBuffers = IntParameter(
        params, step, "num_storage_buffers", counts.numReadWriteStorageBuffers);
    counts.numUniformBuffers =
        IntParameter(params, step, "num_uniforms", counts.numUniformBuffers);
    counts.threadcountX =
        IntParameter(params, step, "threadcount_x", counts.threadcountX);
    counts.threadcountY =
        IntParameter(params, step, "threadcount_y", counts.threadcountY);
    counts.threadcountZ =
        IntParameter(params, step, "threadcount_z", counts.threadcountZ);

    const auto* pipelineKeyParam = params.FindParameter(step, "pipeline_key");
    if (pipelineKeyParam &&
        pipelineKeyParam->type == WorkflowParameterValue::Type::String) {
        p.pipelineKey = pipelineKeyParam->stringValue;
    }
    return p;
}

}  // namespace sdl3cpp::services::impl
