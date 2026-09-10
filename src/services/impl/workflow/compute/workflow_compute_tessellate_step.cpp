#include "services/interfaces/workflow/compute/workflow_compute_tessellate_step.hpp"
#include "services/interfaces/workflow/compute/compute_shader_pipeline.hpp"
#include "services/interfaces/workflow/compute/compute_tessellate_grid.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowComputeTessellateStep::WorkflowComputeTessellateStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowComputeTessellateStep::GetPluginId() const {
    return "compute.tessellate";
}

void WorkflowComputeTessellateStep::Execute(const WorkflowStepDefinition& step,
                                            WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const TessellationGridParams params = ReadTessellationGridParams(step);

    const std::string dispTexKey =
        resolver.GetRequiredInputKey(step, "displacement_texture");
    const std::string shaderPathKey =
        resolver.GetRequiredInputKey(step, "compute_shader_path");

    const auto* shaderPath = context.TryGet<std::string>(shaderPathKey);
    if (!shaderPath) {
        throw std::runtime_error(
            "compute.tessellate: compute_shader_path not found in context "
            "key '" +
            shaderPathKey + "'");
    }

    auto* dispTexture =
        context.Get<SDL_GPUTexture*>(dispTexKey + "_gpu", nullptr);
    auto* dispSampler =
        context.Get<SDL_GPUSampler*>(dispTexKey + "_sampler", nullptr);
    if (!dispTexture || !dispSampler) {
        throw std::runtime_error("compute.tessellate: Displacement texture '" +
                                 dispTexKey + "' not found in context");
    }

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error(
            "compute.tessellate: GPU device not found in context");
    }

    const auto shaderData =
        LoadComputeShaderBinary(ExpandComputeShaderPath(*shaderPath));
    auto* pipeline = CreateComputePipelineFromBinary(
        device, shaderData, ComputePipelineResourceCounts{},
        "compute.tessellate");

    const TessellationGridBuffers buffers =
        CreateAndUploadTessellationGrid(device, params.subdivisions);
    DispatchTessellationCompute(device, pipeline, dispTexture, dispSampler,
                                params, buffers);
    SDL_ReleaseGPUComputePipeline(device, pipeline);  // done after dispatch

    PublishTessellationGrid(context, params, buffers);

    if (logger_) {
        logger_->Info("compute.tessellate: '" + params.name + "' created (" +
                      std::to_string(buffers.vertexCount) + " verts, " +
                      std::to_string(buffers.indexCount) + " indices, " +
                      std::to_string(params.subdivisions) + "x" +
                      std::to_string(params.subdivisions) +
                      " subdivisions, disp=" +
                      std::to_string(params.displacementStrength) + ")");
    }
}

}  // namespace sdl3cpp::services::impl
