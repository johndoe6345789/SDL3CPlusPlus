#pragma once

#include "services/interfaces/workflow/gta5/gta5_stuck.hpp"

namespace sdl3cpp::services::impl {

/// The atlas cells.
enum Gta5Sprite { kGta5Puff = 0, kGta5Flash, kGta5Smoke, kGta5Scorch };

/// One piece: facing the camera, on a surface (`normal`), or along one
/// (`length`, a tracer).
struct Gta5Particle {
    glm::vec3 at{0.f};
    glm::vec3 velocity{0.f};
    glm::vec3 normal{0.f};
    glm::vec3 colour{1.f};
    float size{1.f};
    float growth{0.f};   // metres a second
    float length{0.f};   // a streak this long along `normal`
    float gravity{0.f};  // metres a second squared, downward
    float drag{0.f};     // a share of its speed a second
    float age{0.f};
    float life{1.f};
    float fade{1.f};  // how bright it starts
    int sprite{kGta5Puff};
    int cell{-1};  // a cell of GTA's decal sheet, or -1 for the strip
    /// What a mark is stuck to, and where on it in that thing's own
    /// space: set, it is placed from there every frame.
    const btCollisionObject* on{nullptr};
    glm::vec3 local{0.f};
    glm::vec3 localNormal{0.f};
};

}  // namespace sdl3cpp::services::impl
