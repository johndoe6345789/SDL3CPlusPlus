#include "services/interfaces/workflow/gta5/gta5_tile_coord.hpp"

namespace sdl3cpp::services::impl {

std::string MakeGta5TileTag(const Gta5TileCoord& tile) {
    return "gta5:" + std::to_string(tile.x) + "_" + std::to_string(tile.z);
}

}  // namespace sdl3cpp::services::impl
