#pragma once

#include "services/interfaces/workflow/gta5/gta5_resource.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// One CEntityDef, in GTA's own frame: Z up, rotation stored inverted.
struct Gta5YmapEntity {
    std::uint32_t archetype{0};
    glm::vec3 position{0.f};
    glm::vec4 rotation{0.f, 0.f, 0.f, 1.f};  // x, y, z, w as stored
    float scaleXY{1.f};
    float scaleZ{1.f};
    /// Drawn out to this distance...
    float lodDist{100.f};
    /// ...and, when it has children -- the finer models it stands in
    /// for -- only beyond this one, where they hand over to it.
    float childLodDist{0.f};
    std::uint32_t numChildren{0};
};

/// The entities of a binary .ymap.
///
/// A ymap is an RSC7 resource holding a Meta block: a list of typed data
/// blocks (+0x30, count at +0x4C), each a run of one structure, with the
/// root -- a CMapData -- named by a 1-based id at +0x1C. Pointers inside
/// the data are block references, not page pointers. Verified against
/// GTAUtil's XML on all 3,940 ymaps that place anything: 725,777
/// entities, matching in name, position, rotation and scale.
std::vector<Gta5YmapEntity> ReadGta5YmapEntities(const Gta5Resource& res);

}  // namespace sdl3cpp::services::impl
