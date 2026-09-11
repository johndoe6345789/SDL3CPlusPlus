#include "services/interfaces/workflow/gta5/gta5_ymap_placement.hpp"

namespace sdl3cpp::services::impl {
namespace {

Gta5Lod BandForLodDist(float lodDist) {
    if (lodDist <= 300.f) return Gta5Lod::Hd;
    if (lodDist <= 1000.f) return Gta5Lod::Lod;
    if (lodDist <= 3000.f) return Gta5Lod::Slod1;
    return Gta5Lod::Slod2;
}

}  // namespace

Gta5Placement MakeGta5YmapPlacement(const Gta5YmapEntity& entity) {
    Gta5Placement p;
    p.archetypeHash = entity.archetype;

    const glm::vec3& g = entity.position;
    p.position = glm::vec3(g.x, g.z, -g.y);

    // Conjugate to (-x, -y, -z, w), then the axis change takes (x, y, z)
    // to (x, z, -y): (-x, -z, y, w). glm::quat is (w, x, y, z).
    const glm::vec4& q = entity.rotation;
    p.rotation = glm::quat(q.w, -q.x, -q.z, q.y);

    // Engine Y is GTA Z; engine X and Z are both GTA's XY plane.
    const float xy = entity.scaleXY != 0.f ? entity.scaleXY : 1.f;
    const float z = entity.scaleZ != 0.f ? entity.scaleZ : 1.f;
    p.scale = glm::vec3(xy, z, xy);
    p.lod = BandForLodDist(entity.lodDist);
    // A lodDist of 0 defers to the archetype's, which lives in the ytyp
    // files this does not read: leave it unlimited.
    p.lodDist = entity.lodDist > 0.f ? entity.lodDist : 0.f;
    p.childLodDist = entity.numChildren > 0 ? entity.childLodDist : 0.f;
    return p;
}

}  // namespace sdl3cpp::services::impl
