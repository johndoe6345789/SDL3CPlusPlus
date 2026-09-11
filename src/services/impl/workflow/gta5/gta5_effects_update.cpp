#include "services/interfaces/workflow/gta5/gta5_effects.hpp"
#include "services/interfaces/workflow/gta5/gta5_effects_spawn.hpp"
#include <algorithm>

namespace sdl3cpp::services::impl {

void UpdateGta5Effects(Gta5Effects& effects, float dt) {
    for (Gta5Particle& p : effects.particles) {
        p.age += dt;
        if (p.length > 0.f || p.normal != glm::vec3(0.f)) continue;
        p.velocity.y -= p.gravity * dt;
        p.velocity -= p.velocity * std::min(1.f, p.drag * dt);
        p.at += p.velocity * dt;
    }
    const auto spent = std::remove_if(
        effects.particles.begin(), effects.particles.end(),
        [](const Gta5Particle& p) { return p.age >= p.life; });
    effects.particles.erase(spent, effects.particles.end());
}

}  // namespace sdl3cpp::services::impl
