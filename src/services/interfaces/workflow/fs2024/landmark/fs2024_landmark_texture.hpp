#pragma once

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_kit_gpu.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// Uploads one of FS2024's landmark DDS colour maps (DXT1 or DXT5) into
/// `group`'s texture and sampler, its blocks passed through compressed
/// with every mip. UNORM, not sRGB: the shader decodes, as it does for
/// every fs2024 colour map. Leaves `group` untextured when the file is
/// missing or not a format this reads.
void UploadFs2024LandmarkTexture(SDL_GPUDevice* device,
                                 const std::string& path,
                                 Fs2024LandmarkGroupGpu& group);

}  // namespace sdl3cpp::services::impl
