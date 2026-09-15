#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// One brush side's plane, in engine space: the brush's interior is
/// where dot(normal, point) <= dist.
struct BrushPlane {
    glm::vec3 normal{0.f, 1.f, 0.f};
    float dist = 0.f;
};

/// A convex brush as Quake stores it: a run of side planes, plus the
/// bounds used to reject it cheaply.
struct CollisionBrush {
    int firstSide  = 0;
    int numSides   = 0;
    glm::vec3 mins{0.f};
    glm::vec3 maxs{0.f};
    /// Player-clip brushes stop pmove but not shots, as Quake's
    /// MASK_PLAYERSOLID / MASK_SHOT split does.
    bool playerClip = false;
};

/// The map's brushes as planes, which is what Quake actually traces
/// against. Kept instead of the convex hulls Bullet needs because a
/// plane trace can report the exact surface that stopped a sweep, and a
/// hull sweep cannot.
struct BrushCollisionModel {
    std::vector<BrushPlane> sides;
    std::vector<CollisionBrush> brushes;

    bool empty() const { return brushes.empty(); }
};

/**
 * @brief Builds the brush collision model from a BSP.
 *
 * Planes are converted to engine space (Quake is Z-up and scaled) so
 * traces need no per-call conversion. Brushes that are neither solid nor
 * player clip are dropped, as they are for the render-side hulls.
 */
BrushCollisionModel BuildBrushCollisionModel(
    const std::vector<uint8_t>& bspData, float scale);

}  // namespace sdl3cpp::services::impl
