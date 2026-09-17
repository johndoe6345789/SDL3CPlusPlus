#pragma once

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_instance.hpp"
#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_kit_gpu.hpp"

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// A tile's own `landmarks.json`, or empty if it has none.
std::vector<Fs2024LandmarkInstance> ReadTileLandmarks(
    const std::string& tileDir);

/// Loads and uploads one landmark model's whole kit: its `.lmk` mesh
/// groups, translated and Y-rotated (by `headingDegrees`, clockwise
/// from above) to `(x, z)` on the CPU before upload -- this engine's
/// fs2024 shader takes no model matrix, ground and OSM buildings alike
/// are baked in world space already, so a landmark's own local-space
/// mesh is placed the same way rather than adding one just for this.
/// `tilesRoot` is the bake's own `--out` root, the same one
/// `landmarks/<model>.lmk` sits under regardless of which tile
/// references it.
///
/// The placement baked in is whichever instance is loaded first: a
/// model referenced by more than one instance (not something any
/// current catalog does) would incorrectly share the first one's
/// position, since the kit itself -- not each instance -- is what
/// this caches.
Fs2024LandmarkKitGpu LoadFs2024LandmarkKitGpu(SDL_GPUDevice* device,
                                            const std::string& tilesRoot,
                                            const std::string& model,
                                            float x, float z,
                                            float headingDegrees);

void ReleaseFs2024LandmarkKitGpu(SDL_GPUDevice* device,
                                Fs2024LandmarkKitGpu& kit);

}  // namespace sdl3cpp::services::impl
