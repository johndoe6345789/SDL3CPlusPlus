#include "services/interfaces/workflow/quake3/q3_sky_dome.hpp"

#include "services/interfaces/workflow/quake3/q3_sky_cloud_uv.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265358979f;

/// Sweep past the horizon so the dome's rim never shows through a sky
/// opening the player is looking up at from below.
constexpr float kThetaMax = kPi * 0.58f;

}  // namespace

SkyDomeMesh BuildSkyDome(float radius, int segments, int rings) {
    SkyDomeMesh mesh;
    if (segments < 3 || rings < 1) {
        return mesh;
    }

    const int stride = segments + 1;
    mesh.vertices.reserve(static_cast<size_t>(stride) * (rings + 1));
    for (int r = 0; r <= rings; ++r) {
        const float v     = static_cast<float>(r) / static_cast<float>(rings);
        const float theta = v * kThetaMax;
        const float sinT  = std::sin(theta);
        const float cosT  = std::cos(theta);
        for (int s = 0; s <= segments; ++s) {
            const float u =
                static_cast<float>(s) / static_cast<float>(segments);
            const float phi = u * 2.0f * kPi;

            const glm::vec3 dir(sinT * std::cos(phi), cosT,
                                sinT * std::sin(phi));
            const glm::vec2 uv = CloudTexCoords(dir, kDefaultCloudHeight);

            BspRenderVertex vert{};
            vert.x    = radius * dir.x;
            vert.y    = radius * dir.y;
            vert.z    = radius * dir.z;
            vert.u    = uv.x;
            vert.v    = uv.y;
            vert.lm_u = 0.0f;
            vert.lm_v = 0.0f;
            vert.nx   = -dir.x;
            vert.ny   = -dir.y;
            vert.nz   = -dir.z;
            mesh.vertices.push_back(vert);
        }
    }

    mesh.indices.reserve(static_cast<size_t>(segments) * rings * 6);
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < segments; ++s) {
            const auto a = static_cast<uint16_t>(r * stride + s);
            const auto b = static_cast<uint16_t>(a + 1);
            const auto c = static_cast<uint16_t>(a + stride);
            const auto d = static_cast<uint16_t>(c + 1);
            // Wound so the inward face is the front face: the camera is
            // at the dome's centre, and the pipeline culls back faces.
            mesh.indices.insert(mesh.indices.end(), {a, c, b, b, c, d});
        }
    }
    return mesh;
}

}  // namespace sdl3cpp::services::impl
