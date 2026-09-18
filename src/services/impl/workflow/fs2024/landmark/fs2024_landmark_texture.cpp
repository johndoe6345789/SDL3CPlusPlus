#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_texture.hpp"

#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_blocks.hpp"
#include "services/interfaces/workflow/graphics/texture_array_gpu_upload.hpp"
#include "services/interfaces/workflow/graphics/texture_load_sampler.hpp"

#include <exception>
#include <filesystem>

namespace sdl3cpp::services::impl {

void UploadFs2024LandmarkTexture(SDL_GPUDevice* device,
                                 const std::string& path,
                                 Fs2024LandmarkGroupGpu& group) {
    if (!std::filesystem::exists(path)) return;
    try {
        const auto dds = sdl3cpp::fs2024::ReadDdsBlocks(path);
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
        const UploadedTexture uploaded =
            UploadTextureArrayBlocks(device, view);
        group.texture = uploaded.texture;
        group.sampler = CreateTextureLoadSampler(device, uploaded.texture,
                                                 uploaded.numLevels);
    } catch (const std::exception&) {
        // Drawn untextured: one bad map should not lose the landmark.
    }
}

}  // namespace sdl3cpp::services::impl
