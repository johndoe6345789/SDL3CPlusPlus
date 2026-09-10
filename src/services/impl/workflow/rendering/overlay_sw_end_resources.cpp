#include "services/interfaces/workflow/rendering/overlay_sw_end_resources.hpp"
#include "services/interfaces/workflow/rendering/overlay_sw_end_resources_internal.hpp"

namespace sdl3cpp::services::impl {

bool CreateOverlaySwEndResources(SDL_GPUDevice* device, SDL_Window* window,
                                 int surfaceWidth, int surfaceHeight,
                                 const std::string& vertPath,
                                 const std::string& fragPath,
                                 OverlaySwEndResources& out) {
    namespace detail = overlay_sw_end_detail;

    OverlaySwEndResources res;
    res.device = device;
    res.pipeline =
        detail::CreateShaderStage(device, window, vertPath, fragPath);
    if (!res.pipeline) {
        return false;
    }

    if (!detail::CreateGpuBuffers(device, surfaceWidth, surfaceHeight, res)) {
        DestroyOverlaySwEndResources(res);
        return false;
    }

    out = res;
    return true;
}

void DestroyOverlaySwEndResources(OverlaySwEndResources& res) {
    if (res.device) {
        if (res.headSampler) SDL_ReleaseGPUSampler(res.device, res.headSampler);
        if (res.headVertices)
            SDL_ReleaseGPUBuffer(res.device, res.headVertices);
        if (res.sampler) SDL_ReleaseGPUSampler(res.device, res.sampler);
        if (res.vertices) SDL_ReleaseGPUBuffer(res.device, res.vertices);
        if (res.transfer) {
            SDL_ReleaseGPUTransferBuffer(res.device, res.transfer);
        }
        if (res.texture) SDL_ReleaseGPUTexture(res.device, res.texture);
        if (res.pipeline) {
            SDL_ReleaseGPUGraphicsPipeline(res.device, res.pipeline);
        }
    }
    res = OverlaySwEndResources{};
}

}  // namespace sdl3cpp::services::impl
