#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Claims `window` for `device`.
 * @throws std::runtime_error (destroying `device` first) if the window
 * is null or the claim fails.
 */
void ClaimWindowForGpuOrThrow(SDL_GPUDevice* device, SDL_Window* window);

/// One-line trace description of a completed GPU init, for the logger.
std::string DescribeGpuInit(uint32_t width, uint32_t height,
                            SDL_GPUDevice* device);

}  // namespace sdl3cpp::services::impl
