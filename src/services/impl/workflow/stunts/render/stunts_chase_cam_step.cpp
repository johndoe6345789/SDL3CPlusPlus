#include "services/interfaces/workflow/stunts/render/stunts_chase_cam_step.hpp"

#include "services/interfaces/workflow/stunts/stunts_step_params.hpp"
#include "services/interfaces/workflow_context.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

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

/// Shortest signed turn from `from` to `to`, so the camera never
/// unwinds the long way round when the heading wraps.
float AngleDelta(float from, float to) {
    float delta = std::fmod(to - from + 3.14159265f, 6.28318531f);
    if (delta < 0.f) delta += 6.28318531f;
    return delta - 3.14159265f;
}

}  // namespace

WorkflowStuntsChaseCameraStep::WorkflowStuntsChaseCameraStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowStuntsChaseCameraStep::GetPluginId() const {
    return "stunts.camera.chase";
}

void WorkflowStuntsChaseCameraStep::Execute(const WorkflowStepDefinition& step,
                                            WorkflowContext& context) {
    const auto* at = context.TryGet<glm::vec3>("stunts.car_pos");
    if (!at) return;
    const float target = context.Get<float>("stunts.car_heading", 0.f);
    if (!started_) {
        heading_ = target;
        started_ = true;
    }
    const float follow = StuntsNumberOr(step, "follow", 0.12f);
    heading_ += AngleDelta(heading_, target) * follow;

    // Edge-detect the toggle button: it stays true for as long as the
    // key is held, so acting on every true frame would flicker
    // between modes instead of switching once per press.
    const bool toggleDown = context.GetBool("stunts.camera_toggle_pressed",
                                            false);
    if (toggleDown && !toggleWasDown_) cockpit_ = !cockpit_;
    toggleWasDown_ = toggleDown;
    context.Set("stunts.camera_mode",
               std::string(cockpit_ ? "cockpit" : "chase"));

    const glm::vec3 carAlong(std::cos(target), 0.f, std::sin(target));
    glm::vec3 eye;
    glm::vec3 look;
    if (cockpit_) {
        // The driver's seat: roughly car-length-forward of centre and
        // at head height, looking straight down the car's own nose
        // rather than the eased chase heading, since sitting inside
        // it a real driver's view snaps with the wheel, not lags it.
        eye = *at + carAlong * StuntsNumberOr(step, "eye_forward", 0.3f) +
              glm::vec3(0.f, StuntsNumberOr(step, "eye_height", 1.1f), 0.f);
        look = eye + carAlong * 10.f;
    } else {
        const glm::vec3 along(std::cos(heading_), 0.f, std::sin(heading_));
        const float distance = StuntsNumberOr(step, "distance", 16.f);
        const float height = StuntsNumberOr(step, "height", 6.5f);
        const float lead = StuntsNumberOr(step, "lead", 10.f);
        eye = *at - along * distance + glm::vec3(0.f, height, 0.f);
        look = *at + along * lead + glm::vec3(0.f, 1.5f, 0.f);
    }

    const float aspect = StuntsNumberOr(step, "aspect", 4.f / 3.f);
    nlohmann::json state = nlohmann::json::object();
    state["view"] = ToArray(glm::lookAt(eye, look, glm::vec3(0.f, 1.f, 0.f)));
    state["projection"] = ToArray(glm::perspective(
        glm::radians(StuntsNumberOr(step, "fov", 60.f)), aspect,
        StuntsNumberOr(step, "near", 0.5f),
        StuntsNumberOr(step, "far", 4000.f)));
    context.Set("camera.state", state);
    context.Set("render.camera_pos", eye);
}

}  // namespace sdl3cpp::services::impl
