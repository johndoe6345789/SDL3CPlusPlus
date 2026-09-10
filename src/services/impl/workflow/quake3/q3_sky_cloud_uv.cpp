#include "services/interfaces/workflow/quake3/q3_sky_cloud_uv.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// tr_sky.c's radiusWorld: the curvature the cloud layer is draped over.
constexpr float kRadiusWorld = 4096.0f;

float Square(float value) {
    return value * value;
}

}  // namespace

glm::vec2 CloudTexCoords(const glm::vec3& direction, float heightCloud) {
    // Quake is Z-up and this engine is Y-up, and bsp.load mirrors Y into
    // -Z, so undo that before using ioq3's formula verbatim.
    const glm::vec3 sky(direction.x, -direction.z, direction.y);

    const float r        = kRadiusWorld;
    const float h        = heightCloud;
    const float lengthSq = glm::dot(sky, sky);
    if (lengthSq <= 0.0f) {
        return glm::vec2(0.0f);
    }

    const float discriminant =
        Square(sky.z) * Square(r) + 2.0f * Square(sky.x) * r * h +
        Square(sky.x) * Square(h) + 2.0f * Square(sky.y) * r * h +
        Square(sky.y) * Square(h) + 2.0f * Square(sky.z) * r * h +
        Square(sky.z) * Square(h);
    const float p =
        (1.0f / (2.0f * lengthSq)) *
        (-2.0f * sky.z * r + 2.0f * std::sqrt(std::max(discriminant, 0.0f)));

    glm::vec3 hit = sky * p;
    hit.z += r;
    const float hitLength = glm::length(hit);
    if (hitLength <= 0.0f) {
        return glm::vec2(0.0f);
    }
    hit /= hitLength;

    return glm::vec2(std::acos(std::clamp(hit.x, -1.0f, 1.0f)),
                     std::acos(std::clamp(hit.y, -1.0f, 1.0f)));
}

}  // namespace sdl3cpp::services::impl
