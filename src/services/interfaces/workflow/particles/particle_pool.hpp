#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace sdl3cpp::services::impl {

/// Describes one burst of particles. Each particle's velocity is `velocity`
/// plus a random offset of up to `spread` metres per second.
struct ParticleEmitSpec {
    glm::vec3 origin{0.0f};
    glm::vec3 velocity{0.0f};
    float spread = 0.0f;
    float minLifetime = 1.0f;
    float maxLifetime = 1.0f;
    float size = 0.5f;
    float sizeGrowth = 0.0f;
    float drag = 0.0f;
    float gravityScale = 1.0f;
    std::uint32_t count = 0;
};

struct Particle {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    float age = 0.0f;
    float lifetime = 1.0f;
    float size = 0.5f;
    float sizeGrowth = 0.0f;
    float drag = 0.0f;
    float gravityScale = 1.0f;

    float Progress() const { return age / lifetime; }
};

/// A pool of simulated particles. Emit adds a burst; Step advances all of
/// them and removes the ones that have reached their lifetime.
class ParticlePool {
public:
    explicit ParticlePool(std::uint32_t seed = 1u);

    void Emit(const ParticleEmitSpec& spec);
    void Step(float deltaTime, const glm::vec3& gravity);

    const std::vector<Particle>& Particles() const { return particles_; }
    std::size_t Count() const { return particles_.size(); }

private:
    float RandomUnit();
    float RandomSigned();
    glm::vec3 RandomInSphere(float radius);

    std::vector<Particle> particles_;
    std::mt19937 rng_;
};

}  // namespace sdl3cpp::services::impl
