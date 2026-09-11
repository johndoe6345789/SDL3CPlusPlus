#include "services/interfaces/workflow/gta5/gta5_draw_context_build.hpp"

#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"

namespace sdl3cpp::services::impl {

Gta5DrawContext BuildGta5DrawContext(const WorkflowStepDefinition& step,
                                     const WorkflowContext& context) {
    Gta5DrawContext draw;
    draw.pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    draw.cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    draw.view = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    draw.proj = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.f));
    draw.shadowVP =
        context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.f));
    draw.cameraPos =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f));

    draw.fragUniforms = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});
    // Buildings are rough dielectrics. Left at the shared defaults, a
    // fully metallic surface with no environment to reflect resolves to
    // black, which is what turned the city into silhouettes. The engine's
    // own pickup draw overrides these the same way.
    draw.fragUniforms.material[0] = 0.75f;  // roughness
    draw.fragUniforms.material[1] = 0.0f;   // metallic

    // The sky step publishes the colour it painted the horizon; the
    // model shader fogs to it out of the spotlight-position slot, which
    // this package has no other use for.
    const auto horizon = context.Get<glm::vec3>(
        "gta5.sky.horizon", glm::vec3(0.32f, 0.38f, 0.48f));
    draw.fragUniforms.flash_pos[0] = horizon.x;
    draw.fragUniforms.flash_pos[1] = horizon.y;
    draw.fragUniforms.flash_pos[2] = horizon.z;

    draw.texture = context.Get<SDL_GPUTexture*>(
        Gta5ParameterOr(step, "texture_key", "walls_texture_gpu"), nullptr);
    draw.sampler = context.Get<SDL_GPUSampler*>(
        Gta5ParameterOr(step, "sampler_key", "walls_texture_sampler"),
        nullptr);
    draw.shadowTexture =
        context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    draw.shadowSampler =
        context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr);
    return draw;
}

}  // namespace sdl3cpp::services::impl
