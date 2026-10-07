#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include <algorithm>
#include <cmath>
#include <map>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kHorizonShare = 0.15f;  // lowest share of the sky's height

/// The texel under `u, v` (wrapping), times the vertex colour, 0..1.
glm::vec3 Shade(const RacerTexture& texture, const RacerModelVertex& v) {
    glm::vec3 colour(v.r / 255.f, v.g / 255.f, v.b / 255.f);
    if (texture.width <= 0 || texture.height <= 0) return colour;
    const float u = v.u - std::floor(v.u);
    const float t = v.v - std::floor(v.v);
    const int x = std::min(texture.width - 1,
                           static_cast<int>(u * texture.width));
    const int y = std::min(texture.height - 1,
                           static_cast<int>(t * texture.height));
    const std::uint8_t* p = &texture.rgba[4 * (y * texture.width + x)];
    return colour * glm::vec3(p[0] / 255.f, p[1] / 255.f, p[2] / 255.f);
}

}  // namespace

std::optional<glm::vec3> RacerSkyHorizonColour(
    const RacerAssetLibrary& library, const RacerModel& sky) {
    float low = 1e9f, high = -1e9f;
    for (const RacerModelBatch& batch : sky.batches) {
        for (const RacerModelVertex& v : batch.vertices) {
            low = std::min(low, v.z);
            high = std::max(high, v.z);
        }
    }
    if (high <= low) return std::nullopt;
    const float cut = low + kHorizonShare * (high - low);
    glm::vec3 sum(0.f);
    int count = 0;
    for (const RacerModelBatch& batch : sky.batches) {
        const RacerTexture texture =
            DecodeRacerMaterialTexture(library, batch.material);
        for (const RacerModelVertex& v : batch.vertices) {
            if (v.z > cut) continue;
            sum += Shade(texture, v);
            ++count;
        }
    }
    if (count == 0) return std::nullopt;
    return sum / static_cast<float>(count);
}

}  // namespace sdl3cpp::services::impl
