#include "services/interfaces/workflow/gta5/gta5_draw_instances.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

rendering::VertexUniformData MakeVertexUniforms(
    const Gta5DrawContext& draw, const glm::mat4& model) {
    const glm::mat4 mvp = draw.proj * draw.view * model;
    rendering::VertexUniformData vu = {};
    std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
    std::memcpy(vu.model_mat, glm::value_ptr(model), sizeof(float) * 16);
    std::memcpy(vu.shadow_vp, glm::value_ptr(draw.shadowVP),
                sizeof(float) * 16);
    vu.normal[1] = 1.0f;
    vu.uv_scale[0] = 1.0f;
    vu.uv_scale[1] = 1.0f;
    vu.camera_pos[0] = draw.cameraPos.x;
    vu.camera_pos[1] = draw.cameraPos.y;
    vu.camera_pos[2] = draw.cameraPos.z;
    return vu;
}

}  // namespace

int DrawGta5Instances(const Gta5StreamState& state,
                      const Gta5DrawContext& draw) {
    if (!draw.pass || !draw.cmd) return 0;

    int drawn = 0;
    SDL_GPUTexture* boundTexture = nullptr;
    for (const auto& entry : state.resident) {
        for (const Gta5Instance& instance : entry.second.instances) {
            const Gta5Geometry* geometry = instance.geometry;
            if (!geometry || !geometry->usable) continue;

            glm::mat4 model(1.f);
            std::memcpy(glm::value_ptr(model), instance.modelMatrix.data(),
                        sizeof(float) * 16);
            const rendering::VertexUniformData vu =
                MakeVertexUniforms(draw, model);

            for (const Gta5SubMesh& sub : geometry->subMeshes) {
                if (!sub.indexCount) continue;
                // A submesh with no texture of its own falls back to the
                // package default rather than being dropped.
                SDL_GPUTexture* texture =
                    sub.texture ? sub.texture : draw.texture;
                SDL_GPUSampler* sampler =
                    sub.texture ? sub.sampler : draw.sampler;
                if (!texture || !sampler) continue;

                // Textures repeat across a district; rebind on change.
                if (texture != boundTexture) {
                    SDL_GPUTextureSamplerBinding binding = {texture, sampler};
                    SDL_BindGPUFragmentSamplers(draw.pass, 0, &binding, 1);
                    boundTexture = texture;
                }

                SDL_GPUBufferBinding vb = {sub.vertexBuffer, 0};
                SDL_BindGPUVertexBuffers(draw.pass, 0, &vb, 1);
                SDL_GPUBufferBinding ib = {sub.indexBuffer, 0};
                SDL_BindGPUIndexBuffer(draw.pass, &ib,
                                       SDL_GPU_INDEXELEMENTSIZE_16BIT);
                SDL_PushGPUVertexUniformData(draw.cmd, 0, &vu, sizeof(vu));
                SDL_PushGPUFragmentUniformData(draw.cmd, 0, &draw.fragUniforms,
                                               sizeof(draw.fragUniforms));
                SDL_DrawGPUIndexedPrimitives(draw.pass, sub.indexCount, 1,
                                             0, 0, 0);
                ++drawn;
            }
        }
    }
    return drawn;
}

}  // namespace sdl3cpp::services::impl
