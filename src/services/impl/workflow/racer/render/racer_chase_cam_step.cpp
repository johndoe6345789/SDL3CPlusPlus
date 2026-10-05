#include "services/interfaces/workflow/racer/render/racer_chase_cam_step.hpp"

#include "services/interfaces/workflow/racer/racer_step_params.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

nlohmann::json ToArray(const glm::mat4& matrix) {
    nlohmann::json out = nlohmann::json::array();
    const float* values = glm::value_ptr(matrix);
    for (int i = 0; i < 16; ++i) out.push_back(values[i]);
    return out;
}

float AngleDelta(float from, float to) {
    float delta = std::fmod(to - from + 3.14159265f, 6.28318531f);
    if (delta < 0.f) delta += 6.28318531f;
    return delta - 3.14159265f;
}

}  // namespace

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
    heading_ += AngleDelta(heading_, pod.heading) * std::min(1.f, 5.f * dt);
    const float pace = std::clamp(pod.speed / state_->podSpec.topSpeed, 0.f,
                                  1.5f);
    const float distance =
        RacerFloatParam(step, "distance", 9.f) * (1.f + 0.35f * pace);
    const float height = RacerFloatParam(step, "height", 3.f);
    const glm::vec3 along = RacerPodForward(heading_);
    const glm::vec3 wanted =
        pod.position - along * distance + glm::vec3(0.f, height, 0.f);
    eye_ = started_ ? eye_ + (wanted - eye_) * std::min(1.f, 10.f * dt)
                    : wanted;
    started_ = true;
    const glm::vec3 look = pod.position +
                           along * RacerFloatParam(step, "lead", 12.f) +
                           glm::vec3(0.f, 1.f, 0.f);
    // A touch more field of view at speed sells the sense of pace.
    const float fov = RacerFloatParam(step, "fov", 70.f) + 12.f * pace;
    nlohmann::json state = nlohmann::json::object();
    state["view"] = ToArray(glm::lookAt(eye_, look, glm::vec3(0, 1, 0)));
    state["projection"] = ToArray(glm::perspective(
        glm::radians(fov), RacerFloatParam(step, "aspect", 16.f / 9.f),
        RacerFloatParam(step, "near", 0.3f),
        RacerFloatParam(step, "far", 6000.f)));
    context.Set("camera.state", state);
    context.Set("render.camera_pos", eye_);
}

}  // namespace sdl3cpp::services::impl
