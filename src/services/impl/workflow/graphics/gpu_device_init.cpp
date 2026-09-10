#include "services/interfaces/workflow/graphics/gpu_device_init.hpp"

#include <cstdlib>
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
    return {static_cast<SDL_GPUShaderFormat>(
                SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL),
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
            "fallback: " + std::string(SDL_GetError()));
    }
    return device;
}

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

void ApplyPresentModeOverride(SDL_GPUDevice* device, SDL_Window* window,
                              const nlohmann::json& viewportConfig,
                              const std::shared_ptr<ILogger>& logger) {
    if (!viewportConfig.contains("present_mode")) return;

    std::string mode = viewportConfig["present_mode"];

    if (mode == "auto") {
        // Query actual monitor refresh rate and pick the best present
        // mode. High-refresh displays (>=120 Hz) get VSYNC so frames are
        // delivered right on the scanout boundary — smooth and
        // GPU-efficient. Low-refresh displays (<120 Hz) get MAILBOX so
        // we aren't capped at 60 fps and can stay responsive with a
        // shallow frame queue.
        int refreshHz = 60;
        SDL_DisplayID disp = SDL_GetDisplayForWindow(window);
        if (disp) {
            const SDL_DisplayMode* dm = SDL_GetCurrentDisplayMode(disp);
            if (dm && dm->refresh_rate > 0.0f) {
                refreshHz = static_cast<int>(dm->refresh_rate);
            }
        }
        mode = (refreshHz >= 120) ? "vsync" : "mailbox";
        if (logger) {
            logger->Info("graphics.gpu.init: auto present_mode -> " + mode +
                        " (display " + std::to_string(refreshHz) + " Hz)");
        }
    }

    SDL_GPUPresentMode pm = SDL_GPU_PRESENTMODE_VSYNC;
    if (mode == "mailbox") pm = SDL_GPU_PRESENTMODE_MAILBOX;
    else if (mode == "immediate") pm = SDL_GPU_PRESENTMODE_IMMEDIATE;

    if (SDL_WindowSupportsGPUPresentMode(device, window, pm)) {
        SDL_SetGPUSwapchainParameters(
            device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, pm);
        if (logger) logger->Info("graphics.gpu.init: present_mode=" + mode);
    } else if (logger) {
        logger->Warn("graphics.gpu.init: present_mode '" + mode +
                    "' unsupported, falling back to vsync");
    }
}

}  // namespace sdl3cpp::services::impl
