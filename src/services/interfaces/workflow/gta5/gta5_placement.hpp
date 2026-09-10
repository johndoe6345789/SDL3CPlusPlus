#pragma once

#include "services/interfaces/workflow/gta5/gta5_lod.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

namespace sdl3cpp::services::impl {

/// One entity from a ymap, already converted to engine space by
/// packages/gta5/tools/import_codewalker_export.py.
struct Gta5Placement {
    std::string archetype;
    std::string modelPath;  // empty when the archetype was not exported
    glm::vec3 position{0.f};
    glm::quat rotation{1.f, 0.f, 0.f, 0.f};  // (w, x, y, z)
    glm::vec3 scale{1.f};
    Gta5Lod lod{Gta5Lod::Hd};
};

}  // namespace sdl3cpp::services::impl
