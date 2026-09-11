#include "services/interfaces/workflow/gta5/gta5_effects_step.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint32_t kMaxVertices = 24000;  // 4000 quads at once

}  // namespace

WorkflowGta5EffectsDrawStep::WorkflowGta5EffectsDrawStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5EffectsDrawStep::GetPluginId() const {
    return "gta5.effects.draw";
}

void WorkflowGta5EffectsDrawStep::Execute(const WorkflowStepDefinition&,
                                          WorkflowContext& context) {
    if (!state_ || context.GetBool("frame_skip", false)) return;
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "gpu_pipeline_gta5_effects", nullptr);
    if (!device || !pass || !cmd || !pipeline) return;
    const Gta5EffectsPtr effects = Gta5EffectsOf(context);
    if (!tried_) {
        tried_ = true;
        SetUpGta5Effects(*effects, *state_, device, vertices_, staging_,
                         kMaxVertices);
        if (logger_) logger_->Info("gta5.effects.draw: ready");
    }
    if (!effects->ready || !vertices_ || !staging_) return;
    UpdateGta5Effects(*effects,
                      std::clamp(context.Get<float>("physics_dt", 1.f / 60.f),
                                 0.f, 0.1f));
    // Facing the camera: the view's own right and up, in the world.
    const auto view =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    const auto quads = BuildGta5EffectQuads(
        *effects, glm::vec3(view[0][0], view[1][0], view[2][0]),
        glm::vec3(view[0][1], view[1][1], view[2][1]));
    const auto count =
        static_cast<std::uint32_t>(std::min<std::size_t>(quads.size(),
                                                         kMaxVertices));
    if (count == 0) return;
    const auto bytes = static_cast<Uint32>(count * sizeof(BspRenderVertex));
    void* mapped = SDL_MapGPUTransferBuffer(device, staging_, true);
    if (!mapped) return;
    std::memcpy(mapped, quads.data(), bytes);
    SDL_UnmapGPUTransferBuffer(device, staging_);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    const SDL_GPUTransferBufferLocation from = {staging_, 0};
    const SDL_GPUBufferRegion to = {vertices_, 0, bytes};
    SDL_UploadToGPUBuffer(copy, &from, &to, true);
    SDL_EndGPUCopyPass(copy);
    const glm::mat4 viewProj =
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.f)) * view;
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    const SDL_GPUBufferBinding binding = {vertices_, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
    const SDL_GPUTextureSamplerBinding art = {effects->atlas,
                                              effects->sampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &art, 1);
    SDL_PushGPUVertexUniformData(cmd, 0, &viewProj, sizeof(viewProj));
    SDL_DrawGPUPrimitives(pass, count, 1, 0, 0);
}

}  // namespace sdl3cpp::services::impl
