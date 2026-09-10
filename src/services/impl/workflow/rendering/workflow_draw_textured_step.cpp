#include "services/interfaces/workflow/rendering/workflow_draw_textured_step.hpp"
#include "services/interfaces/workflow/rendering/draw_textured_params.hpp"
#include "services/interfaces/workflow/rendering/draw_textured_resources.hpp"
#include "services/interfaces/workflow/rendering/draw_textured_transform.hpp"
#include "services/interfaces/workflow/rendering/draw_textured_uniforms.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

WorkflowDrawTexturedStep::WorkflowDrawTexturedStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowDrawTexturedStep::GetPluginId() const {
    return "draw.textured";
}

void WorkflowDrawTexturedStep::Execute(const WorkflowStepDefinition& step,
                                       WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    const DrawTexturedParams params = ReadDrawTexturedParams(step);

    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline =
        context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_textured", nullptr);
    if (!pass || !cmd || !pipeline) {
        if (logger_) {
            logger_->Warn(
                "draw.textured: Missing render pass, command "
                "buffer, or textured pipeline");
        }
        return;
    }

    DrawTexturedResources resources;
    if (!ResolveDrawTexturedResources(context, params, logger_, resources)) {
        return;
    }

    const DrawTexturedTransform transform = BuildDrawTexturedTransform(params);

    rendering::VertexUniformData vu;
    rendering::FragmentUniformData fu;
    BuildDrawTexturedUniforms(context, transform, params.roughness,
                              params.metallic, vu, fu);

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    BindDrawTexturedSamplers(pass, context, resources.texture,
                             resources.sampler);

    SDL_GPUBufferBinding vb_binding = {};
    vb_binding.buffer               = resources.vb;
    SDL_BindGPUVertexBuffers(pass, 0, &vb_binding, 1);
    SDL_GPUBufferBinding ib_binding = {};
    ib_binding.buffer               = resources.ib;
    SDL_BindGPUIndexBuffer(pass, &ib_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
    SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));
    SDL_DrawGPUIndexedPrimitives(pass, resources.indexCount, 1, 0, 0, 0);
}

}  // namespace sdl3cpp::services::impl
