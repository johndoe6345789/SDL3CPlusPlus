#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include <functional>

namespace sdl3cpp::services::impl {

glm::vec3 RacerBinderColour(const std::string& racer) {
    // Energy binders differ from pod to pod; a few film-like hues,
    // chosen by name so each racer keeps theirs.
    static const glm::vec3 kHues[] = {{0.45f, 0.75f, 1.f},
                                      {1.f, 0.55f, 0.2f},
                                      {0.5f, 1.f, 0.55f},
                                      {0.85f, 0.5f, 1.f},
                                      {1.f, 0.9f, 0.4f}};
    return kHues[std::hash<std::string>{}(racer) % 5];
}

}  // namespace sdl3cpp::services::impl
