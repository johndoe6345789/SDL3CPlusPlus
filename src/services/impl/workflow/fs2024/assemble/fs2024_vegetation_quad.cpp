#include "services/interfaces/workflow/fs2024/assemble/fs2024_vegetation_quad.hpp"

namespace sdl3cpp::services::impl {
namespace {

void AddQuad(Fs2024TerrainChunkMesh& mesh, const glm::vec3& a,
            const glm::vec3& b, const glm::vec3& c, const glm::vec3& d,
            const glm::vec3& normal, float v0, float v1, float u0,
            float u1, float layer) {
    const auto first = static_cast<std::uint32_t>(mesh.vertices.size());
    const glm::vec3 corners[4] = {a, b, c, d};
    const float us[4] = {u0, u1, u1, u0};
    const float vs[4] = {v0, v0, v1, v1};
    for (int i = 0; i < 4; ++i) {
        mesh.vertices.push_back(BspRenderVertex{
            corners[i].x, corners[i].y, corners[i].z, us[i], vs[i], layer,
            0.f, normal.x, normal.y, normal.z});
        mesh.min = glm::min(mesh.min, corners[i]);
        mesh.max = glm::max(mesh.max, corners[i]);
    }
    mesh.indices.insert(mesh.indices.end(),
                        {first, first + 1, first + 2, first, first + 2,
                         first + 3});
}

}  // namespace

void AppendFs2024VegetationQuad(Fs2024TerrainChunkMesh& mesh, float x,
                                float z, float groundY, float width,
                                float height, float offsetY, int layer,
                                int frames, int frameCol) {
    const float y0 = groundY + offsetY;
    const float y1 = y0 + height;
    const float h = width * 0.5f;
    const float u0 = static_cast<float>(frameCol) /
                     static_cast<float>(frames);
    const float u1 = static_cast<float>(frameCol + 1) /
                     static_cast<float>(frames);
    const float v1 = 1.f / static_cast<float>(frames);
    const float layerF = static_cast<float>(layer);
    AddQuad(mesh, {x - h, y0, z}, {x + h, y0, z}, {x + h, y1, z},
           {x - h, y1, z}, {0.f, 0.f, 1.f}, 0.f, v1, u0, u1, layerF);
    AddQuad(mesh, {x, y0, z - h}, {x, y0, z + h}, {x, y1, z + h},
           {x, y1, z - h}, {1.f, 0.f, 0.f}, 0.f, v1, u0, u1, layerF);
}

}  // namespace sdl3cpp::services::impl
