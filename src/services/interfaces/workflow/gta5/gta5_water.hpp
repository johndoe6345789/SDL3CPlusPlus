#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// One water.xml water quad, in GTA metres: x east, y north, z its height.
/// Type 0 is the whole rectangle; 1 to 4 are right triangles that leave
/// out one corner, the one on the shore.
struct Gta5WaterQuad {
    float minX{0.f}, maxX{0.f}, minY{0.f}, maxY{0.f}, z{0.f};
    int type{0};
};

/// The visible quads of water.xml's <WaterQuads>, the calming and wave
/// quads aside. Empty when the file is missing.
std::vector<Gta5WaterQuad> LoadGta5WaterQuads(const std::string& path);

/// Triangles in engine space -- GTA's (x, z, -y) -- facing up.
std::vector<BspRenderVertex> BuildGta5WaterMesh(
    const std::vector<Gta5WaterQuad>& quads);

/// gta5_water.frag's uniforms, std140.
struct Gta5WaterUniforms {
    glm::vec4 lightDir, lightColor, ambient, horizon, zenith, cameraPos;
    glm::vec4 params;  // seconds, exposure, reflection on, mirror height
    glm::vec4 screen;  // 1 / render target size
};

/// This frame's: the light render.prepare built, the clock's sky, the
/// camera, and whether gta5.reflection.texture has anything in it.
Gta5WaterUniforms BuildGta5WaterUniforms(const WorkflowContext& context,
                                         bool reflection);

}  // namespace sdl3cpp::services::impl
