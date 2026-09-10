#pragma once

#include "services/interfaces/i_logger.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <memory>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/**
 * @brief Applies an optional `present_mode` override from
 * `viewportConfig` ("vsync" | "mailbox" | "immediate" | "auto", picking
 * mailbox/vsync by display refresh rate). Absent means "leave the
 * default VSYNC claim as-is". Unsupported modes fall back to vsync
 * with a warning; never throws.
 */
void ApplyPresentModeOverride(SDL_GPUDevice* device, SDL_Window* window,
                              const nlohmann::json& viewportConfig,
                              const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
