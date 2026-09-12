#pragma once

#include "services/interfaces/workflow/gta5/core/gta5_config_types.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_tile_coord.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

/// Where everything in the extracted map is, held in memory.
///
/// Nothing is converted and nothing is written: this only records which
/// file to open when a tile, an archetype or a texture is first wanted.
/// A .ydr or .yft is named after its archetype, so its file name is the
/// key; a .ydd or .ytd stores names only as hashes, so each is opened --
/// system pages only -- to read its table.
struct Gta5AssetIndex {
    std::vector<std::string> files;
    /// Archetype hash to the file holding its drawable.
    std::unordered_map<std::uint32_t, std::uint32_t> drawables;
    /// Archetype hash to name, where a file name supplies one.
    std::unordered_map<std::uint32_t, std::string> names;
    /// Texture name hash to the .ytd holding it.
    std::unordered_map<std::uint32_t, std::uint32_t> textures;
    /// ymaps with at least one entity inside each tile.
    std::unordered_map<Gta5TileCoord, std::vector<std::uint32_t>,
                       Gta5TileCoordHash>
        ymapsByTile;
    std::size_t ymapCount{0};
    double buildSeconds{0.0};
};

/// Walk `root` recursively and index it, opening dictionaries and ymaps
/// on every core.
Gta5AssetIndex BuildGta5AssetIndex(const std::string& root,
                                   const Gta5WorldConfig& world);

/// Open the listed .ydd, .ytd and .ymap files in parallel and record what
/// each holds. Part of BuildGta5AssetIndex.
void ScanGta5AssetFiles(Gta5AssetIndex& index,
                        const std::vector<std::uint32_t>& ids,
                        const Gta5WorldConfig& world);

/// Archetype name for logs and cache keys: the file name when one is
/// known, otherwise hash_XXXXXXXX, the way GTAUtil writes unknown names.
std::string Gta5ArchetypeName(const Gta5AssetIndex& index,
                              std::uint32_t hash);

}  // namespace sdl3cpp::services::impl
