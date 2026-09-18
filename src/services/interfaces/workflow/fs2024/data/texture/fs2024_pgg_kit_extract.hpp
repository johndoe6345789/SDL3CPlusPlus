#pragma once

#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_texture.hpp"

#include <string>

namespace sdl3cpp::fs2024 {

/// The two images every building is textured with, baked from FS2024's
/// own procedural building generator data (`bf-pgg/PGG/textures`): one
/// facade bay -- its brick with a real window composited in at the size
/// and height the generator itself uses -- and its roof tile.
struct PggBuildingKit {
    DdsImage wall;
    DdsImage roof;
};

/// `pggDir` is the folder holding textures.json and TEXTURES_*.DDS.DDS.
PggBuildingKit BakePggBuildingKit(const std::string& pggDir);

/// The same kit written to `<outDir>/building_kit/{wall,roof}.png`, for
/// the offline bake.
void ExtractPggBuildingKit(const std::string& pggDir,
                          const std::string& outDir);

}  // namespace sdl3cpp::fs2024
