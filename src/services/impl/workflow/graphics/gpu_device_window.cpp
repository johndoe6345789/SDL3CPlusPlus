#include "services/interfaces/workflow/graphics/gpu_device_window.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

void ClaimWindowForGpuOrThrow(SDL_GPUDevice* device, SDL_Window* window) {
    if (!window) {
        SDL_DestroyGPUDevice(device);
        throw std::runtime_error(
            "graphics.gpu.init: SDL window not found in context");
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_DestroyGPUDevice(device);
        throw std::runtime_error(
            "graphics.gpu.init: SDL_ClaimWindowForGPUDevice failed: " +
            std::string(SDL_GetError()));
    }
}

std::string DescribeGpuInit(uint32_t width, uint32_t height,
                            SDL_GPUDevice* device) {
    const char* driver = SDL_GetGPUDeviceDriver(device);
    return "width=" + std::to_string(width) +
           ", height=" + std::to_string(height) +
           ", driver=" + std::string(driver ? driver : "unknown");
}

}  // namespace sdl3cpp::services::impl
