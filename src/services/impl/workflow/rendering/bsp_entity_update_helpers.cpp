#include "services/interfaces/workflow/rendering/bsp_entity_update_helpers.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

namespace {

bool ReadBounds(const nlohmann::json& bounds, btVector3& mn, btVector3& mx) {
    if (!bounds.is_object() || !bounds.contains("min") ||
        !bounds.contains("max")) {
        return false;
    }
    return ReadVec3(bounds["min"], mn) && ReadVec3(bounds["max"], mx);
}

bool AabbIntersectsBounds(const btVector3& bodyMin, const btVector3& bodyMax,
                          const nlohmann::json& bounds, float pad) {
    btVector3 triggerMin, triggerMax;
    if (!ReadBounds(bounds, triggerMin, triggerMax)) return false;
    return bodyMax.x() >= triggerMin.x() - pad &&
           bodyMin.x() <= triggerMax.x() + pad &&
           bodyMax.y() >= triggerMin.y() - pad &&
           bodyMin.y() <= triggerMax.y() + pad &&
           bodyMax.z() >= triggerMin.z() - pad &&
           bodyMin.z() <= triggerMax.z() + pad;
}

void TeleportBody(btRigidBody* body, const btVector3& dest) {
    btTransform xform;
    xform.setIdentity();
    xform.setOrigin(dest);
    body->setCenterOfMassTransform(xform);
    body->setWorldTransform(xform);
    if (body->getMotionState()) {
        body->getMotionState()->setWorldTransform(xform);
    }
    body->setLinearVelocity(btVector3(0, 0, 0));
    body->setAngularVelocity(btVector3(0, 0, 0));
    body->clearForces();
    body->activate(true);
}

btVector3 JumpPadVelocity(const btVector3& from, const btVector3& target) {
    constexpr float kGravity = 9.81f;
    const btVector3 delta    = target - from;
    const float horiz =
        std::sqrt(delta.x() * delta.x() + delta.z() * delta.z());
    const float t = std::clamp(horiz / 18.0f, 0.55f, 1.20f);
    return btVector3(delta.x() / t, (delta.y() + 0.5f * kGravity * t * t) / t,
                     delta.z() / t);
}

}  // namespace

bool HasPrefix(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

bool IsPickupClass(const std::string& classname) {
    return HasPrefix(classname, "weapon_") || HasPrefix(classname, "ammo_") ||
           HasPrefix(classname, "item_") || HasPrefix(classname, "holdable_");
}

bool ReadVec3(const nlohmann::json& value, btVector3& out) {
    if (!value.is_array() || value.size() != 3) return false;
    out = btVector3(value[0].get<float>(), value[1].get<float>(),
                    value[2].get<float>());
    return true;
}

bool TryCollectPickup(const nlohmann::json& ent, const std::string& classname,
                      const std::string& id, const btVector3& playerPos,
                      nlohmann::json& collected, nlohmann::json& inventory,
                      WorkflowContext& context,
                      const std::shared_ptr<ILogger>& logger) {
    if (!IsPickupClass(classname)) return false;
    if (id.empty() || collected.value(id, false)) return true;

    btVector3 itemPos;
    if (!ent.contains("position") || !ReadVec3(ent["position"], itemPos)) {
        return true;
    }
    if ((itemPos - playerPos).length2() > 1.2f * 1.2f) return true;

    collected[id]        = true;
    inventory[classname] = true;
    if (HasPrefix(classname, "weapon_")) {
        context.Set<std::string>("q3.current_weapon", classname);
    }
    if (logger) logger->Info("bsp.entities.update: picked up " + classname);
    return true;
}

bool TryActivateTrigger(const nlohmann::json& ent, const std::string& classname,
                        const std::string& id, btRigidBody* body,
                        const btVector3& playerPos, btVector3& playerAabbMin,
                        btVector3& playerAabbMax, uint32_t frame,
                        nlohmann::json& cooldowns,
                        const std::shared_ptr<ILogger>& logger) {
    if (classname != "trigger_push" && classname != "trigger_teleport") {
        return false;
    }
    if (!ent.contains("bounds") ||
        !AabbIntersectsBounds(playerAabbMin, playerAabbMax, ent["bounds"],
                              0.15f)) {
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
        TeleportBody(body, target);
        playerAabbMin = target - btVector3(0.3f, 0.8f, 0.3f);
        playerAabbMax = target + btVector3(0.3f, 0.8f, 0.3f);
        if (logger) {
            logger->Info("bsp.entities.update: teleported player via " + id);
        }
    } else {
        body->setLinearVelocity(JumpPadVelocity(playerPos, target));
        body->activate(true);
        if (logger) {
            logger->Info("bsp.entities.update: jump pad launch via " + id);
        }
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
