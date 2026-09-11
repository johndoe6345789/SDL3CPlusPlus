#pragma once

#include "services/interfaces/workflow/gta5/gta5_config_types.hpp"
#include "services/interfaces/workflow/gta5/gta5_tile_coord.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

/// What one worker found in its share of the files, merged afterwards by
/// ScanGta5AssetFiles so no worker ever writes to the index itself.
struct Gta5ScanResult {
    using HashFile = std::pair<std::uint32_t, std::uint32_t>;
    std::vector<HashFile> drawables;  // .ydd entry hash, file
    std::vector<HashFile> textures;   // .ytd texture hash, file
    std::vector<std::pair<Gta5TileCoord, std::uint32_t>> ymapTiles;
    std::size_t ymaps{0};
};

/// Open every `step`th file of `ids` starting at `first`. Dictionaries
/// are inflated to their system pages only; ymaps in full.
Gta5ScanResult ScanGta5AssetSlice(const std::vector<std::string>* files,
                                  const std::vector<std::uint32_t>* ids,
                                  std::size_t first, std::size_t step,
                                  Gta5WorldConfig world);

}  // namespace sdl3cpp::services::impl
