// Billboard rules: each live particle becomes two triangles facing the
// camera, and a particle that has reached its lifetime draws nothing.

#include "services/interfaces/workflow/particles/particle_billboards.hpp"
#include "services/interfaces/workflow/particles/particle_pool.hpp"

#include <gtest/gtest.h>

namespace impl = sdl3cpp::services::impl;

TEST(ParticleBillboards, LiveParticleMakesTwoTriangles) {
    impl::Particle live;
    live.lifetime = 1.0f;
    live.size = 1.0f;

    std::vector<impl::BspRenderVertex> vertices;
    impl::AppendParticleBillboards({live}, glm::vec3(1, 0, 0),
                                   glm::vec3(0, 1, 0), vertices);

    EXPECT_EQ(vertices.size(), 6u);
    EXPECT_FLOAT_EQ(vertices.front().nz, 1.0f);
}

TEST(ParticleBillboards, FullyAgedParticleDrawsNothing) {
    impl::Particle spent;
    spent.age = 1.0f;
    spent.lifetime = 1.0f;
    spent.size = 1.0f;

    std::vector<impl::BspRenderVertex> vertices;
    impl::AppendParticleBillboards({spent}, glm::vec3(1, 0, 0),
                                   glm::vec3(0, 1, 0), vertices);

    EXPECT_TRUE(vertices.empty());
}
