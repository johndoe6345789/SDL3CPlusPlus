#include "services/interfaces/workflow/gta5/resource/gta5_ymap_tile.hpp"

#include "services/interfaces/workflow/gta5/stream/gta5_grid.hpp"
#include "services/interfaces/workflow/gta5/resource/gta5_resource.hpp"
#include "services/interfaces/workflow/gta5/resource/gta5_ymap_placement.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

void ReadGta5YmapTile(const Gta5AssetIndex& index,
                      const Gta5WorldConfig& world, const Gta5TileCoord& tile,
                      std::vector<Gta5Placement>& out) {
    const auto found = index.ymapsByTile.find(tile);
    if (found == index.ymapsByTile.end()) return;

    Gta5Resource res;
    for (const std::uint32_t file : found->second) {
        if (!LoadGta5Resource(index.files[file], res)) continue;
        for (const Gta5YmapEntity& entity : ReadGta5YmapEntities(res)) {
            Gta5Placement placement = MakeGta5YmapPlacement(entity);
            if (!(Gta5TileForPosition(world, placement.position) == tile)) {
                continue;
            }
            placement.archetype = Gta5ArchetypeName(index, entity.archetype);
            out.push_back(std::move(placement));
        }
    }
}

}  // namespace sdl3cpp::services::impl
