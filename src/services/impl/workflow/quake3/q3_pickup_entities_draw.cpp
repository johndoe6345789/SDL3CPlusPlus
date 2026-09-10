#include "services/interfaces/workflow/quake3/q3_pickup_entities_draw.hpp"
#include "services/interfaces/workflow/quake3/q3_pickup_classify.hpp"
#include "services/interfaces/workflow/quake3/q3_pickup_entity_draw.hpp"

namespace sdl3cpp::services::impl {

void DrawPickupEntities(const nlohmann::json& entities,
                        const nlohmann::json& collected, const glm::mat4& view,
                        const glm::mat4& proj, const glm::vec3& camPos,
                        const glm::mat4& shadowVP, float time,
                        SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                        WorkflowContext& context) {
    // Each item draws its own md3, which binds its own buffers.
    const glm::vec3 camRight(view[0][0], view[1][0], view[2][0]);
    const glm::vec3 camUp(view[0][1], view[1][1], view[2][1]);

    int drawn = 0;
    for (const auto& ent : entities) {
        const std::string classname = ent.value("classname", std::string{});
        const std::string id        = ent.value("id", std::string{});
        if (!IsPickup(classname) || collected.value(id, false)) continue;
        glm::vec3 pos;
        if (!ent.contains("position") || !ReadVec3(ent["position"], pos)) {
            continue;
        }
        if (++drawn > 96) break;

        DrawSinglePickup(classname, pos, drawn, camRight, camUp, view, proj,
                         camPos, shadowVP, time, pass, cmd, context);
    }
}

}  // namespace sdl3cpp::services::impl
