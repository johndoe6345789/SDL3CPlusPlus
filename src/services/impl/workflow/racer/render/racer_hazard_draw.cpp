#include "services/interfaces/workflow/racer/render/racer_model_draw.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kRockSize = 2.5f;
constexpr float kRockDrop = 40.f;      // metres a rock falls from
constexpr float kPlumeHeight = 26.f;
constexpr float kPlumeRadius = 3.f;

/// A frame whose +z runs from `from` to `to` (scaled to its length).
glm::mat4 Along(const glm::vec3& from, const glm::vec3& to, float width) {
    const glm::vec3 d = to - from;
    const float length = glm::length(d);
    if (length < 1e-3f) return glm::translate(glm::mat4(1.f), from);
    const glm::vec3 z = d / length;
    const glm::vec3 up = std::fabs(z.y) > 0.9f ? glm::vec3(1, 0, 0)
                                               : glm::vec3(0, 1, 0);
    const glm::vec3 x = glm::normalize(glm::cross(up, z));
    const glm::vec3 y = glm::cross(z, x);
    glm::mat4 m(1.f);
    m[0] = glm::vec4(x * width, 0.f);
    m[1] = glm::vec4(y * width, 0.f);
    m[2] = glm::vec4(d, 0.f);
    m[3] = glm::vec4(from, 1.f);
    return m;
}

int DrawRock(RacerDrawPass& d, const RacerWorldState& state,
             const glm::vec3& at) {
    // Two squat cones base to base make a rough boulder.
    int drawn = 0;
    for (const float flip : {1.f, -1.f}) {
        glm::mat4 m = glm::translate(glm::mat4(1.f), at);
        m = glm::rotate(m, -1.5707963f * flip, glm::vec3(1.f, 0.f, 0.f));
        m = glm::scale(m, glm::vec3(kRockSize, kRockSize, 0.7f * kRockSize));
        drawn += DrawRacerModel(d, state.effects.rock, m, true);
    }
    return drawn;
}

}  // namespace

int DrawRacerHazards(RacerDrawPass& d, const RacerWorldState& state) {
    int drawn = 0;
    for (const RacerHazard& h : state.hazards) {
        if (h.kind == RacerHazardKind::Blaster && h.flash > 0.f) {
            drawn += DrawRacerModel(d, state.effects.bolt,
                                    Along(h.at, h.target, 1.f), true);
        } else if (RacerHazardErupting(h)) {
            // The plume rises then falls back over the eruption.
            const float rise = std::sin(3.14159265f * h.clock / 1.8f);
            const glm::vec3 top = h.at + glm::vec3(0.f, kPlumeHeight * rise,
                                                   0.f);
            drawn += DrawRacerModel(d, state.effects.flame,
                                    Along(h.at, top, kPlumeRadius), true);
        } else if (h.kind == RacerHazardKind::Rockfall) {
            const float fall = RacerRockFall(h);
            if (fall < 0.f || fall > 2.f) continue;
            const float height = kRockDrop * std::max(0.f, 1.f - fall);
            drawn += DrawRock(d, state, h.at + glm::vec3(0.f, height, 0.f));
        }
    }
    return drawn;
}

}  // namespace sdl3cpp::services::impl
