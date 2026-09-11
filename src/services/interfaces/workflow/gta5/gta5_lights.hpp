#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// One of GTA's distant lights -- the far-off street lamps and lit windows
/// of lodlights.rpf -- in engine space, with its colour and brightness.
struct Gta5DistantLight {
    glm::vec3 position{0.f};
    glm::vec3 colour{1.f};
    float intensity{0.f};  // 0..1
};

/// Every <DistantLODLightsSOA> in `dir`'s distlodlights_*.ymap.xml, as
/// GTAUtil writes them: a position list of x, y, z items and an RGBI list
/// of one number a light -- intensity in the top byte, then red, green and
/// blue.
std::vector<Gta5DistantLight> LoadGta5DistantLights(const std::string& dir);

/// Six vertices a light, a square gta5_lights.vert turns to the camera:
/// the position its centre, the uv its corner (-1..1), the normal its
/// colour and lm_u its intensity.
std::vector<BspRenderVertex> BuildGta5LightSprites(
    const std::vector<Gta5DistantLight>& lights);

/// gta5_lights.vert's uniforms, std140.
struct Gta5LightUniforms {
    glm::mat4 viewProj{1.f};
    glm::vec4 right{1.f, 0.f, 0.f, 0.f};  // the camera's, in world space
    glm::vec4 up{0.f, 1.f, 0.f, 0.f};
    glm::vec4 camera{0.f};
    glm::vec4 params{0.f};  // night, metres a metre away, fade from, to
};

}  // namespace sdl3cpp::services::impl
