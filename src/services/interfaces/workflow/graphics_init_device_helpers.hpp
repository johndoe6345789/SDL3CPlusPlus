#pragma once

#include "services/interfaces/i_graphics_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_window_service.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Initializes the GPU device via `graphicsService` using
 * `windowService`'s native window handle.
 *
 * Logs and swallows any exception InitializeDevice throws -- the
 * swapchain init step will retry later if needed.
 */
void InitializeGraphicsDevice(const std::shared_ptr<IGraphicsService>& graphicsService,
                              const std::shared_ptr<IWindowService>& windowService,
                              const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
