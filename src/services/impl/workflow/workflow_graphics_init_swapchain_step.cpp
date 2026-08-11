#include "services/interfaces/workflow/workflow_graphics_init_swapchain_step.hpp"
#include "services/interfaces/i_graphics_service.hpp"
#include "services/interfaces/i_window_service.hpp"

namespace sdl3cpp::services::impl {

WorkflowGraphicsInitSwapchainStep::WorkflowGraphicsInitSwapchainStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)),
      graphicsService_(nullptr),
      windowService_(nullptr) {
    if (logger_) {
        logger_->Trace("WorkflowGraphicsInitSwapchainStep", "Constructor (logger only)", "Entry");
    }
}

WorkflowGraphicsInitSwapchainStep::WorkflowGraphicsInitSwapchainStep(std::shared_ptr<ILogger> logger,
                                                                     std::shared_ptr<IGraphicsService> graphicsService,
                                                                     std::shared_ptr<IWindowService> windowService)
    : logger_(std::move(logger)),
      graphicsService_(std::move(graphicsService)),
      windowService_(std::move(windowService)) {
    if (logger_) {
        logger_->Trace("WorkflowGraphicsInitSwapchainStep", "Constructor (with services)", "Entry");
    }
}

std::string WorkflowGraphicsInitSwapchainStep::GetPluginId() const {
    return "graphics.swapchain.init";
}

void WorkflowGraphicsInitSwapchainStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    (void)step;     // Unused
    (void)context;  // Unused

    if (logger_) {
        logger_->Trace("WorkflowGraphicsInitSwapchainStep", "Execute",
                       "graphicsSwapchainInitialization");
    }

    // Initialize swapchain if services are available, otherwise just checkpoint
    if (graphicsService_ && windowService_) {
        try {
            // Get window size and initialize swapchain
            auto windowSize = windowService_->GetSize();
            if (logger_) {
                logger_->Info("WorkflowGraphicsInitSwapchainStep::Execute: Window size=" +
                             std::to_string(windowSize.first) + "x" + std::to_string(windowSize.second));
            }

            // Recreate swapchain with current window size
            graphicsService_->RecreateSwapchain();
            graphicsService_->InitializeSwapchain();

            if (logger_) {
                logger_->Info("WorkflowGraphicsInitSwapchainStep::Execute: Swapchain initialization complete");
            }
        } catch (const std::exception& e) {
            if (logger_) {
                logger_->Warn("WorkflowGraphicsInitSwapchainStep::Execute: Swapchain init failed: " + std::string(e.what()) +
                             " (will retry during frame rendering)");
            }
            // Don't throw - let frame rendering handle it with retry logic
        }
    } else {
        // Services not available - just checkpoint
        if (logger_) {
            logger_->Info("WorkflowGraphicsInitSwapchainStep::Execute: Graphics swapchain initialization checkpoint (services unavailable)");
        }
    }

    if (logger_) {
        logger_->Trace("WorkflowGraphicsInitSwapchainStep", "Execute",
                       "graphicsSwapchainInitializationComplete");
    }
}

}  // namespace sdl3cpp::services::impl
