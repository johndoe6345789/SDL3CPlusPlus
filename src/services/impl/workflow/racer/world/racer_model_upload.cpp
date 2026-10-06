#include "services/interfaces/workflow/racer/world/racer_gpu_upload.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

bool IsDecal(const RacerMaterialRef& m) {
    return m.format == RacerTextureFormat::Intensity4 ||
           m.format == RacerTextureFormat::Intensity8;
}

RacerGpuVertex ToGpu(const RacerModelVertex& v, RacerVertexShading shading) {
    const glm::vec3 p = RacerToEngine(v.x, v.y, v.z);
    if (shading == RacerVertexShading::Colour) {
        return {p.x, p.y, p.z, v.u, v.v, v.a / 255.f, 0.f,
                v.r / 255.f, v.g / 255.f, v.b / 255.f};
    }
    // Lit vertices keep a signed normal in the colour bytes. Light them
    // once from above and in front, in the model's own space.
    const glm::vec3 n(static_cast<std::int8_t>(v.r),
                      static_cast<std::int8_t>(v.g),
                      static_cast<std::int8_t>(v.b));
    const float length = glm::length(n);
    const glm::vec3 light = glm::normalize(glm::vec3(0.3f, 0.5f, 0.8f));
    const float diffuse =
        length > 0.f ? std::max(0.f, glm::dot(n / length, light)) : 0.5f;
    const float shade = 0.45f + 0.65f * diffuse;
    return {p.x, p.y, p.z, v.u, v.v, v.a / 255.f, 0.f, shade, shade, shade};
}

}  // namespace

RacerGpuModel UploadRacerModel(SDL_GPUDevice* device, RacerWorldState& state,
                               const RacerModel& model, RacerGround* ground,
                               RacerVertexShading shading) {
    RacerGpuModel out;
    for (const RacerModelBatch& batch : model.batches) {
        std::vector<RacerGpuVertex> vertices;
        vertices.reserve(batch.vertices.size());
        for (const RacerModelVertex& v : batch.vertices) {
            vertices.push_back(ToGpu(v, shading));
        }
        const bool decal = IsDecal(batch.material);
        if (ground && !decal) {
            for (std::size_t i = 0; i + 2 < vertices.size(); i += 3) {
                const auto& a = vertices[i];
                const auto& b = vertices[i + 1];
                const auto& c = vertices[i + 2];
                AddRacerGroundTriangle(*ground, {a.x, a.y, a.z},
                                       {b.x, b.y, b.z}, {c.x, c.y, c.z});
            }
        }
        RacerGpuBatch gpu;
        gpu.vertices = UploadRacerVertexBuffer(device, vertices);
        gpu.vertexCount = static_cast<std::uint32_t>(vertices.size());
        const RacerGpuTexture texture =
            AcquireRacerTexture(device, state, batch.material);
        gpu.texture = texture.texture;
        gpu.sampler = texture.sampler;
        gpu.blended = decal;
        if (gpu.vertices) out.batches.push_back(gpu);
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
