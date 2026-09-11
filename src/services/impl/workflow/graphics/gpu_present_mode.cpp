#include "services/interfaces/workflow/graphics/gpu_present_mode.hpp"

#include <cstdlib>
#include <string>

namespace sdl3cpp::services::impl {

void ApplyPresentModeOverride(SDL_GPUDevice* device, SDL_Window* window,
                              const nlohmann::json& viewportConfig,
                              const std::shared_ptr<ILogger>& logger) {
    if (!viewportConfig.contains("present_mode")) return;

    std::string mode = viewportConfig["present_mode"];
    // For benchmarking: SDL3CPP_PRESENT_MODE=immediate lifts the vsync
    // cap without editing a package.
    if (const char* forced = std::getenv("SDL3CPP_PRESENT_MODE")) {
        if (*forced) mode = forced;
    }

    if (mode == "auto") {
        // Query actual monitor refresh rate and pick the best present
        // mode. High-refresh displays (>=120 Hz) get VSYNC so frames are
        // delivered right on the scanout boundary — smooth and
        // GPU-efficient. Low-refresh displays (<120 Hz) get MAILBOX so
        // we aren't capped at 60 fps and can stay responsive with a
        // shallow frame queue.
        int refreshHz      = 60;
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
    if (mode == "mailbox")
        pm = SDL_GPU_PRESENTMODE_MAILBOX;
    else if (mode == "immediate")
        pm = SDL_GPU_PRESENTMODE_IMMEDIATE;

    if (SDL_WindowSupportsGPUPresentMode(device, window, pm)) {
        SDL_SetGPUSwapchainParameters(device, window,
                                      SDL_GPU_SWAPCHAINCOMPOSITION_SDR, pm);
        if (logger) logger->Info("graphics.gpu.init: present_mode=" + mode);
    } else if (logger) {
        logger->Warn("graphics.gpu.init: present_mode '" + mode +
                     "' unsupported, falling back to vsync");
    }
}

}  // namespace sdl3cpp::services::impl
