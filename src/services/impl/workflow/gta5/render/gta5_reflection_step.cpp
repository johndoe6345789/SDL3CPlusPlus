#include "services/interfaces/workflow/gta5/render/gta5_reflection_step.hpp"

#include "services/interfaces/workflow/gta5/stream/gta5_proxy.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_reflection.hpp"
#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint32_t kMirrored =
    1u | 1u << static_cast<int>(Gta5ProxyKind::Reflection) |
    1u << static_cast<int>(Gta5ProxyKind::Water);

}  // namespace

WorkflowGta5ReflectionDrawStep::WorkflowGta5ReflectionDrawStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5ReflectionDrawStep::GetPluginId() const {
    return "gta5.reflection.draw";
}

void WorkflowGta5ReflectionDrawStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!state_ || context.GetBool("frame_skip", false)) return;
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    // Half the render size: the swell blurs it anyway.
    const auto width = context.Get<uint32_t>("render_width", 0u) / 2;
    const auto height = context.Get<uint32_t>("render_height", 0u) / 2;
    if (!device || width == 0 || height == 0 ||
        !Ensure(device, width, height)) {
        return;
    }
    if (!loaded_) {
        loaded_ = true;
        water_ = LoadGta5WaterQuads(Gta5ParameterOr(step, "water_file", ""));
    }
    // The camera reflected in the water it is over: the view up through it.
    glm::vec3 eye = context.Get<glm::vec3>("render.camera_pos", glm::vec3(0));
    const float level = ChooseGta5MirrorHeight(water_, eye);
    const glm::mat4 view =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f)) *
        Gta5MirrorAt(level);
    const auto proj = context.Get<glm::mat4>("render.proj_matrix", view);
    eye.y = 2.f * level - eye.y;
    Gta5CullOptions options;
    options.kinds = kMirrored;  // scenery too, only the large of it
    options.sizeRatio = Gta5NumberOr(step, "size_ratio", 0.015f);
    BuildGta5InstanceBatch(*state_, proj * view, eye, options, batch_);
    // Clipped at the water line, whose kept side is above it.
    const glm::mat4 clipped = Gta5ObliqueProjection(
        proj, glm::transpose(glm::inverse(view)) *
                  glm::vec4(0.f, 1.f, 0.f, -level));
    SDL_GPUCommandBuffer* cmd = UploadGta5InstanceBatch(device, batch_)
                                    ? SDL_AcquireGPUCommandBuffer(device)
                                    : nullptr;
    if (!cmd) return;
    const int drawn = DrawMirror(step, context, cmd, view, clipped, eye);
    SDL_SubmitGPUCommandBuffer(cmd);
    context.Set<SDL_GPUTexture*>("gta5.reflection.texture", colour_);
    context.Set<float>("gta5.reflection.height", level);
    // Once, when something is first mirrored: silence says nothing was.
    if (logger_ && !logged_ && drawn > 0) {
        logged_ = true;
        logger_->Info("gta5.reflection.draw: " + std::to_string(drawn) +
                      " mirrored draws into a " + std::to_string(width) +
                      "x" + std::to_string(height) + " target, mirror at " +
                      std::to_string(static_cast<int>(level)) + " m");
    }
}

}  // namespace sdl3cpp::services::impl
