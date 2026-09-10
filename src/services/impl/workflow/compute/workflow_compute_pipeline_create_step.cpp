#include "services/interfaces/workflow/compute/workflow_compute_pipeline_create_step.hpp"
#include "services/interfaces/workflow/compute/compute_shader_pipeline.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowComputePipelineCreateStep::WorkflowComputePipelineCreateStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowComputePipelineCreateStep::GetPluginId() const {
    return "compute.pipeline.create";
}

void WorkflowComputePipelineCreateStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    const ComputePipelineCreateParams params =
        ReadComputePipelineCreateParams(step);

    WorkflowStepIoResolver resolver;
    const std::string shaderPathKey =
        resolver.GetRequiredInputKey(step, "shader_path");
    const auto* shaderPath = context.TryGet<std::string>(shaderPathKey);
    if (!shaderPath) {
        throw std::runtime_error(
            "compute.pipeline.create: shader_path not found in context key '" +
            shaderPathKey + "'");
    }
    const std::string resolvedShader = ExpandComputeShaderPath(*shaderPath);

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error(
            "compute.pipeline.create: GPU device not found in context");
    }

    auto* pipeline = CreateComputePipelineFromBinary(
        device, LoadComputeShaderBinary(resolvedShader), params.counts,
        "compute.pipeline.create");
    context.Set<SDL_GPUComputePipeline*>(params.pipelineKey, pipeline);

    if (logger_) {
        const auto& c = params.counts;
        logger_->Info(
            "compute.pipeline.create: Pipeline '" + params.pipelineKey +
            "' created from '" + resolvedShader +
            "' (threads=" + std::to_string(c.threadcountX) + "x" +
            std::to_string(c.threadcountY) + "x" +
            std::to_string(c.threadcountZ) +
            ", samplers=" + std::to_string(c.numSamplers) +
            ", storage=" + std::to_string(c.numReadWriteStorageBuffers) +
            ", uniforms=" + std::to_string(c.numUniformBuffers) + ")");
    }
}

}  // namespace sdl3cpp::services::impl
