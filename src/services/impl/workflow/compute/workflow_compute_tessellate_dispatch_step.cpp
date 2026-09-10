#include "services/interfaces/workflow/compute/workflow_compute_tessellate_dispatch_step.hpp"
#include "services/interfaces/workflow/compute/compute_tessellate_grid.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowComputeTessellateDispatchStep::WorkflowComputeTessellateDispatchStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowComputeTessellateDispatchStep::GetPluginId() const {
    return "compute.tessellate.dispatch";
}

void WorkflowComputeTessellateDispatchStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver stepParams;
    WorkflowStepIoResolver resolver;
    const TessellationGridParams params = ReadTessellationGridParams(step);

    const auto* pipelineKeyParam =
        stepParams.FindParameter(step, "pipeline_key");
    const std::string pipelineKey =
        (pipelineKeyParam &&
         pipelineKeyParam->type == WorkflowParameterValue::Type::String)
            ? pipelineKeyParam->stringValue
            : "compute_pipeline";

    const std::string dispTexKey =
        resolver.GetRequiredInputKey(step, "displacement_texture");
    auto* dispTexture =
        context.Get<SDL_GPUTexture*>(dispTexKey + "_gpu", nullptr);
    auto* dispSampler =
        context.Get<SDL_GPUSampler*>(dispTexKey + "_sampler", nullptr);
    if (!dispTexture || !dispSampler) {
        throw std::runtime_error(
            "compute.tessellate.dispatch: Displacement texture '" + dispTexKey +
            "' not found in context");
    }

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error(
            "compute.tessellate.dispatch: GPU device not found in context");
    }

    auto* pipeline = context.Get<SDL_GPUComputePipeline*>(pipelineKey, nullptr);
    if (!pipeline) {
        throw std::runtime_error(
            "compute.tessellate.dispatch: Compute pipeline '" + pipelineKey +
            "' not found in context. Run compute.pipeline.create first.");
    }

    const TessellationGridBuffers buffers =
        CreateAndUploadTessellationGrid(device, params.subdivisions);
    DispatchTessellationCompute(device, pipeline, dispTexture, dispSampler,
                                params, buffers);
    // Pipeline ownership stays with compute.pipeline.create; not released.

    PublishTessellationGrid(context, params, buffers);

    if (logger_) {
        logger_->Info("compute.tessellate.dispatch: '" + params.name +
                      "' created (" + std::to_string(buffers.vertexCount) +
                      " verts, " + std::to_string(buffers.indexCount) +
                      " indices, " + std::to_string(params.subdivisions) + "x" +
                      std::to_string(params.subdivisions) +
                      " subdivisions, disp=" +
                      std::to_string(params.displacementStrength) + ")");
    }
}

}  // namespace sdl3cpp::services::impl
