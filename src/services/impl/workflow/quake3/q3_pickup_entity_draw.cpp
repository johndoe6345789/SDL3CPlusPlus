#include "services/interfaces/workflow/quake3/q3_pickup_entity_draw.hpp"

#include "services/interfaces/workflow/quake3/q3_item_models.hpp"
#include "services/interfaces/workflow/quake3/q3_item_transform.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_draw_surfaces.hpp"
#include "services/interfaces/workflow/quake3/q3_pickup_classify.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_constants.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"

namespace sdl3cpp::services::impl {

void DrawSinglePickup(const std::string& classname, glm::vec3 pos,
                      int drawIndex, const glm::vec3& camRight,
                      const glm::vec3& camUp, const glm::mat4& view,
                      const glm::mat4& proj, const glm::vec3& camPos,
                      const glm::mat4& shadowVP, float time,
                      SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                      WorkflowContext& context) {
    (void)camRight;
    (void)camUp;

    const std::string prefix = Q3ItemModelPrefix(classname);
    if (prefix.empty()) return;

    pos.y += Q3ItemBobHeight(time, drawIndex);
    if (HasClassnamePrefix(classname, "weapon_")) {
        // CG_Item gives a dropped weapon an extra height boost.
        pos.y += q3::FromQuakeUnits(8.0f);
    }

    auto fu = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});
    fu.material[0] = 0.35f;
    fu.material[1] = 0.0f;

    const float yaw = Q3ItemSpinYaw(time, Q3ItemSpinsFast(classname));
    DrawMd3Surfaces(
        prefix, 0, Q3ItemMatrix(pos, yaw), view, proj, camPos, shadowVP, fu,
        pass, cmd,
        context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr),
        context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr), context);
}

}  // namespace sdl3cpp::services::impl
