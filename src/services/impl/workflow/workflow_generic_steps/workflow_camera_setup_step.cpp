#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_setup_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Vulkan/Metal clip space [0,1]
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>

using json = nlohmann::json;

namespace sdl3cpp::services::impl {

WorkflowCameraSetupStep::WorkflowCameraSetupStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowCameraSetupStep::GetPluginId() const {
    return "camera.setup";
}

void WorkflowCameraSetupStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    try {
        WorkflowStepIoResolver ioResolver;
        WorkflowStepParameterResolver paramResolver;

        const std::string outputKey = ioResolver.GetRequiredOutputKey(step, "camera_state");

        // Extract parameters with defaults
        float distance = 35.0f;
        float fov = 60.0f;
        float aspectRatio = 1.777f;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;

        auto distIt = step.inputs.find("camera_distance");
        if (distIt != step.inputs.end()) {
            const auto* distValue = context.TryGet<double>(distIt->second);
            if (distValue) distance = static_cast<float>(*distValue);
        }

        auto fovIt = step.inputs.find("camera_fov");
        if (fovIt != step.inputs.end()) {
            const auto* fovValue = context.TryGet<double>(fovIt->second);
            if (fovValue) fov = static_cast<float>(*fovValue);
        }

        auto aspectIt = step.inputs.find("aspect_ratio");
        if (aspectIt != step.inputs.end()) {
            const auto* aspectValue = context.TryGet<double>(aspectIt->second);
            if (aspectValue) aspectRatio = static_cast<float>(*aspectValue);
        }

        auto nearIt = step.inputs.find("near_plane");
        if (nearIt != step.inputs.end()) {
            const auto* nearValue = context.TryGet<double>(nearIt->second);
            if (nearValue) nearPlane = static_cast<float>(*nearValue);
        }

        auto farIt = step.inputs.find("far_plane");
        if (farIt != step.inputs.end()) {
            const auto* farValue = context.TryGet<double>(farIt->second);
            if (farValue) farPlane = static_cast<float>(*farValue);
        }

        // Compute view matrix using GLM
        glm::mat4 viewMatrix = glm::lookAt(
            glm::vec3(0.0f, 0.0f, -distance),  // Eye
            glm::vec3(0.0f, 0.0f, 0.0f),        // Center
            glm::vec3(0.0f, 1.0f, 0.0f)          // Up
        );

        // Compute projection matrix (GLM_FORCE_DEPTH_ZERO_TO_ONE for Vulkan/Metal clip space)
        glm::mat4 projMatrix = glm::perspective(
            glm::radians(fov),
            aspectRatio,
            nearPlane,
            farPlane
        );

        // Build JSON output
        json cameraState = json::object();

        // Store view matrix as 16 floats (column-major, GLM default)
        json viewArray = json::array();
        const float* viewPtr = glm::value_ptr(viewMatrix);
        for (int i = 0; i < 16; ++i) {
            viewArray.push_back(viewPtr[i]);
        }
        cameraState["view"] = viewArray;

        // Store projection matrix as 16 floats
        json projArray = json::array();
        const float* projPtr = glm::value_ptr(projMatrix);
        for (int i = 0; i < 16; ++i) {
            projArray.push_back(projPtr[i]);
        }
        cameraState["projection"] = projArray;

        cameraState["distance"] = distance;
        cameraState["fov"] = fov;
        cameraState["aspect_ratio"] = aspectRatio;
        cameraState["near_plane"] = nearPlane;
        cameraState["far_plane"] = farPlane;
        cameraState["camera_setup_success"] = true;

        context.Set(outputKey, cameraState);

        if (logger_) {
            logger_->Info(std::string("WorkflowCameraSetupStep: Camera matrices computed ") +
                          "(distance=" + std::to_string(distance) +
                          ", fov=" + std::to_string(fov) +
                          ", aspect=" + std::to_string(aspectRatio) + ")");
        }

    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowCameraSetupStep::Execute: " + std::string(e.what()));
        }

        json errorState = json::object();
        errorState["camera_setup_success"] = false;
        errorState["error"] = e.what();
        context.Set("camera_state", errorState);

        throw;
    }
}

}  // namespace sdl3cpp::services::impl
