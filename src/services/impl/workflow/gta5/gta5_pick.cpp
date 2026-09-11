#include "services/interfaces/workflow/gta5/gta5_pick.hpp"

#include "services/interfaces/workflow/gta5/gta5_asset_index.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::size_t kShown = 8;

/// How far along the ray it enters the sphere: 0 from inside, -1 missed.
float Enter(const glm::vec3& eye, const glm::vec3& ahead, const glm::vec3& c,
            float r) {
    const glm::vec3 to = c - eye;
    const float along = glm::dot(to, ahead);
    const float miss = glm::dot(to, to) - along * along;
    if (miss > r * r || along + r < 0.f) return -1.f;
    return std::max(along - std::sqrt(r * r - miss), 0.f);
}

std::string Describe(const Gta5StreamState& state, const Gta5Instance& in,
                     float at, float radius) {
    std::string line =
        "gta5.pick: " +
        (state.assets ? Gta5ArchetypeName(*state.assets, in.archetype)
                      : std::string("?")) +
        " at " + std::to_string(static_cast<int>(at)) + " m, radius " +
        std::to_string(static_cast<int>(radius)) + ", lod " +
        std::to_string(static_cast<int>(in.childLodDist)) + "-" +
        std::to_string(static_cast<int>(in.lodDist)) +
        (in.childLodDist > 0.f ? " (LOD parent)" : "") + ", textures";
    for (const Gta5SubMesh& sub : in.geometry->subMeshes) {
        line += !sub.texture ? std::string(" none")
                             : " " + std::to_string(sub.textureSize[0]) +
                                   "x" + std::to_string(sub.textureSize[1]);
    }
    return line;
}

}  // namespace

void LogGta5Pick(const Gta5StreamState& state, const glm::vec3& eye,
                 const glm::vec3& ahead,
                 const std::shared_ptr<ILogger>& logger) {
    if (!logger) return;
    struct Hit { float at; float radius; const Gta5Instance* in; };
    std::vector<Hit> hits;
    for (const Gta5Instance* in : state.batch.visible) {
        if (!in || !in->geometry || in->geometry->bounds[3] < 0.f) continue;
        const glm::mat4 m = glm::make_mat4(in->modelMatrix.data());
        const auto& b = in->geometry->bounds;
        const glm::vec3 c(m * glm::vec4(b[0], b[1], b[2], 1.f));
        const float r = b[3] * glm::length(glm::vec3(m[0]));
        const float at = Enter(eye, ahead, c, r);
        if (at >= 0.f) hits.push_back({at, r, in});
    }
    std::sort(hits.begin(), hits.end(),
              [](const Hit& a, const Hit& b) { return a.at < b.at; });
    logger->Info("gta5.pick: " + std::to_string(hits.size()) +
                 " drawn instances under the crosshair");
    for (std::size_t i = 0; i < hits.size() && i < kShown; ++i) {
        logger->Info(Describe(state, *hits[i].in, hits[i].at, hits[i].radius));
    }
}

}  // namespace sdl3cpp::services::impl
