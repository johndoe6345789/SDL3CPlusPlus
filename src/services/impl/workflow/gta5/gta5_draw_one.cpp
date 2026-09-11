#include "services/interfaces/workflow/gta5/gta5_draw_one.hpp"

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

int DrawGta5Instance(const Gta5Instance& instance,
                     const Gta5DrawContext& draw,
                     SDL_GPUTexture*& boundTexture) {
    const Gta5Geometry* geometry = instance.geometry;
    if (!geometry || !geometry->usable) return 0;

    glm::mat4 model(1.f);
    std::memcpy(glm::value_ptr(model), instance.modelMatrix.data(),
                sizeof(float) * 16);
    const rendering::VertexUniformData vu = MakeVertexUniforms(draw, model);

    int drawn = 0;
    for (const Gta5SubMesh& sub : geometry->subMeshes) {
        if (!sub.indexCount) continue;
        // A submesh with no texture of its own falls back to the package
        // default rather than being dropped.
        SDL_GPUTexture* texture = sub.texture ? sub.texture : draw.texture;
        SDL_GPUSampler* sampler = sub.texture ? sub.sampler : draw.sampler;
        if (!texture || !sampler) continue;

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
        // The gta5 fragment shader reads the spotlight slot, which it
        // has no use for, as this submesh's tint and alpha threshold.
        rendering::FragmentUniformData fu = draw.fragUniforms;
        fu.flash_color[0] = sub.surface[0];
        fu.flash_color[1] = sub.surface[1];
        fu.flash_color[2] = sub.surface[2];
        fu.flash_color[3] = sub.surface[3];

        SDL_PushGPUVertexUniformData(draw.cmd, 0, &vu, sizeof(vu));
        SDL_PushGPUFragmentUniformData(draw.cmd, 0, &fu, sizeof(fu));
        SDL_DrawGPUIndexedPrimitives(draw.pass, sub.indexCount, 1, 0, 0, 0);
        ++drawn;
    }
    return drawn;
}

}  // namespace sdl3cpp::services::impl
