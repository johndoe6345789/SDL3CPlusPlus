#pragma once

#include "services/interfaces/workflow/fs2024/data/bgl/fs2024_bgl_model_library.hpp"
#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_kit_gpu.hpp"

#include <cstddef>
#include <string>

namespace sdl3cpp::services::impl {

/// Loads one landmark model straight from FS2024's own library BGL: the
/// most detailed LOD whose binary glTF fits `lodBudgetBytes`, in the
/// model's own space, each primitive with its own base-colour DDS from
/// `texturesDir` uploaded still compressed. A primitive whose texture
/// is missing or unreadable draws untextured rather than losing the
/// model.
Fs2024LandmarkKitGpu LoadFs2024LandmarkKit(
    SDL_GPUDevice* device, const std::string& library,
    const sdl3cpp::fs2024::ModelLibraryEntry& entry,
    const std::string& texturesDir, std::size_t lodBudgetBytes);

void ReleaseFs2024LandmarkKitGpu(SDL_GPUDevice* device,
                                 Fs2024LandmarkKitGpu& kit);

}  // namespace sdl3cpp::services::impl
