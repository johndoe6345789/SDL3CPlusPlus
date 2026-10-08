#pragma once

#include "services/interfaces/workflow/particles/particle_pool.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace sdl3cpp::services::impl {

/// Appends one camera-facing quad (two triangles, six vertices) per particle.
/// `right` and `up` are the camera's axes. BspRenderVertex has no alpha, so
/// a particle fades by shrinking to nothing over its lifetime.
void AppendParticleBillboards(const std::vector<Particle>& particles,
                              const glm::vec3& right, const glm::vec3& up,
                              std::vector<BspRenderVertex>& out);

}  // namespace sdl3cpp::services::impl
