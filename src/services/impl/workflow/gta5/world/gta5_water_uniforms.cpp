#include "services/interfaces/workflow/gta5/world/gta5_water.hpp"

#include "services/interfaces/workflow/rendering/rendering_types.hpp"

#include <SDL3/SDL_timer.h>

#include <algorithm>
#include <cstdint>

namespace sdl3cpp::services::impl {

Gta5WaterUniforms BuildGta5WaterUniforms(const WorkflowContext& context,
                                         bool reflection) {
    const auto fu = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});
    Gta5WaterUniforms w;
    w.lightDir = glm::vec4(fu.light_dir[0], fu.light_dir[1], fu.light_dir[2],
                           0.f);
    w.lightColor = glm::vec4(fu.light_color[0], fu.light_color[1],
                             fu.light_color[2], 0.f);
    w.ambient = glm::vec4(fu.ambient[0], fu.ambient[1], fu.ambient[2], 0.f);
    w.horizon = context.Get<glm::vec4>("gta5.time.horizon",
                                       glm::vec4(0.3f, 0.36f, 0.46f, 0.f));
    w.zenith = context.Get<glm::vec4>("gta5.time.zenith",
                                      glm::vec4(0.05f, 0.14f, 0.42f, 0.f));
    w.cameraPos = glm::vec4(
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f)), 1.f);
    // Seconds, wrapped long before a float loses the fractions the swell
    // moves by.
    const std::uint64_t ms = SDL_GetTicks() % 3600000u;
    const float exposure = fu.light_color[3] > 0.f ? fu.light_color[3] : 1.f;
    w.params = glm::vec4(static_cast<float>(ms) / 1000.f, exposure,
                         reflection ? 1.f : 0.f,
                         context.Get<float>("gta5.reflection.height", 0.f));
    const auto width = std::max(context.Get<uint32_t>("render_width", 1u), 1u);
    const auto height =
        std::max(context.Get<uint32_t>("render_height", 1u), 1u);
    w.screen = glm::vec4(1.f / static_cast<float>(width),
                         1.f / static_cast<float>(height), 0.f, 0.f);
    return w;
}

}  // namespace sdl3cpp::services::impl
