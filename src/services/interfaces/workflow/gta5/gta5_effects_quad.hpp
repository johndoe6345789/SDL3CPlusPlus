#pragma once

#include "services/interfaces/workflow/gta5/gta5_effects.hpp"

namespace sdl3cpp::services::impl {

/// One piece as two triangles, about `centre` and along its own axes.
/// A piece wearing a cell of GTA's sheet takes its corners from there;
/// the rest take them from the drawn strip.
void AppendGta5Quad(std::vector<BspRenderVertex>& out,
                    const Gta5Particle& p, const glm::vec3& centre,
                    const glm::vec3& across, const glm::vec3& down,
                    float alpha);

/// Two directions across the surface a mark lies on.
void Gta5DecalAxes(const glm::vec3& normal, glm::vec3& right,
                   glm::vec3& ahead);

}  // namespace sdl3cpp::services::impl
