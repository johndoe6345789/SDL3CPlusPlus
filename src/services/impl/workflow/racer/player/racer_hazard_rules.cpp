#include "racer_hazard_rules.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kBoltHitChance = 0.35f;
constexpr float kBoltDamage = 0.12f;
constexpr float kRockDamage = 0.2f;

float Random(std::uint32_t& seed) {
    seed = seed * 1664525u + 1013904223u;
    return static_cast<float>(seed >> 8) / 16777216.f;
}

void Hurt(RacerPodState& pod, int side, float amount) {
    float& damage = pod.engineDamage[side];
    damage = std::min(1.f, damage + amount);
}

}  // namespace

bool FireRacerBlaster(RacerHazard& h,
                      const std::vector<RacerPodState*>& pods) {
    RacerPodState* nearest = nullptr;
    float best = h.radius;
    for (RacerPodState* pod : pods) {
        const float d = glm::distance(pod->position, h.at);
        if (d < best) {
            best = d;
            nearest = pod;
        }
    }
    if (!nearest) return false;
    const bool hit = Random(h.seed) < kBoltHitChance;
    const glm::vec3 miss(Random(h.seed) * 8.f - 4.f, -1.f, 0.f);
    h.target = nearest->position + (hit ? glm::vec3(0.f) : miss);
    h.flash = 0.15f;
    if (hit) Hurt(*nearest, Random(h.seed) < 0.5f ? 0 : 1, kBoltDamage);
    return true;
}

void RunRacerEruption(const RacerHazard& h,
                      const std::vector<RacerPodState*>& pods, float dt) {
    for (RacerPodState* pod : pods) {
        if (glm::distance(pod->position, h.at) > h.radius) continue;
        pod->heat = std::min(1.f, pod->heat + 0.6f * dt);
        pod->verticalSpeed = std::max(pod->verticalSpeed, 14.f);
    }
}

void DropRacerRock(const RacerHazard& h,
                   const std::vector<RacerPodState*>& pods) {
    for (RacerPodState* pod : pods) {
        if (glm::distance(pod->position, h.at) > h.radius) continue;
        Hurt(*pod, 0, kRockDamage);
        Hurt(*pod, 1, kRockDamage);
        pod->speed *= 0.5f;
    }
}

}  // namespace sdl3cpp::services::impl
