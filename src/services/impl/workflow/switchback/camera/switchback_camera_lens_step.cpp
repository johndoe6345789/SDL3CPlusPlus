#include "services/interfaces/workflow/switchback/camera/switchback_camera_lens_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow_context.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Vulkan/Metal clip space [0,1]
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowSwitchbackCameraLensStep::WorkflowSwitchbackCameraLensStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowSwitchbackCameraLensStep::GetPluginId() const {
    return "switchback.camera.lens";
}

void WorkflowSwitchbackCameraLensStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    nlohmann::json camera =
        context.Get<nlohmann::json>("camera.state", nlohmann::json::object());
    const float fov = Gta5NumberOr(step, "fov", 60.f);
    const float nearPlane = Gta5NumberOr(step, "near", 0.5f);
    const float farPlane = Gta5NumberOr(step, "far", 4000.f);
    const float aspect = camera.value("aspect_ratio", 1.777f);
    const glm::mat4 proj = glm::perspective(glm::radians(fov), aspect,
                                            nearPlane, farPlane);
    const float* p = glm::value_ptr(proj);
    camera["projection"] = std::vector<float>(p, p + 16);
    camera["near_plane"] = nearPlane;
    camera["far_plane"] = farPlane;
    context.Set("camera.state", camera);
}

}  // namespace sdl3cpp::services::impl
