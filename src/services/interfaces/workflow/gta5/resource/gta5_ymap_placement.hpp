#pragma once

#include "services/interfaces/workflow/gta5/resource/gta5_placement.hpp"
#include "services/interfaces/workflow/gta5/resource/gta5_ymap.hpp"

namespace sdl3cpp::services::impl {

/// A ymap entity as a placement in engine space.
///
/// GTA is Z-up and the engine Y-up, so (x, y, z) becomes (x, z, -y). The
/// stored rotation is the inverse of the entity's, so it is conjugated
/// before the same axis change; left as written, every rotated prop on
/// the map comes out mirrored. The band comes from lodDist, on the rings
/// in config/gta5_world.json. This is what the retired Python importer
/// wrote into the tile JSON, done here instead.
Gta5Placement MakeGta5YmapPlacement(const Gta5YmapEntity& entity);

}  // namespace sdl3cpp::services::impl
