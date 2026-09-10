#pragma once

#include "core/vertex.hpp"
#include "services/interfaces/workflow/gta5/gta5_lod.hpp"

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

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

/// One archetype's mesh, loaded once and copied per instance.
struct Gta5Geometry {
    std::vector<core::Vertex> vertices;
    std::vector<std::uint16_t> indices;
    /// False once loading has failed, so a broken asset is not retried.
    bool usable{false};
};

}  // namespace sdl3cpp::services::impl
