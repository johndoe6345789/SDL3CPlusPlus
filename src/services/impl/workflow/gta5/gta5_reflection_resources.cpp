#include "services/interfaces/workflow/gta5/gta5_reflection_step.hpp"

namespace sdl3cpp::services::impl {
namespace {

SDL_GPUTexture* Target(SDL_GPUDevice* device, SDL_GPUTextureFormat format,
                       SDL_GPUTextureUsageFlags usage, std::uint32_t width,
                       std::uint32_t height) {
    SDL_GPUTextureCreateInfo info = {};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = format;
    info.usage = usage;
    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    return SDL_CreateGPUTexture(device, &info);
}

}  // namespace

bool WorkflowGta5ReflectionDrawStep::Ensure(SDL_GPUDevice* device,
                                            std::uint32_t width,
                                            std::uint32_t height) {
    if (colour_ && depth_ && width == width_ && height == height_) {
        return true;
    }
    if (colour_) SDL_ReleaseGPUTexture(device, colour_);
    if (depth_) SDL_ReleaseGPUTexture(device, depth_);
    // The scene's own formats, so its pipelines draw here unchanged.
    colour_ = Target(device, SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
                     SDL_GPU_TEXTUREUSAGE_SAMPLER |
                         SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
                     width, height);
    depth_ = Target(device, SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
                    SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET, width, height);
    width_ = width;
    height_ = height;
    if ((!colour_ || !depth_) && logger_) {
        logger_->Warn(std::string("gta5.reflection.draw: no target: ") +
                      SDL_GetError());
    }
    return colour_ && depth_;
}

}  // namespace sdl3cpp::services::impl
