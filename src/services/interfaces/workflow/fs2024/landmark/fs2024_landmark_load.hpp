#pragma once

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_kit_gpu.hpp"

namespace sdl3cpp::services::impl {

/// Uploads a decoded landmark (main thread only): its geometry, one
/// buffer pair per primitive, and each colour map once, its DXT1/DXT5
/// blocks passed through compressed with every mip.
Fs2024LandmarkKitGpu UploadFs2024LandmarkKit(SDL_GPUDevice* device,
                                             const Fs2024LandmarkMesh& mesh);

void ReleaseFs2024LandmarkKitGpu(SDL_GPUDevice* device,
                                 Fs2024LandmarkKitGpu& kit);

}  // namespace sdl3cpp::services::impl
