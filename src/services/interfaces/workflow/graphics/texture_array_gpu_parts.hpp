#pragma once

#include "services/interfaces/workflow/graphics/texture_array_gpu_upload.hpp"

namespace sdl3cpp::services::impl {

/// The empty texture `view` describes.
/// Throws when the device cannot sample the format or creation fails.
SDL_GPUTexture* CreateTextureArray(SDL_GPUDevice* device,
                                   const TextureArrayBlocksView& view);

/// `view`'s blocks copied into a new upload transfer buffer, or nullptr.
SDL_GPUTransferBuffer* StageTextureArray(SDL_GPUDevice* device,
                                         const TextureArrayBlocksView& view);

/// Bytes of one mip of one layer.
std::size_t TextureArrayMipBytes(const TextureArrayBlocksView& view,
                                 std::uint32_t mip);

}  // namespace sdl3cpp::services::impl
