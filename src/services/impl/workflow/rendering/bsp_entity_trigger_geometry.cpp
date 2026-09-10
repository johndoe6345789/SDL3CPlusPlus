#include "services/interfaces/workflow/rendering/bsp_entity_trigger_internal.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl::bsp_entity_trigger_detail {
namespace {

bool ReadBounds(const nlohmann::json& bounds, btVector3& mn, btVector3& mx) {
    if (!bounds.is_object() || !bounds.contains("min") ||
        !bounds.contains("max")) {
        return false;
    }
    return ReadVec3(bounds["min"], mn) && ReadVec3(bounds["max"], mx);
}

}  // namespace

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

}  // namespace sdl3cpp::services::impl::bsp_entity_trigger_detail
