#include "services/interfaces/workflow/gta5/gta5_effects_prepare_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_effects_spawn.hpp"

#include <algorithm>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint32_t kMaxVertices = 24000;  // 4000 quads at once

}  // namespace

WorkflowGta5EffectsPrepareStep::WorkflowGta5EffectsPrepareStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5EffectsPrepareStep::GetPluginId() const {
    return "gta5.effects.prepare";
}

void WorkflowGta5EffectsPrepareStep::Execute(const WorkflowStepDefinition&,
                                             WorkflowContext& context) {
    if (!state_ || context.GetBool("frame_skip", false)) return;
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) return;
    const Gta5EffectsPtr effects = Gta5EffectsOf(context);
    if (!tried_) {
        tried_ = true;
        SetUpGta5Effects(*effects, *state_, device, kMaxVertices);
        if (logger_) logger_->Info("gta5.effects: ready");
    }
    effects->count = 0;
    if (!effects->ready || !effects->vertices) return;
    UpdateGta5Effects(*effects,
                      std::clamp(context.Get<float>("physics_dt", 1.f / 60.f),
                                 0.f, 0.1f));
    // Facing the camera: the view right and up, in the world.
    const auto view =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    const auto quads = BuildGta5EffectQuads(
        *effects, glm::vec3(view[0][0], view[1][0], view[2][0]),
        glm::vec3(view[0][1], view[1][1], view[2][1]));
    if (quads.empty()) return;
    const auto count = static_cast<std::uint32_t>(
        std::min<std::size_t>(quads.size(), kMaxVertices));
    const auto bytes =
        static_cast<std::uint32_t>(count * sizeof(BspRenderVertex));
    const bool staged = state_->uploads.StageBuffer(
        device, quads.data(), bytes, effects->vertices, 0);
    if (staged) effects->count = count;
}

}  // namespace sdl3cpp::services::impl
