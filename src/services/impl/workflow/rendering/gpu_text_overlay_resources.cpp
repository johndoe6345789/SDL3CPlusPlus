#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"

#include "services/interfaces/workflow/rendering/gpu_text_overlay_buffer_factory.hpp"
#include "services/interfaces/workflow/rendering/gpu_text_overlay_pipeline_factory.hpp"
#include "services/interfaces/workflow/rendering/gpu_text_overlay_shader_loader.hpp"
#include "services/interfaces/workflow/rendering/gpu_text_overlay_surface_factory.hpp"

#include <string>

namespace sdl3cpp::services::impl {

const char* CreateGpuTextOverlayResources(SDL_GPUDevice* device,
                                          SDL_GPUTextureFormat swapchainFmt,
                                          GpuTextOverlayResources& out) {
    if (!device) {
        return "no GPU device";
    }

    // The overlay shaders are only shipped as SPIR-V; the Metal backend would
    // need MSL variants before this could run there.
    const char* driver = SDL_GetGPUDeviceDriver(device);
    if (driver && std::string(driver) != "vulkan") {
        return "overlay only supported on Vulkan";
    }

    GpuTextOverlayResources res;
    res.device = device;

    SDL_GPUShader* vertex   = nullptr;
    SDL_GPUShader* fragment = nullptr;
    const char* err = LoadOverlayShaderPair(device, vertex, fragment);
    if (*err) {
        return err;
    }

    res.pipeline =
        CreateOverlayPipeline(device, swapchainFmt, vertex, fragment);
    SDL_ReleaseGPUShader(device, vertex);
    SDL_ReleaseGPUShader(device, fragment);
    if (!res.pipeline) {
        DestroyGpuTextOverlayResources(res);
        return "overlay pipeline creation failed";
    }

    err = CreateOverlayGpuBuffers(device, res);
    if (*err) {
        DestroyGpuTextOverlayResources(res);
        return err;
    }

    err = CreateOverlaySurfaceAndRenderer(res);
    if (*err) {
        DestroyGpuTextOverlayResources(res);
        return err;
    }

    out = res;
    return "";
}

}  // namespace sdl3cpp::services::impl
