#include "services/interfaces/workflow/workflow_graphics_init_device_step.hpp"

#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowGraphicsInitDeviceStep::WorkflowGraphicsInitDeviceStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)),
      graphicsService_(nullptr),
      windowService_(nullptr) {
    if (logger_) {
        logger_->Trace("WorkflowGraphicsInitDeviceStep", "Constructor (logger only)", "Entry");
    }
}

WorkflowGraphicsInitDeviceStep::WorkflowGraphicsInitDeviceStep(std::shared_ptr<ILogger> logger,
                                                               std::shared_ptr<IGraphicsService> graphicsService,
                                                               std::shared_ptr<IWindowService> windowService)
    : logger_(std::move(logger)),
      graphicsService_(std::move(graphicsService)),
      windowService_(std::move(windowService)) {
    if (logger_) {
        logger_->Trace("WorkflowGraphicsInitDeviceStep", "Constructor (with services)", "Entry");
    }
}

std::string WorkflowGraphicsInitDeviceStep::GetPluginId() const {
    return "graphics.device.init";
}

void WorkflowGraphicsInitDeviceStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    (void)step;     // Unused
    (void)context;  // Unused

    if (logger_) {
        logger_->Trace("WorkflowGraphicsInitDeviceStep", "Execute",
                       "graphicsDeviceInitialization");
    }

    // Initialize graphics device if services are available, otherwise just checkpoint
    if (graphicsService_ && windowService_) {
        try {
            if (logger_) {
                logger_->Info("WorkflowGraphicsInitDeviceStep::Execute: Calling graphics->InitializeDevice()");
            }

            // Create minimal config for graphics initialization
            GraphicsConfig config;
            config.preferredFormat = 0;  // Use default format

            // Get the native window handle from the window service
            auto* nativeWindowHandle = windowService_->GetNativeHandle();
            if (logger_) {
                logger_->Info("WorkflowGraphicsInitDeviceStep::Execute: Native window handle = " + std::to_string(reinterpret_cast<uintptr_t>(nativeWindowHandle)));
            }

            // Initialize graphics with the native window handle
            graphicsService_->InitializeDevice(nativeWindowHandle, config);

            if (logger_) {
                logger_->Info("WorkflowGraphicsInitDeviceStep::Execute: Graphics device initialization complete");
            }
        } catch (const std::exception& e) {
            if (logger_) {
                logger_->Warn("WorkflowGraphicsInitDeviceStep::Execute: Graphics init failed: " + std::string(e.what()));
            }
            // Continue - swapchain init will retry if needed
        }
    } else {
        // Services not available - just checkpoint
        if (logger_) {
            logger_->Info("WorkflowGraphicsInitDeviceStep::Execute: Graphics device initialization checkpoint (services unavailable)");
        }
    }

    if (logger_) {
        logger_->Trace("WorkflowGraphicsInitDeviceStep", "Execute",
                       "graphicsDeviceInitializationComplete");
    }
}

}  // namespace sdl3cpp::services::impl
