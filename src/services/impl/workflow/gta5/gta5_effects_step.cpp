#include "services/interfaces/workflow/gta5/gta5_effects_step.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5EffectsDrawStep::WorkflowGta5EffectsDrawStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5EffectsDrawStep::GetPluginId() const {
    return "gta5.effects.draw";
}

void WorkflowGta5EffectsDrawStep::Execute(const WorkflowStepDefinition&,
                                          WorkflowContext& context) {
    if (!state_ || context.GetBool("frame_skip", false)) return;
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "gpu_pipeline_gta5_effects", nullptr);
    const Gta5EffectsPtr effects = Gta5EffectsOf(context);
    if (!pass || !cmd || !pipeline) return;
    if (!effects->ready || effects->count == 0) return;
    const auto view =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    const glm::mat4 viewProj =
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.f)) * view;
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    const SDL_GPUBufferBinding binding = {effects->vertices, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
    const SDL_GPUTextureSamplerBinding art = {effects->atlas,
                                              effects->sampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &art, 1);
    SDL_PushGPUVertexUniformData(cmd, 0, &viewProj, sizeof(viewProj));
    SDL_DrawGPUPrimitives(pass, effects->count, 1, 0, 0);
}

}  // namespace sdl3cpp::services::impl
