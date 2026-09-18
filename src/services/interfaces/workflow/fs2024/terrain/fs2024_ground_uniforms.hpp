#pragma once

#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_uniforms.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

#include <glm/glm.hpp>

#include <array>

namespace sdl3cpp::services::impl {

/// Matches FragmentUniforms in fs2024_ground.frag. Lit exactly as the
/// terrain pipeline lights buildings, plus FS2024's material table.
struct Fs2024GroundFragmentUniforms {
    glm::vec4 sunDir{0.f, -1.f, 0.f, 0.f};
    glm::vec4 sunColour{1.f};
    glm::vec4 ambient{0.4f, 0.45f, 0.55f, 1.f};
    glm::vec4 fog{0.55f, 0.6f, 0.7f, 0.00006f};
    glm::vec4 material{64.f, 64.f, 0.f, 0.f};  ///< repeat metres, map size
    std::array<glm::vec4, kFs2024LandClasses> table{};
};

/// The ground's uniforms from the terrain's lighting and the world's
/// material table; one FS2024 material repeats every `repeatMetres`.
Fs2024GroundFragmentUniforms BuildFs2024GroundFragmentUniforms(
    const Fs2024TerrainFragmentUniforms& lighting, const Fs2024World& world,
    float repeatMetres, int classMapSize);

}  // namespace sdl3cpp::services::impl
