#pragma once

#include "services/interfaces/i_logger.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace sdl3cpp::services::impl {

/// The shader format(s) and preferred driver name for a renderer string
/// ("metal", "vulkan", or anything else for "auto").
struct GpuShaderFormatChoice {
    SDL_GPUShaderFormat format;
    const char* driverName;
};

GpuShaderFormatChoice ResolveGpuShaderFormat(const std::string& renderer);

/**
 * @brief Creates a GPU device for `renderer`, falling back to SDL
 * auto-selection (any shipped shader format, no driver preference) if
 * the preferred driver fails.
 *
 * `debugMode` should default to false: SDL sets MTL_DEBUG_LAYER=1 when
 * enabled, which triggers a macOS 26 Metal validation bug where
 * newLibraryWithSource receives nil source. Opt in via SDL_GPU_DEBUG=1
 * for explicit GPU validation.
 *
 * @throws std::runtime_error (naming SDL_GetError()) if even the
 * fallback attempt fails.
 */
SDL_GPUDevice* CreateGpuDeviceWithFallback(
    const std::shared_ptr<ILogger>& logger, const std::string& renderer,
    bool debugMode);

/**
 * @brief Claims `window` for `device`.
 * @throws std::runtime_error (destroying `device` first) if the window
 * is null or the claim fails.
 */
void ClaimWindowForGpuOrThrow(SDL_GPUDevice* device, SDL_Window* window);

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
