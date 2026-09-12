#pragma once

#include "services/interfaces/workflow/gta5/stream/gta5_lod.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

/// One entity from a ymap, already converted to engine space by
/// the ymap reader (gta5_ymap_placement), or by a legacy tile file.
struct Gta5Placement {
    std::string archetype;
    /// Jenkins hash of the archetype name, which is all a binary ymap
    /// stores. Nonzero means the drawable is found through the asset
    /// index rather than through modelPath.
    std::uint32_t archetypeHash{0};
    std::string modelPath;  // empty when the archetype was not exported
    glm::vec3 position{0.f};
    glm::quat rotation{1.f, 0.f, 0.f, 0.f};  // (w, x, y, z)
    glm::vec3 scale{1.f};
    Gta5Lod lod{Gta5Lod::Hd};
    /// Drawn from childLodDist out to lodDist; 0 lifts either limit.
    float lodDist{0.f};
    float childLodDist{0.f};
};

}  // namespace sdl3cpp::services::impl
