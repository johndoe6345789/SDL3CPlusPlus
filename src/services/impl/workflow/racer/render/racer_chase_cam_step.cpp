#include "services/interfaces/workflow/racer/render/racer_chase_cam_step.hpp"

#include "services/interfaces/workflow/racer/racer_step_params.hpp"
#include "services/interfaces/workflow/racer/render/racer_camera_math.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl {
WorkflowRacerChaseCameraStep::WorkflowRacerChaseCameraStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowRacerChaseCameraStep::GetPluginId() const {
    return "racer.camera.chase";
}

void WorkflowRacerChaseCameraStep::Execute(const WorkflowStepDefinition& step,
                                           WorkflowContext& context) {
    if (!state_->loaded) return;
    const RacerPodState& pod = state_->pod;
    const float dt = std::min(
        0.1f, static_cast<float>(context.Get<double>("frame.delta_time", 0.0)));
    if (!started_) heading_ = pod.heading;
    heading_ +=
        RacerAngleDelta(heading_, pod.heading) * std::min(1.f, 8.f * dt);
    const float pace = std::clamp(pod.speed / state_->podSpec.topSpeed, 0.f,
                                  1.5f);
    // Big pods (Sebulba's) need the camera further back and higher.
    const float reach = state_->podRig.reach;
    const float distance =
        std::max(RacerFloatParam(step, "distance", 9.f), reach + 4.f) *
        (1.f + 0.25f * pace);
    const float height =
        std::max(RacerFloatParam(step, "height", 3.f), 0.4f * reach);
    const glm::vec3 along = RacerPodForward(heading_);
    // The eye rides rigidly behind the pod (easing its position made it
    // trail by metres at speed); only its height over the pod eases.
    const float rise = std::max(0.f, eyeY_ - pod.position.y);
    const float ease = started_ ? std::min(1.f, 6.f * dt) : 1.f;
    const float lift = rise + (height - rise) * ease;
    const glm::vec3 focus = pod.position + glm::vec3(0.f, 1.5f, 0.f);
    eye_ = RacerCameraPullIn(
        *state_, focus,
        pod.position - along * distance + glm::vec3(0.f, lift, 0.f),
        reach + 1.f);
    eye_.y =
        std::max(eye_.y, RacerCameraClearance(*state_, pod.position, eye_));
    eyeY_ = eye_.y;
    if (logger_ && ++traceFrame_ % 120 == 0) {
        logger_->Trace("racer.camera.chase: eye " +
                       std::to_string(glm::distance(eye_, pod.position)) +
                       " m from the pod, wanted " + std::to_string(distance));
    }
    started_ = true;
    const glm::vec3 look = pod.position +
                           along * RacerFloatParam(step, "lead", 12.f) +
                           glm::vec3(0.f, 1.f, 0.f);
    // A touch more field of view at speed sells the sense of pace.
    const float fov = RacerFloatParam(step, "fov", 70.f) + 12.f * pace;
    nlohmann::json state = nlohmann::json::object();
    const glm::vec3 up(0.f, 1.f, 0.f);
    state["view"] = RacerMatrixJson(glm::lookAt(eye_, look, up));
    state["projection"] = RacerMatrixJson(glm::perspective(
        glm::radians(fov), RacerFloatParam(step, "aspect", 16.f / 9.f),
        RacerFloatParam(step, "near", 0.3f),
        RacerFloatParam(step, "far", 12000.f)));
    context.Set("camera.state", state);
    context.Set("render.camera_pos", eye_);
}

}  // namespace sdl3cpp::services::impl
