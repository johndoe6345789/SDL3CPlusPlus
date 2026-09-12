#include "services/interfaces/workflow/gta5/gta5_effects.hpp"
#include "services/interfaces/workflow/gta5/gta5_effects_spawn.hpp"

#include <algorithm>
#include <random>

namespace sdl3cpp::services::impl {
namespace {

std::mt19937& Rng() {
    static std::mt19937 rng(1987);
    return rng;
}

glm::vec3 Scatter(float much) {
    std::uniform_real_distribution<float> spread(-much, much);
    return glm::vec3(spread(Rng()), spread(Rng()), spread(Rng()));
}

}  // namespace

void SpawnGta5Explosion(Gta5Effects& effects, const glm::vec3& at,
                        float radius) {
    Gta5Particle flash;
    flash.at = at;
    flash.colour = glm::vec3(1.f, 0.95f, 0.75f);
    flash.size = radius * 1.2f;
    flash.growth = radius * 4.f;
    flash.life = 0.12f;
    flash.sprite = kGta5Flash;
    effects.particles.push_back(flash);
    // The fireball: lumps of flame thrown out, cooling as they rise.
    for (int i = 0; i < 14; ++i) {
        Gta5Particle fire;
        fire.at = at + Scatter(radius * 0.35f);
        fire.velocity = Scatter(radius * 1.6f) + glm::vec3(0.f, radius, 0.f);
        fire.colour = glm::vec3(1.f, 0.45f + 0.2f * float(i % 3), 0.12f);
        fire.size = radius * 0.5f;
        fire.growth = radius * 0.9f;
        fire.drag = 1.6f;
        fire.life = 0.5f + 0.05f * float(i % 5);
        fire.sprite = kGta5Puff;
        effects.particles.push_back(fire);
    }
    for (int i = 0; i < 10; ++i) {
        Gta5Particle smoke;
        smoke.at = at + Scatter(radius * 0.5f);
        smoke.velocity = Scatter(radius * 0.8f) +
                         glm::vec3(0.f, radius * 0.9f, 0.f);
        smoke.colour = glm::vec3(0.28f);
        smoke.size = radius * 0.8f;
        smoke.growth = radius * 1.1f;
        smoke.drag = 0.8f;
        smoke.life = 2.4f + 0.2f * float(i);
        smoke.fade = 0.75f;
        smoke.sprite = kGta5Smoke;
        effects.particles.push_back(smoke);
    }
}

void SpawnGta5Scorch(Gta5Effects& effects, const glm::vec3& at,
                     const glm::vec3& normal, float radius, float seconds,
                     int cell, const Gta5Stuck& stuck) {
    Gta5Particle mark;
    mark.at = at + normal * 0.03f;  // clear of the surface it lies on
    mark.normal = normal;
    mark.colour = glm::vec3(1.f);
    mark.size = radius;
    mark.life = seconds;
    mark.sprite = kGta5Scorch;
    mark.cell = cell;
    // Stuck to something that moves, it is placed from there each frame
    // rather than left where the shot happened to find it.
    mark.on = stuck.on;
    mark.local = stuck.local;
    mark.localNormal = stuck.normal;
    effects.particles.push_back(mark);
}

}  // namespace sdl3cpp::services::impl
