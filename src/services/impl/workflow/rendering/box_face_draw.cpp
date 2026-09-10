#include "services/interfaces/workflow/rendering/box_face_draw.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {

void BindBoxTextures(SDL_GPURenderPass* pass, SDL_GPUTexture* texture,
                     SDL_GPUSampler* sampler, SDL_GPUTexture* shadowTex,
                     SDL_GPUSampler* shadowSamp) {
    if (shadowTex && shadowSamp) {
        SDL_GPUTextureSamplerBinding bindings[2] = {};
        bindings[0].texture                      = texture;
        bindings[0].sampler                      = sampler;
        bindings[1].texture                      = shadowTex;
        bindings[1].sampler                      = shadowSamp;
        SDL_BindGPUFragmentSamplers(pass, 0, bindings, 2);
    } else {
        SDL_GPUTextureSamplerBinding tex_binding = {};
        tex_binding.texture                      = texture;
        tex_binding.sampler                      = sampler;
        SDL_BindGPUFragmentSamplers(pass, 0, &tex_binding, 1);
    }
}

void DrawBoxFaces(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                  const std::array<BoxFace, 6>& faces, const glm::vec3& center,
                  const glm::mat4& bodyRotation, const glm::mat4& view,
                  const glm::mat4& proj, const glm::vec3& camPos,
                  const glm::mat4& shadowVP,
                  const rendering::FragmentUniformData& fu,
                  uint32_t indexCount) {
    for (const auto& f : faces) {
        glm::mat4 model_mat =
            glm::translate(glm::mat4(1.0f), center) * bodyRotation *
            glm::translate(glm::mat4(1.0f), f.offset) * f.rotation *
            glm::scale(glm::mat4(1.0f), glm::vec3(f.scaleW, 1.0f, f.scaleD));

        glm::mat4 mvp = proj * view * model_mat;
        glm::vec3 worldNormal =
            glm::vec3(bodyRotation * glm::vec4(f.normal, 0.0f));

        rendering::VertexUniformData vu = {};
        std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
        std::memcpy(vu.model_mat, glm::value_ptr(model_mat),
                    sizeof(float) * 16);
        vu.normal[0]     = worldNormal.x;
        vu.normal[1]     = worldNormal.y;
        vu.normal[2]     = worldNormal.z;
        vu.uv_scale[0]   = f.uvW;
        vu.uv_scale[1]   = f.uvH;
        vu.camera_pos[0] = camPos.x;
        vu.camera_pos[1] = camPos.y;
        vu.camera_pos[2] = camPos.z;
        std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP), sizeof(float) * 16);

        SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
        SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));
        SDL_DrawGPUIndexedPrimitives(pass, indexCount, 1, 0, 0, 0);
    }
}

}  // namespace sdl3cpp::services::impl
