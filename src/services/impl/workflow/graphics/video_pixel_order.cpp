#include "services/interfaces/workflow/graphics/video_recorder_ops.hpp"

namespace sdl3cpp::services::impl {

// SDL_GPU names formats by byte order, so these are the bytes a
// download holds. Anything wider, an HDR swapchain, is not recordable.
bool VideoByteOrder(SDL_GPUTextureFormat format, bool& bgra) {
    switch (format) {
        case SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM:
        case SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM_SRGB:
            bgra = true;
            return true;
        case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM:
        case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB:
            bgra = false;
            return true;
        default:
            return false;
    }
}

}  // namespace sdl3cpp::services::impl
