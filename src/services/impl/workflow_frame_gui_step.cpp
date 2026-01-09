#include "workflow_frame_gui_step.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowFrameGuiStep::WorkflowFrameGuiStep(std::shared_ptr<IInputService> inputService,
                                           std::shared_ptr<ILogger> logger)
    : inputService_(std::move(inputService)),
      logger_(std::move(logger)) {}

std::string WorkflowFrameGuiStep::GetPluginId() const {
    return "frame.gui";
}

void WorkflowFrameGuiStep::Execute(const WorkflowStepDefinition&,
                                   WorkflowContext&) {
    if (!inputService_) {
        throw std::runtime_error("frame.gui requires an IInputService");
    }
    inputService_->UpdateGuiInput();
    if (logger_) {
        logger_->Trace("WorkflowFrameGuiStep", "Execute", "", "GUI input dispatched");
    }
}

}  // namespace sdl3cpp::services::impl
