#include "services/interfaces/workflow/rendering/bsp_entity_trigger_internal.hpp"

namespace sdl3cpp::services::impl {

bool TryActivateTrigger(const nlohmann::json& ent, const std::string& classname,
                        const std::string& id, btRigidBody* body,
                        const btVector3& playerPos, btVector3& playerAabbMin,
                        btVector3& playerAabbMax, uint32_t frame,
                        nlohmann::json& cooldowns,
                        const std::shared_ptr<ILogger>& logger) {
    namespace detail = bsp_entity_trigger_detail;
    if (classname != "trigger_push" && classname != "trigger_teleport") {
        return false;
    }
    if (!ent.contains("bounds") ||
        !detail::AabbIntersectsBounds(playerAabbMin, playerAabbMax,
                                      ent["bounds"], 0.15f)) {
        return true;
    }

    const uint32_t lastFrame      = cooldowns.value(id, 0u);
    const uint32_t cooldownFrames = classname == "trigger_teleport" ? 45u : 15u;
    if (lastFrame != 0u && frame < lastFrame + cooldownFrames) return true;
    cooldowns[id] = frame == 0u ? 1u : frame;

    btVector3 target;
    if (!ent.contains("target_position") ||
        !ReadVec3(ent["target_position"], target)) {
        return true;
    }

    if (classname == "trigger_teleport") {
        target += btVector3(0, 1.0f, 0);
        detail::TeleportBody(body, target);
        playerAabbMin = target - btVector3(0.3f, 0.8f, 0.3f);
        playerAabbMax = target + btVector3(0.3f, 0.8f, 0.3f);
        if (logger) {
            logger->Info("bsp.entities.update: teleported player via " + id);
        }
    } else {
        body->setLinearVelocity(detail::JumpPadVelocity(playerPos, target));
        body->activate(true);
        if (logger) {
            logger->Info("bsp.entities.update: jump pad launch via " + id);
        }
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
