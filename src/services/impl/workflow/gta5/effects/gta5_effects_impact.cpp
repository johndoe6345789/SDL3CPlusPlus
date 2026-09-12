#include "services/interfaces/workflow/gta5/effects/gta5_effects.hpp"
#include "services/interfaces/workflow/gta5/effects/gta5_effects_spawn.hpp"
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

/// The cells of GTA's sheet that look like a round went in: dark
/// holes with a rim, and the cracked concrete bangs.
constexpr int kHoles[] = {16, 17, 25, 26, 27, 3, 4};

void SpawnGta5Impact(Gta5Effects& effects, const glm::vec3& at,
                     const glm::vec3& normal, const Gta5Stuck& stuck) {
    for (int i = 0; i < 5; ++i) {
        Gta5Particle spark;
        spark.at = at;
        spark.velocity = normal * 2.5f + Scatter(2.f);
        spark.colour = glm::vec3(1.f, 0.7f, 0.3f);
        spark.size = 0.06f;
        spark.gravity = 9.f;
        spark.life = 0.18f + Spread(0.06f);
        spark.sprite = kGta5Flash;
        effects.particles.push_back(spark);
    }
    Gta5Particle dust;
    dust.at = at + normal * 0.05f;
    dust.velocity = normal * 0.8f;
    dust.colour = glm::vec3(0.38f, 0.33f, 0.28f);
    dust.size = 0.4f;
    dust.growth = 1.8f;
    dust.drag = 3.f;
    dust.life = 0.6f;
    dust.fade = 0.95f;
    dust.sprite = kGta5Smoke;
    effects.particles.push_back(dust);
    const std::size_t pick =
        static_cast<std::size_t>(Rng()() % (sizeof(kHoles) /
                                            sizeof(kHoles[0])));
    SpawnGta5Scorch(effects, at, normal, 0.25f, 90.f, kHoles[pick],
                    stuck);
}

}  // namespace sdl3cpp::services::impl
