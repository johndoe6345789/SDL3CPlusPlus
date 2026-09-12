#include "services/interfaces/workflow/gta5/render/gta5_daylight.hpp"

#include <glm/gtc/constants.hpp>

#include <cmath>

namespace sdl3cpp::services::impl {

Gta5Daylight ComputeGta5Daylight(float hours) {
    const float a = (hours - 6.f) / 12.f * glm::pi<float>();
    const glm::vec3 sun =
        glm::normalize(glm::vec3(std::cos(a) * 0.8f, std::sin(a), 0.45f));
    const float up = sun.y;
    const float day = glm::smoothstep(-0.12f, 0.2f, up);
    // Sunrise and sunset: the sun near the horizon, either side of it.
    const float low = 1.f - glm::smoothstep(0.f, 0.35f, std::abs(up));
    Gta5Daylight d;
    d.night = 1.f - day;
    if (up > -0.03f) {
        const glm::vec3 warm(1.f, 0.5f, 0.25f), white(1.f, 0.96f, 0.9f);
        d.lightDir = -sun;
        d.lightColor = glm::mix(warm, white, glm::smoothstep(0.f, 0.4f, up)) *
                       2.2f * glm::smoothstep(-0.03f, 0.1f, up);
    } else {
        // The moon, across the sky from the sun but still to the south.
        const glm::vec3 moon = glm::normalize(glm::vec3(-sun.x, -up, sun.z));
        d.lightDir = -moon;
        d.lightColor = glm::vec3(0.3f, 0.36f, 0.52f) * 0.45f *
                       glm::smoothstep(-0.03f, -0.15f, up);
    }
    d.ambient = glm::mix(glm::vec3(0.05f, 0.06f, 0.1f),
                         glm::vec3(0.38f, 0.42f, 0.52f), day) +
                glm::vec3(0.12f, 0.06f, 0.02f) * low * day;
    d.exposure = glm::mix(0.6f, 0.2f, day);
    const glm::vec3 dayH(0.3f, 0.36f, 0.46f), dayZ(0.05f, 0.14f, 0.42f);
    const glm::vec3 duskH(0.85f, 0.45f, 0.28f), duskZ(0.14f, 0.15f, 0.34f);
    const glm::vec3 nightH(0.025f, 0.035f, 0.07f);
    const glm::vec3 nightZ(0.004f, 0.008f, 0.022f);
    glm::vec3 h = glm::mix(nightH, dayH, day);
    glm::vec3 z = glm::mix(nightZ, dayZ, day);
    h = glm::mix(h, duskH, low * 0.85f);
    z = glm::mix(z, duskZ, low * 0.6f);
    // Stars only once dusk has gone: they read wrong in a bright sky.
    d.horizon = glm::vec4(h, glm::smoothstep(0.85f, 1.f, d.night));
    d.zenith = glm::vec4(z, up > -0.03f ? 8.f : 3.f);
    return d;
}

}  // namespace sdl3cpp::services::impl
