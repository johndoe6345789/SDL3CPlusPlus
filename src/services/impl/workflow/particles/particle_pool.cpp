#include "services/interfaces/workflow/particles/particle_pool.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

ParticlePool::ParticlePool(std::uint32_t seed) : rng_(seed) {}

float ParticlePool::RandomUnit() {
    return std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_);
}

float ParticlePool::RandomSigned() {
    return std::uniform_real_distribution<float>(-1.0f, 1.0f)(rng_);
}

glm::vec3 ParticlePool::RandomInSphere(float radius) {
    for (;;) {
        const glm::vec3 candidate(RandomSigned(), RandomSigned(),
                                  RandomSigned());
        if (glm::dot(candidate, candidate) <= 1.0f) {
            return candidate * radius;
        }
    }
}

void ParticlePool::Emit(const ParticleEmitSpec& spec) {
    particles_.reserve(particles_.size() + spec.count);
    for (std::uint32_t i = 0; i < spec.count; ++i) {
        Particle particle;
        particle.position = spec.origin;
        particle.velocity = spec.velocity + RandomInSphere(spec.spread);
        particle.lifetime =
            spec.minLifetime +
            (spec.maxLifetime - spec.minLifetime) * RandomUnit();
        particle.size = spec.size;
        particle.sizeGrowth = spec.sizeGrowth;
        particle.drag = spec.drag;
        particle.gravityScale = spec.gravityScale;
        particles_.push_back(particle);
    }
}

void ParticlePool::Step(float deltaTime, const glm::vec3& gravity) {
    for (Particle& particle : particles_) {
        particle.velocity += gravity * particle.gravityScale * deltaTime;
        const float retained =
            std::max(0.0f, 1.0f - particle.drag * deltaTime);
        particle.velocity *= retained;
        particle.position += particle.velocity * deltaTime;
        particle.size += particle.sizeGrowth * deltaTime;
        particle.age += deltaTime;
    }

    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(),
                       [](const Particle& particle) {
                           return particle.age >= particle.lifetime;
                       }),
        particles_.end());
}

}  // namespace sdl3cpp::services::impl
