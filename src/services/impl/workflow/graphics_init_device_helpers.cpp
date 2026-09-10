#include "services/interfaces/workflow/graphics_init_device_helpers.hpp"
#include "services/interfaces/graphics_types.hpp"

#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

void InitializeGraphicsDevice(
    const std::shared_ptr<IGraphicsService>& graphicsService,
    const std::shared_ptr<IWindowService>& windowService,
    const std::shared_ptr<ILogger>& logger) {
    try {
        if (logger) {
            logger->Info(
                "WorkflowGraphicsInitDeviceStep::Execute: Calling "
                "graphics->InitializeDevice()");
        }

        GraphicsConfig config;
        config.preferredFormat = 0;  // Use default format

        auto* nativeWindowHandle = windowService->GetNativeHandle();
        if (logger) {
            logger->Info(
                "WorkflowGraphicsInitDeviceStep::Execute: Native window "
                "handle = " +
                std::to_string(
                    reinterpret_cast<uintptr_t>(nativeWindowHandle)));
        }

        graphicsService->InitializeDevice(nativeWindowHandle, config);

        if (logger) {
            logger->Info(
                "WorkflowGraphicsInitDeviceStep::Execute: Graphics device "
                "initialization complete");
        }
    } catch (const std::exception& e) {
        if (logger) {
            logger->Warn(
                "WorkflowGraphicsInitDeviceStep::Execute: Graphics init "
                "failed: " +
                std::string(e.what()));
        }
        // Continue - swapchain init will retry if needed.
    }
}

}  // namespace sdl3cpp::services::impl
