#pragma once

#include "services/interfaces/workflow/fs2024/prepare/fs2024_landmark_catalog.hpp"

#include <string>

namespace sdl3cpp::tools::fs2024 {

/// Extracts one catalog entry's model -- its LOD0 mesh plus every
/// base-colour texture its primitives use -- into `<outDir>/landmarks/`,
/// unless already there: a landmark is shared, global data, the same
/// regardless of which bake or which of its tiles references it, so
/// re-extracting it on every bake would repeat real work (a big
/// landmark's LOD0 can run past half a million vertices and a dozen
/// 2048^2 textures). Writes `<outDir>/landmarks/<model>.lmk` (see
/// fs2024_landmark_kit.hpp for the format the runtime reads back) and
/// `<outDir>/landmarks/textures/<image uri>.png`.
void ExtractLandmarkKit(const LandmarkCatalogEntry& entry,
                       const std::string& outDir);

}  // namespace sdl3cpp::tools::fs2024
