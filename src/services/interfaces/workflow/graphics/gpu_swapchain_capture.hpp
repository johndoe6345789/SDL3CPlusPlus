#pragma once

#include "services/interfaces/i_logger.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Downloads a swapchain texture and saves it to a BMP file.
 *
 * Submits @p cmd and blocks on SDL_WaitForGPUIdle() — the download has to
 * complete before the pixels can be read back — so callers must not submit
 * @p cmd again afterwards.
 *
 * @return true if the file was written; false if the transfer buffer or
 * the SDL_Surface used to save it could not be created.
 */
bool CaptureGpuSwapchainToBmp(SDL_GPUCommandBuffer* cmd, SDL_GPUDevice* device,
                              SDL_GPUTexture* swapchain, uint32_t width,
                              uint32_t height, SDL_GPUTextureFormat format,
                              const std::string& path,
                              const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
