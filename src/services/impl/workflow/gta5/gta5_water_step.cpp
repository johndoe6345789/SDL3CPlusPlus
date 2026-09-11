#include "services/interfaces/workflow/gta5/gta5_water_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_water.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5WaterDrawStep::WorkflowGta5WaterDrawStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5WaterDrawStep::GetPluginId() const {
    return "gta5.water.draw";
}

void WorkflowGta5WaterDrawStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    if (!state_ || context.GetBool("frame_skip", false)) return;
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "gpu_pipeline_gta5_water", nullptr);
    if (!device || !pass || !cmd || !pipeline) return;
    if (!tried_) Load(step, device);
    if (!count_) return;
    if (!staged_) {  // the first frame only stages it
        staged_ = true;
        return;
    }
    auto* reflection =
        context.Get<SDL_GPUTexture*>("gta5.reflection.texture", nullptr);
    // Any texture will do in the slot while there is no reflection.
    auto* texture =
        reflection ? reflection
                   : context.Get<SDL_GPUTexture*>("walls_texture_gpu", nullptr);
    auto* sampler =
        context.Get<SDL_GPUSampler*>("walls_texture_sampler", nullptr);
    if (!texture || !sampler) return;
    const glm::mat4 viewProj =
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.f)) *
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    const Gta5WaterUniforms frag =
        BuildGta5WaterUniforms(context, reflection != nullptr);
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    const SDL_GPUBufferBinding binding = {vertices_, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
    const SDL_GPUTextureSamplerBinding textures = {texture, sampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &textures, 1);
    SDL_PushGPUVertexUniformData(cmd, 0, glm::value_ptr(viewProj),
                                 sizeof(viewProj));
    SDL_PushGPUFragmentUniformData(cmd, 0, &frag, sizeof(frag));
    SDL_DrawGPUPrimitives(pass, count_, 1, 0, 0);
}

}  // namespace sdl3cpp::services::impl
