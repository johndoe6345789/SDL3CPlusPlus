#pragma once

/// Internal helper shared by the bsp_texture_gpu_upload_*.cpp files that
/// together implement bsp_texture_gpu_upload.hpp. Not part of the public
/// workflow-step API.

#include "services/interfaces/workflow/rendering/bsp_texture_gpu_upload.hpp"

namespace sdl3cpp::services::impl::bsp_texture_detail {

/// Uploads already-decoded RGBA8 pixels, generating a full mip chain.
BspTextureUpload UploadRgba8WithMips(SDL_GPUDevice* device,
                                     const unsigned char* pixels, int width,
                                     int height);

}  // namespace sdl3cpp::services::impl::bsp_texture_detail
