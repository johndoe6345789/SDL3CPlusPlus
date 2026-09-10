#include "services/interfaces/workflow/quake3/workflow_q3_sky_draw_step.hpp"

#include "services/interfaces/workflow/quake3/q3_sky_scroll.hpp"
#include "services/interfaces/workflow/quake3/q3_sky_uniforms.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

WorkflowQ3SkyDrawStep::WorkflowQ3SkyDrawStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3SkyDrawStep::GetPluginId() const {
    return "q3.sky.draw";
}

void WorkflowQ3SkyDrawStep::Execute(const WorkflowStepDefinition&,
                                    WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline =
        context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_bsp", nullptr);
    if (!pass || !cmd || !pipeline) return;

    if (!resources_.attempted) {
        if (!InitSkyResources(context, resources_) && logger_) {
            logger_->Warn("q3.sky.draw: no sky shader in this map's BSP");
        }
    }
    if (!resources_.cloudTex || !resources_.vertexBuffer) return;

    // time.frame_delta, first step of the q3 frame, publishes
    // frame.elapsed_time; frame.elapsed is the seed pipeline's name for
    // the same thing and is never set here, so reading only that left
    // the sky frozen at t=0.
    const auto elapsed = static_cast<float>(context.GetDouble(
        "frame.elapsed_time", context.GetDouble("frame.elapsed", 0.0)));
    UpdateSkyScroll(context.Get<SDL_GPUDevice*>("gpu_device", nullptr),
                    resources_, elapsed);

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    SDL_GPUBufferBinding vbBinding = {};
    vbBinding.buffer               = resources_.vertexBuffer;
    SDL_BindGPUVertexBuffers(pass, 0, &vbBinding, 1);
    SDL_GPUBufferBinding ibBinding = {};
    ibBinding.buffer               = resources_.indexBuffer;
    SDL_BindGPUIndexBuffer(pass, &ibBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_GPUTextureSamplerBinding bindings[4] = {};
    bindings[0].texture                      = resources_.cloudTex;
    bindings[0].sampler                      = resources_.cloudSampler;
    for (int i = 1; i < 4; ++i) {
        bindings[i].texture = resources_.whiteTex;
        bindings[i].sampler = resources_.whiteSampler;
    }
    SDL_BindGPUFragmentSamplers(pass, 0, bindings, 4);

    const auto vu = BuildSkyVertexUniforms(
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f)),
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f)));
    const auto fu = BuildSkyFragmentUniforms();
    SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
    SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));

    SDL_DrawGPUIndexedPrimitives(pass, resources_.indexCount, 1, 0, 0, 0);
}

}  // namespace sdl3cpp::services::impl
