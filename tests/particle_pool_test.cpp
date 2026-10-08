// Particle pool rules: a burst starts where it was emitted, gravity and
// drag act on it each step, and it is removed once its lifetime is reached.

#include "services/interfaces/workflow/particles/particle_pool.hpp"

#include <gtest/gtest.h>

namespace impl = sdl3cpp::services::impl;

namespace {

impl::ParticleEmitSpec Burst(std::uint32_t count, float lifetime) {
    impl::ParticleEmitSpec spec;
    spec.origin = glm::vec3(1.0f, 2.0f, 3.0f);
    spec.velocity = glm::vec3(0.0f, 4.0f, 0.0f);
    spec.minLifetime = lifetime;
    spec.maxLifetime = lifetime;
    spec.count = count;
    return spec;
}

}  // namespace

TEST(ParticlePool, EmitStartsEveryParticleAtTheOrigin) {
    impl::ParticlePool pool(7u);
    pool.Emit(Burst(5, 1.0f));

    ASSERT_EQ(pool.Count(), 5u);
    for (const impl::Particle& particle : pool.Particles()) {
        EXPECT_EQ(particle.position, glm::vec3(1.0f, 2.0f, 3.0f));
        EXPECT_FLOAT_EQ(particle.lifetime, 1.0f);
    }
}

TEST(ParticlePool, GravityPullsParticlesDownOverTime) {
    impl::ParticlePool pool(7u);
    pool.Emit(Burst(1, 10.0f));

    pool.Step(0.5f, glm::vec3(0.0f, -10.0f, 0.0f));

    const impl::Particle& particle = pool.Particles().front();
    EXPECT_FLOAT_EQ(particle.velocity.y, 4.0f - 5.0f);
    EXPECT_FLOAT_EQ(particle.age, 0.5f);
}

TEST(ParticlePool, DragSlowsParticlesEachStep) {
    impl::ParticlePool pool(7u);
    impl::ParticleEmitSpec spec = Burst(1, 10.0f);
    spec.drag = 1.0f;
    pool.Emit(spec);

    pool.Step(0.5f, glm::vec3(0.0f));

    EXPECT_FLOAT_EQ(pool.Particles().front().velocity.y, 2.0f);
}

TEST(ParticlePool, ParticlesAreRemovedAtTheEndOfTheirLifetime) {
    impl::ParticlePool pool(7u);
    pool.Emit(Burst(3, 1.0f));

    pool.Step(0.5f, glm::vec3(0.0f));
    EXPECT_EQ(pool.Count(), 3u);

    pool.Step(0.5f, glm::vec3(0.0f));
    EXPECT_EQ(pool.Count(), 0u);
}
