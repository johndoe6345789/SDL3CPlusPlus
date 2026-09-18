#pragma once

#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

/// One entry of FS2024's building generator texture catalogue
/// (bf-pgg/PGG/textures/textures.json): which layer of the BC7
/// texture arrays holds it, and how big it is in the real world --
/// the generator sizes every brick course and every window from
/// these metres, not from pixels.
struct PggAsset {
    std::string file;       ///< e.g. "wall/wall_bricks_04.png"
    int albedoLayer = 0;    ///< index into TEXTURES_ALBEDO.DDS.DDS
    float widthMetres = 1.f, heightMetres = 1.f;
    float offsetX = 0.f, offsetY = 0.f;  ///< metres, from its tiling anchor
    std::string tilingX, tilingY;  ///< REPEAT / CENTER / BOTTOM ...
};

/// Every asset in that catalogue, in file order.
std::vector<PggAsset> ReadPggAssets(const std::string& jsonPath);

/// The first asset whose `file` is exactly `file`, by name -- the
/// catalogue lists some files more than once, at different sizes.
const PggAsset& FindPggAsset(const std::vector<PggAsset>& assets,
                             const std::string& file);

}  // namespace sdl3cpp::fs2024
