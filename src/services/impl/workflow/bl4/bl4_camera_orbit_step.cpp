#include "services/interfaces/workflow/bl4/bl4_player_steps.hpp"

#include "services/interfaces/workflow/bl4/bl4_player_pin.hpp"
#include "services/interfaces/workflow/bl4/bl4_step_params.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265f;

/// The yaw the fps camera needs to look along (dx, dz): the inverse of
/// bl4.player.spawn's heading convention, forward = (sin h, -cos h).
float YawToward(float dx, float dz) { return -std::atan2(dx, -dz); }

}  // namespace

WorkflowBl4CameraOrbitStep::WorkflowBl4CameraOrbitStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBl4CameraOrbitStep::GetPluginId() const { return "bl4.camera.orbit"; }

void WorkflowBl4CameraOrbitStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    const float radius = Bl4NumberOrEnv(step, "radius", 0.f);
    if (radius <= 0.f) return;  // off unless asked for
    const glm::vec3 centre{Bl4NumberOrEnv(step, "centre_x", 0.f),
                           Bl4NumberOrEnv(step, "centre_y", 0.f),
                           Bl4NumberOrEnv(step, "centre_z", 0.f)};
    const float height = Bl4NumberOrEnv(step, "height", 2000.f);
    const float period = std::max(Bl4NumberOrEnv(step, "seconds_per_orbit", 90.f), 1.f);

    // physics_dt, as gta5.player.fly uses: the frame's step, clamped.
    const float dt = std::clamp(context.Get<float>("physics_dt", 1.f / 60.f), 0.f, 0.1f);
    angle_ += 2.f * kPi * dt / period;
    const glm::vec3 eye{centre.x + radius * std::sin(angle_), centre.y + height,
                        centre.z - radius * std::cos(angle_)};
    PinBl4Player(context, eye);

    const glm::vec3 toCentre = centre - eye;
    const float across = std::sqrt(toCentre.x * toCentre.x + toCentre.z * toCentre.z);
    context.Set<float>("camera_yaw", YawToward(toCentre.x, toCentre.z));
    context.Set<float>("camera_pitch", std::atan2(toCentre.y, across));
    if (logger_ && !announced_) {
        logger_->Info("bl4.camera.orbit: radius " + std::to_string(radius) + " m at " +
                      std::to_string(height) + " m, " + std::to_string(period) + " s a lap");
        announced_ = true;
    }
}

}  // namespace sdl3cpp::services::impl
