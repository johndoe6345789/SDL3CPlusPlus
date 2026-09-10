#pragma once

#include "services/interfaces/workflow/gta5/gta5_config_types.hpp"
#include "services/interfaces/workflow/gta5/gta5_lod.hpp"
#include "services/interfaces/workflow/gta5/gta5_tile_coord.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Which tile a world position falls in.
Gta5TileCoord Gta5TileForPosition(const Gta5WorldConfig& world,
                                  const glm::vec3& position);

/// Centre of a tile in world space, at y = 0.
glm::vec3 Gta5TileCentre(const Gta5WorldConfig& world,
                         const Gta5TileCoord& tile);

/// Detail band for a distance in metres.
Gta5Lod Gta5BandForDistance(const Gta5WorldConfig& world,
                            float distanceMetres);

}  // namespace sdl3cpp::services::impl
