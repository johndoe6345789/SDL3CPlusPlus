#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

/// One mesh instance from a bl4x-baked tile's placements.json, already
/// in engine space (bl4x's walker writes Y-up metres and xyzw quaternions,
/// the same convention glTF uses -- see D:\BL4Export\src\cpp\README.md).
struct Bl4Placement {
    std::string archetype;
    std::string modelPath;  // relative to the map root, e.g. "models/SM_X.obj"
    glm::vec3 position{0.f};
    glm::quat rotation{1.f, 0.f, 0.f, 0.f};  // (w, x, y, z)
    glm::vec3 scale{1.f};
};

/// Identifies one placement across the tiles that list it: its model
/// and its transform, quantised to a millimetre so the same instance
/// read twice hashes the same.
std::uint64_t Bl4PlacementIdentity(const Bl4Placement& placement);

}  // namespace sdl3cpp::services::impl
