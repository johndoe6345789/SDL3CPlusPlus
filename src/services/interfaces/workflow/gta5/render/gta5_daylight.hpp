#pragma once

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// The light and sky for one time of day.
struct Gta5Daylight {
    glm::vec3 lightDir{0.f, -1.f, 0.f};  // the way the light travels
    glm::vec3 lightColor{1.f};           // colour times intensity
    glm::vec3 ambient{0.4f};
    float exposure{0.2f};
    glm::vec4 horizon{0.3f, 0.36f, 0.46f, 0.f};  // a: how many stars
    glm::vec4 zenith{0.05f, 0.14f, 0.42f, 8.f};  // a: disc size
    float night{0.f};                            // 0 by day, 1 at night
};

/// The sun rises in the east (+x) at six, crosses the southern sky (+z:
/// north is -z) and sets in the west at eighteen, low and warm at each
/// end. After dark the moon, opposite, lights the city dimly and blue,
/// and the sky goes from blue through dusk to night and stars.
Gta5Daylight ComputeGta5Daylight(float hours);

}  // namespace sdl3cpp::services::impl
