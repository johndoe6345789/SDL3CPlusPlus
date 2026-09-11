#include "services/interfaces/workflow/gta5/gta5_player_camera_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_look.hpp"
#include "services/interfaces/workflow/gta5/gta5_shown_transform.hpp"
#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowGta5PlayerCameraStep::WorkflowGta5PlayerCameraStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5PlayerCameraStep::GetPluginId() const {
    return "gta5.player.camera";
}

void WorkflowGta5PlayerCameraStep::Execute(const WorkflowStepDefinition& step,
                                           WorkflowContext& context) {
    if (!state_) return;
    const bool toggle = Gta5KeyDown(
        context.TryGet<nlohmann::json>("input.keyboard.state"), "V");
    if (toggle && !held_) {
        third_ = !third_;
        if (logger_) {
            logger_->Info(third_ ? "gta5.player.camera: third person"
                                 : "gta5.player.camera: first person");
        }
    }
    held_ = toggle;
    const bool on = third_ && state_->seated < 0;
    context.Set<bool>("gta5.third_person", on);
    btRigidBody* player = Gta5PlayerBody(context);
    if (!on || !player) return;

    const btVector3 body = Gta5ShownTransform(player).getOrigin();
    const glm::vec3 head(body.x(),
                         body.y() + Gta5NumberOr(step, "head_height", 0.6f),
                         body.z());
    const glm::vec3 front =
        Gta5LookFront(context.Get<float>("camera_yaw", 0.f),
                      context.Get<float>("camera_pitch", 0.f));
    const glm::vec3 wanted =
        head - front * Gta5NumberOr(step, "distance", 3.2f) +
        glm::vec3(0.f, Gta5NumberOr(step, "height", 0.4f), 0.f);
    // In front of a wall rather than behind it, looking at its back.
    const glm::vec3 eye = Gta5ClearEye(
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr), head,
        wanted, player);
    const glm::mat4 view =
        glm::lookAt(eye, eye + front, glm::vec3(0.f, 1.f, 0.f));
    nlohmann::json camera =
        context.Get<nlohmann::json>("camera.state", nlohmann::json::object());
    const float* v = glm::value_ptr(view);
    camera["view"] = std::vector<float>(v, v + 16);
    camera["position"] = {eye.x, eye.y, eye.z};
    context.Set("camera.state", camera);
}

}  // namespace sdl3cpp::services::impl
