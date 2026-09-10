#include "services/interfaces/workflow/workflow_graphics_init_device_step.hpp"
#include "services/interfaces/workflow/graphics_init_device_helpers.hpp"

#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGraphicsInitDeviceStep::WorkflowGraphicsInitDeviceStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)),
      graphicsService_(nullptr),
      windowService_(nullptr) {
    if (logger_) {
        logger_->Trace("WorkflowGraphicsInitDeviceStep",
                       "Constructor (logger only)", "Entry");
    }
}

WorkflowGraphicsInitDeviceStep::WorkflowGraphicsInitDeviceStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IGraphicsService> graphicsService,
    std::shared_ptr<IWindowService> windowService)
    : logger_(std::move(logger)),
      graphicsService_(std::move(graphicsService)),
      windowService_(std::move(windowService)) {
    if (logger_) {
        logger_->Trace("WorkflowGraphicsInitDeviceStep",
                       "Constructor (with services)", "Entry");
    }
}

std::string WorkflowGraphicsInitDeviceStep::GetPluginId() const {
    return "graphics.device.init";
}

void WorkflowGraphicsInitDeviceStep::Execute(const WorkflowStepDefinition&,
                                             WorkflowContext&) {
    if (logger_) {
        logger_->Trace("WorkflowGraphicsInitDeviceStep", "Execute",
                       "graphicsDeviceInitialization");
    }

    if (graphicsService_ && windowService_) {
        InitializeGraphicsDevice(graphicsService_, windowService_, logger_);
    } else if (logger_) {
        logger_->Info(
            "WorkflowGraphicsInitDeviceStep::Execute: Graphics device "
            "initialization checkpoint (services unavailable)");
    }

    if (logger_) {
        logger_->Trace("WorkflowGraphicsInitDeviceStep", "Execute",
                       "graphicsDeviceInitializationComplete");
    }
}

}  // namespace sdl3cpp::services::impl
