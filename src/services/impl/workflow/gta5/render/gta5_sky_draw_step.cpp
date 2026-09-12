#include "services/interfaces/workflow/gta5/render/gta5_sky_draw_step.hpp"

#include "services/interfaces/workflow/gta5/render/gta5_sky_uniforms.hpp"
#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"

#include <SDL3/SDL_gpu.h>

#include <glm/glm.hpp>

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5SkyDrawStep::WorkflowGta5SkyDrawStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGta5SkyDrawStep::GetPluginId() const {
    return "gta5.sky.draw";
}

void WorkflowGta5SkyDrawStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        Gta5ParameterOr(step, "pipeline_key", "gpu_pipeline_gta5_sky"),
        nullptr);
    if (!pass || !cmd || !pipeline) return;

    const Gta5SkyUniforms sky = BuildGta5SkyUniforms(step, context);
    // The city fogs to this. Published rather than repeated in the model
    // shader, because where the two disagree the join shows as a band
    // across the horizon.
    context.Set<glm::vec3>("gta5.sky.horizon", glm::vec3(sky.horizon));

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_PushGPUFragmentUniformData(cmd, 0, &sky, sizeof(sky));
    // Three vertices, no buffers: the triangle is built from
    // gl_VertexIndex in the vertex stage.
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
}

}  // namespace sdl3cpp::services::impl
