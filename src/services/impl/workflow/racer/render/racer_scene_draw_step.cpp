#include "services/interfaces/workflow/racer/render/racer_scene_draw_step.hpp"

#include "services/interfaces/workflow/racer/racer_step_params.hpp"
#include "services/interfaces/workflow/racer/render/racer_model_draw.hpp"

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kHideNearEye = 8.f;

}  // namespace
WorkflowRacerSceneDrawStep::WorkflowRacerSceneDrawStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowRacerSceneDrawStep::GetPluginId() const {
    return "racer.scene.draw";
}

void WorkflowRacerSceneDrawStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    if (context.GetBool("frame_skip", false) || !state_->loaded) return;
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* opaque =
        context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_racer", nullptr);
    auto* blend = context.Get<SDL_GPUGraphicsPipeline*>(
        "gpu_pipeline_racer_blend", nullptr);
    if (!pass || !cmd || !opaque) return;
    const glm::mat4 view =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    const glm::mat4 proj =
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.f));
    RacerDrawPass d{pass, cmd, {proj * view, glm::mat4(1.f)}};
    const glm::vec3 eye =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f));
    const glm::vec3 fog = state_->fogColour;
    const RacerFragmentUniforms fragment{
        {fog.r, fog.g, fog.b, RacerFloatParam(step, "fog_start", 700.f)},
        {RacerFloatParam(step, "fog_end", 4400.f), 0.5f, 0.f, 0.f},
        {eye, 1.f}};
    const glm::mat4 pod = RacerPodMatrix(state_->pod, state_->podRoll);
    int drawn = DrawRacerSky(d, *state_, blend, eye);
    for (int blended = 0; blended < 2; ++blended) {
        SDL_GPUGraphicsPipeline* pipeline = blended ? blend : opaque;
        if (!pipeline) continue;
        SDL_BindGPUGraphicsPipeline(pass, pipeline);
        SDL_PushGPUFragmentUniformData(cmd, 0, &fragment, sizeof(fragment));
        drawn += DrawRacerModel(d, state_->trackModel, glm::mat4(1.f), blended);
        drawn += DrawRacerModel(d, state_->podModel, pod, blended);
        drawn += DrawRacerPodEffects(d, *state_, state_->pod,
                                     state_->podRoll, state_->podRig,
                                     blended);
        for (const RacerOpponent& opponent : state_->opponents) {
            // A rival on top of the camera would fill the screen with
            // the inside of its engines; it is left out until clear.
            if (glm::distance(opponent.pod.position, eye) < kHideNearEye) {
                continue;
            }
            drawn += DrawRacerModel(
                d, opponent.model, RacerPodMatrix(opponent.pod, opponent.roll),
                blended);
            drawn += DrawRacerPodEffects(d, *state_, opponent.pod,
                                         opponent.roll, opponent.rig,
                                         blended);
        }
    }
    if (!traced_ && logger_) {
        traced_ = true;
        logger_->Info("racer.scene.draw: first frame, " +
                      std::to_string(drawn) + " draws");
    }
}

}  // namespace sdl3cpp::services::impl
