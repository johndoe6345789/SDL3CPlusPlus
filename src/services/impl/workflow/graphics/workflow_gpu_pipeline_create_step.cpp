#include "services/interfaces/workflow/graphics/workflow_gpu_pipeline_create_step.hpp"
#include "services/interfaces/workflow/graphics/gpu_graphics_pipeline.hpp"

#include <SDL3/SDL_gpu.h>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowGpuPipelineCreateStep::WorkflowGpuPipelineCreateStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGpuPipelineCreateStep::GetPluginId() const {
    return "graphics.gpu.pipeline.create";
}

void WorkflowGpuPipelineCreateStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    const GpuPipelineCreateParams p = ReadGpuPipelineCreateParams(step);

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error(
            "graphics.gpu.pipeline.create: GPU device not found in "
            "context");
    }

    const GpuPipelineShaders shaders = RequireGpuPipelineShaders(context, p);
    auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    GpuVertexAttributeLayout layout;
    SDL_GPUColorTargetDescription colorTarget;
    SDL_GPUGraphicsPipelineCreateInfo info = BuildGraphicsPipelineCreateInfo(
        p, shaders.vertex, shaders.fragment, device, window, layout,
        colorTarget);

    SDL_GPUGraphicsPipeline* pipeline =
        SDL_CreateGPUGraphicsPipeline(device, &info);

    // Optionally release shaders (they're baked into the pipeline now).
    if (p.releaseShaders) {
        SDL_ReleaseGPUShader(device, shaders.vertex);
        SDL_ReleaseGPUShader(device, shaders.fragment);
        // Remove from context so nobody uses stale pointers.
        context.Remove(p.vertexShaderKey);
        context.Remove(p.fragmentShaderKey);
    }

    if (!pipeline) {
        throw std::runtime_error(
            "graphics.gpu.pipeline.create: Failed to create graphics "
            "pipeline: " + std::string(SDL_GetError()));
    }

    context.Set<SDL_GPUGraphicsPipeline*>(p.pipelineKey, pipeline);

    if (logger_) {
        logger_->Trace(
            "WorkflowGpuPipelineCreateStep", "Execute",
            "pipeline_key=" + p.pipelineKey +
                ", format=" + p.vertexFormat + ", cull=" + p.cullMode +
                ", color_targets=" +
                std::to_string(p.numColorTargets) +
                ", depth_bias=" + std::to_string(p.depthBias),
            "Graphics pipeline created");
    }
}

}  // namespace sdl3cpp::services::impl
