#include "services/interfaces/workflow/gta5/gta5_effects.hpp"
#include <random>

namespace sdl3cpp::services::impl {

namespace {

std::mt19937& Rng() {
    static std::mt19937 rng(31337);
    return rng;
}

float Spread(float much) {
    return std::uniform_real_distribution<float>(-much, much)(Rng());
}

glm::vec3 Scatter(float much) {
    return glm::vec3(Spread(much), Spread(much), Spread(much));
}

}  // namespace

void SpawnGta5Impact(Gta5Effects& effects, const glm::vec3& at,
                     const glm::vec3& normal) {
    for (int i = 0; i < 5; ++i) {
        Gta5Particle spark;
        spark.at = at;
        spark.velocity = normal * 2.5f + Scatter(2.f);
        spark.colour = glm::vec3(1.f, 0.7f, 0.3f);
        spark.size = 0.03f;
        spark.gravity = 9.f;
        spark.life = 0.18f + Spread(0.06f);
        spark.sprite = kGta5Flash;
        effects.particles.push_back(spark);
    }
    Gta5Particle dust;
    dust.at = at + normal * 0.05f;
    dust.velocity = normal * 0.8f;
    dust.colour = glm::vec3(0.62f, 0.58f, 0.52f);
    dust.size = 0.12f;
    dust.growth = 0.9f;
    dust.drag = 3.f;
    dust.life = 0.6f;
    dust.fade = 0.7f;
    dust.sprite = kGta5Smoke;
    effects.particles.push_back(dust);
    SpawnGta5Scorch(effects, at, normal, 0.06f, 90.f);
}

}  // namespace sdl3cpp::services::impl
