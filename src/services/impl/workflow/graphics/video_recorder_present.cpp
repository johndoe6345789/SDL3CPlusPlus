#include "services/interfaces/workflow/graphics/video_recorder_ops.hpp"

namespace sdl3cpp::services::impl {

void PresentVideoTarget(VideoRecorder& rec, SDL_GPUCommandBuffer* cmd,
                        SDL_GPUTexture* swapchain) {
    SDL_GPUBlitInfo blit     = {};
    blit.source.texture      = rec.target;
    blit.source.w            = rec.targetWidth;
    blit.source.h            = rec.targetHeight;
    blit.destination.texture = swapchain;
    blit.destination.w       = rec.targetWidth;
    blit.destination.h       = rec.targetHeight;
    blit.load_op             = SDL_GPU_LOADOP_DONT_CARE;
    blit.filter              = SDL_GPU_FILTER_NEAREST;
    SDL_BlitGPUTexture(cmd, &blit);
}

}  // namespace sdl3cpp::services::impl
