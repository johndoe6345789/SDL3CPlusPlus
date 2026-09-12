#include "services/interfaces/workflow/gta5/effects/gta5_effects.hpp"
#include "services/interfaces/workflow/gta5/effects/gta5_effects_spawn.hpp"

#include <random>

namespace sdl3cpp::services::impl {
namespace {

std::mt19937& Rng() {
    static std::mt19937 rng(20260911);
    return rng;
}

float Spread(float much) {
    return std::uniform_real_distribution<float>(-much, much)(Rng());
}

glm::vec3 Scatter(float much) {
    return glm::vec3(Spread(much), Spread(much), Spread(much));
}

}  // namespace

Gta5EffectsPtr Gta5EffectsOf(WorkflowContext& context) {
    auto effects = context.Get<Gta5EffectsPtr>("gta5.effects", nullptr);
    if (!effects) {
        effects = std::make_shared<Gta5Effects>();
        context.Set<Gta5EffectsPtr>("gta5.effects", effects);
    }
    return effects;
}

void SpawnGta5Muzzle(Gta5Effects& effects, const glm::vec3& at,
                     const glm::vec3& ahead) {
    Gta5Particle flash;
    flash.at = at + ahead * 0.1f;
    flash.colour = glm::vec3(1.f, 0.85f, 0.45f);
    flash.size = 0.22f;
    flash.growth = 0.6f;
    flash.life = 0.05f;
    flash.fade = 1.f;
    flash.sprite = kGta5Flash;
    effects.particles.push_back(flash);
    Gta5Particle smoke;
    smoke.at = flash.at;
    smoke.velocity = ahead * 1.6f + glm::vec3(0.f, 0.4f, 0.f);
    smoke.colour = glm::vec3(0.7f);
    smoke.size = 0.1f;
    smoke.growth = 0.5f;
    smoke.drag = 2.5f;
    smoke.life = 0.5f;
    smoke.fade = 0.35f;
    smoke.sprite = kGta5Smoke;
    effects.particles.push_back(smoke);
}

void SpawnGta5Tracer(Gta5Effects& effects, const glm::vec3& from,
                     const glm::vec3& to) {
    const glm::vec3 along = to - from;
    const float far = glm::length(along);
    if (far < 0.5f) return;
    Gta5Particle streak;
    streak.at = from;
    streak.normal = along / far;
    streak.length = far;
    streak.size = 0.09f;
    streak.colour = glm::vec3(1.f, 0.8f, 0.4f);
    streak.life = 0.13f;
    streak.sprite = kGta5Puff;
    effects.particles.push_back(streak);
}

}  // namespace sdl3cpp::services::impl
