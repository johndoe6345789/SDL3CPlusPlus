#include "services/interfaces/workflow/compute/workflow_compute_pipeline_create_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowComputePipelineCreateStep::WorkflowComputePipelineCreateStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowComputePipelineCreateStep::GetPluginId() const {
    return "compute.pipeline.create";
}

void WorkflowComputePipelineCreateStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver params;
    WorkflowStepIoResolver resolver;

    auto getInt = [&](const char* pname, int def) -> int {
        const auto* p = params.FindParameter(step, pname);
        return (p && p->type == WorkflowParameterValue::Type::Number) ? static_cast<int>(p->numberValue) : def;
    };
    auto getStr = [&](const char* pname, const std::string& def) -> std::string {
        const auto* p = params.FindParameter(step, pname);
        return (p && p->type == WorkflowParameterValue::Type::String) ? p->stringValue : def;
    };

    // Pipeline configuration parameters
    const int num_samplers = getInt("num_samplers", 1);
    const int num_storage_buffers = getInt("num_storage_buffers", 1);
    const int num_uniforms = getInt("num_uniforms", 1);
    const int threadcount_x = getInt("threadcount_x", 8);
    const int threadcount_y = getInt("threadcount_y", 8);
    const int threadcount_z = getInt("threadcount_z", 1);
    const std::string pipeline_key = getStr("pipeline_key", "compute_pipeline");

    // Get shader path from inputs (context key)
    const std::string shaderPathKey = resolver.GetRequiredInputKey(step, "shader_path");

    const auto* shader_path = context.TryGet<std::string>(shaderPathKey);
    if (!shader_path) {
        throw std::runtime_error("compute.pipeline.create: shader_path not found in context key '" + shaderPathKey + "'");
    }

    // Resolve ~ in path
    std::string resolved_shader = *shader_path;
    if (!resolved_shader.empty() && resolved_shader[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) resolved_shader = std::string(home) + resolved_shader.substr(1);
    }

    // Get GPU device
    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error("compute.pipeline.create: GPU device not found in context");
    }

    // Load compute shader binary
    std::ifstream file(resolved_shader, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("compute.pipeline.create: Failed to open shader: " + resolved_shader);
    }
    std::streamsize shader_size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> shader_data(shader_size);
    file.read(reinterpret_cast<char*>(shader_data.data()), shader_size);

    // Detect shader format based on GPU driver
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_SPIRV;
    const char* driver = SDL_GetGPUDeviceDriver(device);
    if (driver && std::string(driver) == "metal") {
        format = SDL_GPU_SHADERFORMAT_MSL;
    }
    const char* entrypoint = (format == SDL_GPU_SHADERFORMAT_MSL) ? "main0" : "main";

    // Create compute pipeline
    SDL_GPUComputePipelineCreateInfo pipeline_info = {};
    pipeline_info.code = shader_data.data();
    pipeline_info.code_size = shader_data.size();
    pipeline_info.entrypoint = entrypoint;
    pipeline_info.format = format;
    pipeline_info.num_samplers = static_cast<Uint32>(num_samplers);
    pipeline_info.num_readwrite_storage_buffers = static_cast<Uint32>(num_storage_buffers);
    pipeline_info.num_uniform_buffers = static_cast<Uint32>(num_uniforms);
    pipeline_info.threadcount_x = static_cast<Uint32>(threadcount_x);
    pipeline_info.threadcount_y = static_cast<Uint32>(threadcount_y);
    pipeline_info.threadcount_z = static_cast<Uint32>(threadcount_z);

    SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(device, &pipeline_info);
    if (!pipeline) {
        throw std::runtime_error("compute.pipeline.create: Failed to create compute pipeline: " +
                                 std::string(SDL_GetError()));
    }

    // Store pipeline in context under the specified key
    context.Set<SDL_GPUComputePipeline*>(pipeline_key, pipeline);

    if (logger_) {
        logger_->Info("compute.pipeline.create: Pipeline '" + pipeline_key + "' created from '" +
                     resolved_shader + "' (threads=" + std::to_string(threadcount_x) + "x" +
                     std::to_string(threadcount_y) + "x" + std::to_string(threadcount_z) +
                     ", samplers=" + std::to_string(num_samplers) +
                     ", storage=" + std::to_string(num_storage_buffers) +
                     ", uniforms=" + std::to_string(num_uniforms) + ")");
    }
}

}  // namespace sdl3cpp::services::impl
