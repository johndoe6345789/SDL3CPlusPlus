#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_build_view_state_step.hpp"

#include "services/interfaces/workflow/workflow_camera_view_state_builder.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowCameraBuildViewStateStep::WorkflowCameraBuildViewStateStep(std::shared_ptr<IConfigService> configService,
                                                                   std::shared_ptr<ILogger> logger)
    : configService_(std::move(configService)),
      logger_(std::move(logger)) {}

std::string WorkflowCameraBuildViewStateStep::GetPluginId() const {
    return "camera.build_view_state";
}

void WorkflowCameraBuildViewStateStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string poseKey = resolver.GetRequiredInputKey(step, "pose");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "view_state");

    const auto* pose = context.TryGet<CameraPose>(poseKey);
    if (!pose) {
        throw std::runtime_error("camera.build_view_state requires pose input");
    }

    float aspect = 1.0f;
    auto aspectIt = step.inputs.find("aspect");
    if (aspectIt != step.inputs.end()) {
        const auto* aspectValue = context.TryGet<double>(aspectIt->second);
        if (!aspectValue) {
            throw std::runtime_error("camera.build_view_state missing aspect input");
        }
        aspect = static_cast<float>(*aspectValue);
    } else if (configService_) {
        uint32_t width = configService_->GetWindowWidth();
        uint32_t height = configService_->GetWindowHeight();
        if (width > 0 && height > 0) {
            aspect = static_cast<float>(width) / static_cast<float>(height);
        }
    }

    ViewState viewState = BuildViewState(*pose, aspect);
    context.Set(outputKey, viewState);

    if (logger_) {
        logger_->Trace("WorkflowCameraBuildViewStateStep", "Execute",
                       "input=" + poseKey +
                           ", output=" + outputKey +
                           ", aspect=" + std::to_string(aspect),
                       "Built camera view state");
    }
}

}  // namespace sdl3cpp::services::impl
