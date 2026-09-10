#include "services/interfaces/workflow/graphics/gpu_device_create.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

GpuShaderFormatChoice ResolveGpuShaderFormat(const std::string& renderer) {
    if (renderer == "metal") {
        return {SDL_GPU_SHADERFORMAT_MSL, "metal"};
    }
    if (renderer == "vulkan") {
        return {SDL_GPU_SHADERFORMAT_SPIRV, "vulkan"};
    }
    // auto: accept every shipped format so SDL picks best available backend
    return {static_cast<SDL_GPUShaderFormat>(SDL_GPU_SHADERFORMAT_SPIRV |
                                             SDL_GPU_SHADERFORMAT_MSL),
            nullptr};
}

SDL_GPUDevice* CreateGpuDeviceWithFallback(
    const std::shared_ptr<ILogger>& logger, const std::string& renderer,
    bool debugMode) {
    const GpuShaderFormatChoice choice = ResolveGpuShaderFormat(renderer);

    SDL_GPUDevice* device =
        SDL_CreateGPUDevice(choice.format, debugMode, choice.driverName);
    if (device) return device;

    if (logger) {
        logger->Warn("graphics.gpu.init: Failed with " + renderer + ": " +
                     std::string(SDL_GetError()));
    }

    // Fallback: let SDL auto-select
    device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, debugMode,
        nullptr);
    if (!device) {
        throw std::runtime_error(
            "graphics.gpu.init: SDL_CreateGPUDevice failed even with "
            "fallback: " +
            std::string(SDL_GetError()));
    }
    return device;
}

}  // namespace sdl3cpp::services::impl
