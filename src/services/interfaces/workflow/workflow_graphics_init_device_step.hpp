#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_graphics_service.hpp"
#include "services/interfaces/i_window_service.hpp"
#include "services/interfaces/graphics_types.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Initialize graphics device via the IGraphicsService backend.
 *
 * This is the device initialization step in the bootstrap workflow.
 * It calls graphics->InitializeDevice() to set up the GPU.
 *
 * Can be instantiated in two modes:
 * 1. Single-param (logger only): checkpoint mode for testing
 * 2. Three-param (with graphics/window services): actual initialization mode
 *
 * Plugin ID: graphics.device.init
 * Inputs: (none)
 * Outputs: (none)
 */
class WorkflowGraphicsInitDeviceStep : public IWorkflowStep {
public:
    explicit WorkflowGraphicsInitDeviceStep(std::shared_ptr<ILogger> logger);
    WorkflowGraphicsInitDeviceStep(std::shared_ptr<ILogger> logger,
                                   std::shared_ptr<IGraphicsService> graphicsService,
                                   std::shared_ptr<IWindowService> windowService);
    ~WorkflowGraphicsInitDeviceStep() override = default;

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IGraphicsService> graphicsService_;
    std::shared_ptr<IWindowService> windowService_;
};

}  // namespace sdl3cpp::services::impl
