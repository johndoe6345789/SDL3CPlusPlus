#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// One draw's worth of a landmark's mesh: everything using the same
/// base-colour texture. `textureFile` names a PNG under the kit's own
/// `textures/` directory (converted once from FS2024's own DDS at
/// bake time), or is empty for an untextured group.
struct LandmarkMeshGroup {
    std::string textureFile;
    TextureGroup mesh;
};

/// Reads a `.lmk` file, as `fs2024_prepare`'s `ExtractLandmarkKit`
/// writes it: magic `LMK1`, a `u32` group count, then per group a
/// length-prefixed texture filename, its `BspRenderVertex` array and
/// its `u32` index array, each itself `u32`-count-prefixed.
std::vector<LandmarkMeshGroup> ReadLandmarkKit(const std::string& path);

}  // namespace sdl3cpp::services::impl
