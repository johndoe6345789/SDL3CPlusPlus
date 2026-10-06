#pragma once

#include <glm/glm.hpp>

#include <optional>

namespace sdl3cpp::services::impl {

/// What the pod flies over: the height of the highest surface below
/// `ceiling` at (x, z) (nothing over a gap), and whether a wall stands
/// between two points. `context` is passed back to both.
struct RacerSurface {
    std::optional<float> (*height)(const void* context, float x, float z,
                                   float ceiling) = nullptr;
    bool (*wall)(const void* context, const glm::vec3& from,
                 const glm::vec3& to) = nullptr;
    const void* context = nullptr;
};

}  // namespace sdl3cpp::services::impl
