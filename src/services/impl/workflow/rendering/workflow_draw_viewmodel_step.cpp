#include "services/interfaces/workflow/rendering/workflow_draw_viewmodel_step.hpp"
#include "services/interfaces/workflow/rendering/viewmodel_draw.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

WorkflowDrawViewmodelStep::WorkflowDrawViewmodelStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowDrawViewmodelStep::GetPluginId() const {
    return "draw.viewmodel";
}

void WorkflowDrawViewmodelStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    const ViewmodelDrawParams params = ReadViewmodelDrawParams(step);

    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer",
                                                   nullptr);
    auto* pipeline =
        context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_textured",
                                              nullptr);
    if (!pass || !cmd || !pipeline) return;

    const auto mesh =
        TryGetViewmodelMesh(context, params.meshName, logger_);
    if (!mesh) return;

    const ViewmodelUniforms uniforms = BuildViewmodelUniforms(context,
                                                               params);

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    BindViewmodelTexture(context, pass, params.texName);

    SDL_GPUBufferBinding vbBinding = {};
    vbBinding.buffer = mesh->vertexBuffer;
    SDL_BindGPUVertexBuffers(pass, 0, &vbBinding, 1);
    SDL_GPUBufferBinding ibBinding = {};
    ibBinding.buffer = mesh->indexBuffer;
    SDL_BindGPUIndexBuffer(pass, &ibBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_PushGPUVertexUniformData(cmd, 0, &uniforms.vertex,
                                 sizeof(uniforms.vertex));
    SDL_PushGPUFragmentUniformData(cmd, 0, &uniforms.fragment,
                                   sizeof(uniforms.fragment));
    SDL_DrawGPUIndexedPrimitives(pass, mesh->indexCount, 1, 0, 0, 0);
}

}  // namespace sdl3cpp::services::impl
