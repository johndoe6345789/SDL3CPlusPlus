#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_setup_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/camera_matrix_builder.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowCameraSetupStep::WorkflowCameraSetupStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowCameraSetupStep::GetPluginId() const {
    return "camera.setup";
}

namespace {

void ReadDoubleInput(const WorkflowStepDefinition& step,
                     WorkflowContext& context, const char* inputName,
                     float& out) {
    auto it = step.inputs.find(inputName);
    if (it != step.inputs.end()) {
        if (const auto* v = context.TryGet<double>(it->second)) {
            out = static_cast<float>(*v);
        }
    }
}

}  // namespace

void WorkflowCameraSetupStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    try {
        WorkflowStepIoResolver ioResolver;
        const std::string outputKey =
            ioResolver.GetRequiredOutputKey(step, "camera_state");

        CameraSetupParams params;
        ReadDoubleInput(step, context, "camera_distance", params.distance);
        ReadDoubleInput(step, context, "camera_fov", params.fov);
        ReadDoubleInput(step, context, "aspect_ratio", params.aspectRatio);
        ReadDoubleInput(step, context, "near_plane", params.nearPlane);
        ReadDoubleInput(step, context, "far_plane", params.farPlane);

        const nlohmann::json cameraState = BuildCameraStateJson(params);
        context.Set(outputKey, cameraState);

        if (logger_) {
            logger_->Info(
                "WorkflowCameraSetupStep: Camera matrices computed "
                "(distance=" + std::to_string(params.distance) +
                ", fov=" + std::to_string(params.fov) +
                ", aspect=" + std::to_string(params.aspectRatio) + ")");
        }
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error(
                "WorkflowCameraSetupStep::Execute: " + std::string(e.what()));
        }

        nlohmann::json errorState = nlohmann::json::object();
        errorState["camera_setup_success"] = false;
        errorState["error"] = e.what();
        context.Set("camera_state", errorState);

        throw;
    }
}

}  // namespace sdl3cpp::services::impl
