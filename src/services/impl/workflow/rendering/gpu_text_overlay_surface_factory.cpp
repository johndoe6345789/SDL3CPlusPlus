#include "services/interfaces/workflow/rendering/gpu_text_overlay_surface_factory.hpp"

namespace sdl3cpp::services::impl {

const char* CreateOverlaySurfaceAndRenderer(GpuTextOverlayResources& res) {
    res.surface = SDL_CreateSurface(kGpuTextOverlayWidth, kGpuTextOverlayHeight,
                                    SDL_PIXELFORMAT_RGBA32);
    if (!res.surface) {
        return "overlay surface creation failed";
    }

    res.renderer = SDL_CreateSoftwareRenderer(res.surface);
    if (!res.renderer) {
        return "overlay software renderer creation failed";
    }

    return "";
}

}  // namespace sdl3cpp::services::impl
