#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Sky geometry in the BSP vertex format, so it can be drawn with the
/// existing BSP pipeline instead of a bespoke shader (this build has no
/// shader compiler available to add one).
struct SkyDomeMesh {
    std::vector<BspRenderVertex> vertices;
    std::vector<uint16_t> indices;
};

/**
 * @brief Builds a dome of `radius` spanning zenith to below the horizon,
 * with ioq3 cloud-layer texture coordinates scaled by `uvScale`.
 *
 * Quake skies are a cloud layer projected onto a dome rather than a
 * six-sided box: q3dm1's textures/skies/tim_hell declares
 * `skyparms - 384 -`, so both box slots are empty and only the shader's
 * cloud stages exist. Triangles are wound to be visible from inside,
 * since the camera sits at the dome's centre.
 */
SkyDomeMesh BuildSkyDome(float radius, int segments, int rings,
                         float uvScale);

}  // namespace sdl3cpp::services::impl
