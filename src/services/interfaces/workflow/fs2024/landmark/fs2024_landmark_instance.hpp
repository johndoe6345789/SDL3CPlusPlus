#pragma once

#include "services/interfaces/workflow/fs2024/data/bgl/fs2024_bgl_model_library.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// One landmark standing in a tile, where FS2024 itself places it. Its
/// mesh is shared by every instance of the model (see
/// Fs2024TileStreamState::landmarkKits, keyed by `entry.name`); only
/// the placement is per instance.
struct Fs2024LandmarkInstance {
    sdl3cpp::fs2024::ModelLibraryEntry entry;
    glm::mat4 model{1.f};  ///< the model's own space to tile space
};

}  // namespace sdl3cpp::services::impl
