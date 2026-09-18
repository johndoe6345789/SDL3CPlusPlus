#include "services/interfaces/workflow/fs2024/terrain/fs2024_ground_uniforms.hpp"

#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

namespace sdl3cpp::services::impl {

Fs2024GroundFragmentUniforms BuildFs2024GroundFragmentUniforms(
    const Fs2024TerrainFragmentUniforms& lighting, const Fs2024World& world,
    float repeatMetres, int classMapSize) {
    Fs2024GroundFragmentUniforms uniforms;
    uniforms.sunDir = lighting.sunDir;
    uniforms.sunColour = lighting.sunColour;
    uniforms.ambient = lighting.ambient;
    uniforms.fog = lighting.fog;
    uniforms.material =
        glm::vec4(repeatMetres, static_cast<float>(classMapSize), 0.f, 0.f);
    uniforms.table = world.materialTable;
    return uniforms;
}

}  // namespace sdl3cpp::services::impl
