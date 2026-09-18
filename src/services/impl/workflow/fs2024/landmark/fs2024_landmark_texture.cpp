#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_texture.hpp"

#include "services/interfaces/workflow/graphics/texture_array_gpu_upload.hpp"
#include "services/interfaces/workflow/graphics/texture_load_sampler.hpp"

#include <exception>

namespace sdl3cpp::services::impl {

SDL_GPUTextureSamplerBinding UploadFs2024LandmarkTexture(
    SDL_GPUDevice* device, const sdl3cpp::fs2024::DdsBlocks& dds) {
    TextureArrayBlocksView view;
    view.type = SDL_GPU_TEXTURETYPE_2D;
    view.format = dds.alpha ? SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM
                            : SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM;
    view.width = dds.width;
    view.height = dds.height;
    view.layers = 1;
    view.mips = dds.mips;
    view.blockBytes = dds.blockBytes;
    view.data = dds.data.data();
    view.size = dds.data.size();
    try {
        const UploadedTexture uploaded =
            UploadTextureArrayBlocks(device, view);
        return {uploaded.texture,
                CreateTextureLoadSampler(device, uploaded.texture,
                                         uploaded.numLevels)};
    } catch (const std::exception&) {
        return {};
    }
}

}  // namespace sdl3cpp::services::impl
