#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services {
class IGraphicsService;
class IWindowService;
}

namespace sdl3cpp::services::impl {

/**
 * @brief Initializes the graphics swapchain for frame presentation.
 *
 * Calls RecreateSwapchain to prepare the swapchain for presentation.
 * This must be called AFTER graphics.device.init and BEFORE rendering begins.
 *
 * Must be called AFTER graphics.device.init.
 *
 * Plugin ID: graphics.swapchain.init
 * Inputs: (none)
 * Outputs: (none)
 */
class WorkflowGraphicsInitSwapchainStep : public IWorkflowStep {
public:
    explicit WorkflowGraphicsInitSwapchainStep(std::shared_ptr<ILogger> logger);
    WorkflowGraphicsInitSwapchainStep(std::shared_ptr<ILogger> logger,
                                      std::shared_ptr<sdl3cpp::services::IGraphicsService> graphicsService,
                                      std::shared_ptr<sdl3cpp::services::IWindowService> windowService);
    ~WorkflowGraphicsInitSwapchainStep() override = default;

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<sdl3cpp::services::IGraphicsService> graphicsService_;
    std::shared_ptr<sdl3cpp::services::IWindowService> windowService_;
};

}  // namespace sdl3cpp::services::impl
