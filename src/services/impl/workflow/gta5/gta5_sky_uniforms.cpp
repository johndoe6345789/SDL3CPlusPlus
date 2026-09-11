#include "services/interfaces/workflow/gta5/gta5_sky_uniforms.hpp"

#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"

#include <glm/gtc/matrix_inverse.hpp>

namespace sdl3cpp::services::impl {
namespace {

glm::vec4 ColourParam(const WorkflowStepDefinition& step, const char* prefix,
                      glm::vec4 fallback) {
    const std::string base(prefix);
    fallback.r = Gta5NumberOr(step, base + "_r", fallback.r);
    fallback.g = Gta5NumberOr(step, base + "_g", fallback.g);
    fallback.b = Gta5NumberOr(step, base + "_b", fallback.b);
    return fallback;
}

}  // namespace

Gta5SkyUniforms BuildGta5SkyUniforms(const WorkflowStepDefinition& step,
                                     const WorkflowContext& context) {
    Gta5SkyUniforms sky;
    const auto view =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    const auto proj =
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.f));
    sky.invViewProj = glm::inverse(proj * view);

    const auto eye =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f));
    sky.cameraPos = glm::vec4(eye, 1.f);

    // The same light the city is shaded by, so the sun sits where the
    // shadows say it should.
    const auto frag = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});
    sky.sunDir = glm::vec4(frag.light_dir[0], frag.light_dir[1],
                           frag.light_dir[2], 0.f);

    sky.horizon = ColourParam(step, "horizon", sky.horizon);
    sky.zenith = ColourParam(step, "zenith", sky.zenith);
    sky.zenith.a = Gta5NumberOr(step, "sun_size", sky.zenith.a);
    sky.horizon.a = 0.f;  // no stars without a clock
    // The clock's sky when there is a clock (gta5.time): dusk and night.
    sky.horizon = context.Get<glm::vec4>("gta5.time.horizon", sky.horizon);
    sky.zenith = context.Get<glm::vec4>("gta5.time.zenith", sky.zenith);
    // Under water, the sky is murk: the flag rides in the sun's w.
    sky.sunDir.w =
        context.Get<float>("gta5.camera.underwater", -1.f) > 0.f ? 1.f : 0.f;
    return sky;
}

}  // namespace sdl3cpp::services::impl
