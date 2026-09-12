#pragma once

#include <glm/glm.hpp>

class btCollisionObject;

namespace sdl3cpp::services::impl {

/// Where a mark sits on the thing it struck, in that thing's own space.
///
/// A hole in a car has to ride in the car's space: held in the world it
/// hangs in the air the moment the car drives off. `on` null is the
/// world itself, which never moves, and leaves the mark where it fell.
struct Gta5Stuck {
    const btCollisionObject* on{nullptr};
    glm::vec3 local{0.f};
    glm::vec3 normal{0.f};
};

}  // namespace sdl3cpp::services::impl
