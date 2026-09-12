#include "services/interfaces/workflow/gta5/resource/gta5_asset_scan.hpp"

#include "services/interfaces/workflow/gta5/stream/gta5_grid.hpp"
#include "services/interfaces/workflow/gta5/resource/gta5_resource.hpp"
#include "services/interfaces/workflow/gta5/resource/gta5_ymap_placement.hpp"

#include <cctype>
#include <filesystem>
#include <unordered_set>

namespace sdl3cpp::services::impl {
namespace {

/// A dictionary's name table: hashes at +0x20, count at +0x28. A .ydd
/// and a .ytd both keep it there, in the system pages.
void ScanDictionary(const Gta5Resource& res, std::uint32_t id,
                    std::vector<Gta5ScanResult::HashFile>& out) {
    const std::int64_t hashes = res.Follow(0x20);
    for (std::uint16_t i = 0; hashes >= 0 && i < res.U16(0x28); ++i) {
        out.emplace_back(res.U32(hashes + 4 * std::int64_t{i}), id);
    }
}

/// The tiles a ymap has entities in -- by entity rather than by its
/// extents box, so a tile later reads only ymaps with something in it.
void ScanYmap(const Gta5Resource& res, std::uint32_t id,
              const Gta5WorldConfig& world, Gta5ScanResult& out) {
    std::unordered_set<Gta5TileCoord, Gta5TileCoordHash> tiles;
    for (const Gta5YmapEntity& e : ReadGta5YmapEntities(res)) {
        tiles.insert(
            Gta5TileForPosition(world, MakeGta5YmapPlacement(e).position));
    }
    if (!tiles.empty()) ++out.ymaps;
    for (const Gta5TileCoord& tile : tiles) {
        out.ymapTiles.emplace_back(tile, id);
    }
}

std::string Extension(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    for (char& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return ext;
}

}  // namespace

Gta5ScanResult ScanGta5AssetSlice(const std::vector<std::string>* files,
                                  const std::vector<std::uint32_t>* ids,
                                  std::size_t first, std::size_t step,
                                  Gta5WorldConfig world) {
    Gta5ScanResult result;
    Gta5Resource res;
    for (std::size_t i = first; i < ids->size(); i += step) {
        const std::uint32_t id = (*ids)[i];
        const std::string ext = Extension((*files)[id]);
        const bool ymap = ext == ".ymap";
        if (!LoadGta5Resource((*files)[id], res, !ymap)) continue;
        if (ymap) {
            ScanYmap(res, id, world, result);
        } else if (ext == ".ytd") {
            ScanGta5Textures(res, id, result);
        } else {
            ScanDictionary(res, id, result.drawables);
        }
    }
    return result;
}

}  // namespace sdl3cpp::services::impl
