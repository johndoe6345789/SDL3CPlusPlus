#include "services/interfaces/workflow/gta5/gta5_vehicle_camera_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_shown_transform.hpp"

#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowGta5VehicleCameraStep::WorkflowGta5VehicleCameraStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5VehicleCameraStep::GetPluginId() const {
    return "gta5.vehicle.camera";
}

void WorkflowGta5VehicleCameraStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!state_ || state_->seated < 0 ||
        state_->seated >= static_cast<int>(state_->vehicles.size())) {
        placed_ = false;
        return;
    }
    const btRigidBody* chassis = state_->vehicles[state_->seated].chassis;
    if (!chassis) return;

    // The interpolated transform, as the car is drawn: following the
    // stepped one jolted the view at 60 Hz.
    const btTransform t = Gta5ShownTransform(chassis);
    const glm::vec3 car(t.getOrigin().x(), t.getOrigin().y(),
                        t.getOrigin().z());
    // Heading only. Following the chassis's pitch and roll puts every
    // bump in the road into the view.
    const btVector3 f = t.getBasis() * btVector3(0.f, 0.f, 1.f);
    glm::vec3 forward(f.x(), 0.f, f.z());
    if (glm::length(forward) < 1e-3f) forward = glm::vec3(0.f, 0.f, 1.f);
    forward = glm::normalize(forward);

    const float back = Gta5NumberOr(step, "distance", 6.5f);
    const float up = Gta5NumberOr(step, "height", 2.2f);
    const glm::vec3 wanted = car - forward * back + glm::vec3(0.f, up, 0.f);
    // Exponential ease: the camera closes a fixed share of the gap each
    // frame, so it trails a hard turn instead of snapping round it.
    const float ease = Gta5NumberOr(step, "ease", 0.18f);
    eye_ = placed_ ? eye_ + (wanted - eye_) * ease : wanted;
    placed_ = true;

    const glm::mat4 view = glm::lookAt(
        eye_, car + glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
    nlohmann::json camera =
        context.Get<nlohmann::json>("camera.state", nlohmann::json::object());
    const float* v = glm::value_ptr(view);
    camera["view"] = std::vector<float>(v, v + 16);
    camera["position"] = {eye_.x, eye_.y, eye_.z};
    context.Set("camera.state", camera);
}

}  // namespace sdl3cpp::services::impl
