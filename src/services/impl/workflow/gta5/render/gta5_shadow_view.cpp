#include "services/interfaces/workflow/gta5/render/gta5_shadow.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kBack = 800.f;  // how far upwind the box starts

}  // namespace

glm::mat4 Gta5SunShadowViewProj(const glm::vec3& lightDir,
                                const glm::vec3& focus, float radius,
                                int size) {
    const glm::vec3 l = glm::normalize(lightDir);
    const glm::vec3 up = std::abs(l.y) > 0.99f ? glm::vec3(0.f, 0.f, 1.f)
                                               : glm::vec3(0.f, 1.f, 0.f);
    const glm::vec3 right = glm::normalize(glm::cross(l, up));
    const glm::vec3 above = glm::cross(right, l);
    // Whole texels across the light; along it, nothing moves on screen.
    const float texel = 2.f * radius / static_cast<float>(size);
    const float r = glm::dot(focus, right), a = glm::dot(focus, above);
    const glm::vec3 snapped = focus +
                              right * (std::floor(r / texel) * texel - r) +
                              above * (std::floor(a / texel) * texel - a);
    const glm::mat4 view = glm::lookAt(snapped - l * kBack, snapped, above);
    return glm::orthoRH_ZO(-radius, radius, -radius, radius, 1.f,
                           2.f * kBack) *
           view;
}

}  // namespace sdl3cpp::services::impl
