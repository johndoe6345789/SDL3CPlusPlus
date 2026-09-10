#include "services/interfaces/workflow/quake3/q3_pickup_entity_draw.hpp"
#include "services/interfaces/workflow/quake3/q3_pickup_classify.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstring>

namespace sdl3cpp::services::impl {

void DrawSinglePickup(const std::string& classname, glm::vec3 pos,
                      int drawIndex, const glm::vec3& camRight,
                      const glm::vec3& camUp, const glm::mat4& view,
                      const glm::mat4& proj, const glm::vec3& camPos,
                      const glm::mat4& shadowVP, float time,
                      SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                      WorkflowContext& context) {
    const float bob =
        std::sin(time * 3.0f + static_cast<float>(drawIndex)) * 0.08f;
    const float size = HasPrefix(classname, "weapon_") ? 0.9f : 0.55f;
    pos.y += 0.45f + bob;
    glm::mat4 model(1.0f);
    model[0] = glm::vec4(camRight * size, 0.0f);
    model[1] = glm::vec4(camUp * size, 0.0f);
    model[2] =
        glm::vec4(glm::normalize(glm::cross(camRight, camUp)) * size, 0.0f);
    model[3] = glm::vec4(pos, 1.0f);

    rendering::VertexUniformData vu = {};
    glm::mat4 mvp                   = proj * view * model;
    std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
    std::memcpy(vu.model_mat, glm::value_ptr(model), sizeof(float) * 16);
    vu.normal[1]     = 1.0f;
    vu.uv_scale[0]   = 1.0f;
    vu.uv_scale[1]   = 1.0f;
    vu.camera_pos[0] = camPos.x;
    vu.camera_pos[1] = camPos.y;
    vu.camera_pos[2] = camPos.z;
    std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP), sizeof(float) * 16);

    auto fu = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});
    fu.material[0] = 0.35f;
    fu.material[1] = 0.0f;

    const std::string texKey = TextureKeyForClass(classname);
    auto* tex  = context.Get<SDL_GPUTexture*>(texKey + "_gpu", nullptr);
    auto* samp = context.Get<SDL_GPUSampler*>(texKey + "_sampler", nullptr);
    if (!tex || !samp) return;
    SDL_GPUTextureSamplerBinding bindings[2] = {};
    bindings[0].texture                      = tex;
    bindings[0].sampler                      = samp;
    bindings[1].texture                      = tex;
    bindings[1].sampler                      = samp;
    SDL_BindGPUFragmentSamplers(pass, 0, bindings, 2);
    SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
    SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));
    SDL_DrawGPUIndexedPrimitives(pass, 6, 1, 0, 0, 0);
}

}  // namespace sdl3cpp::services::impl
