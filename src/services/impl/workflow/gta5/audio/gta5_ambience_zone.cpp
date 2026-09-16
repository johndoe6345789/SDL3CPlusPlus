#include "services/interfaces/workflow/gta5/audio/gta5_ambience.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

bool Gta5InAmbientZone(const Gta5AmbientZone& z, const glm::vec3& p) {
    const float a     = glm::radians(z.angle);
    const glm::vec3 d = p - z.centre;
    const float x     = d.x * std::cos(a) + d.y * std::sin(a);
    const float y     = -d.x * std::sin(a) + d.y * std::cos(a);
    // Zones are thin slabs; a street or two of height still counts.
    return std::abs(x) <= z.half.x && std::abs(y) <= z.half.y &&
           std::abs(d.z) <= std::max(z.half.z, 50.f);
}

bool Gta5AmbientRuleAwake(const Gta5AmbientRule& r, float minutes) {
    return r.start <= r.end ? minutes >= r.start && minutes < r.end
                            : minutes >= r.start || minutes < r.end;
}

}  // namespace sdl3cpp::services::impl
