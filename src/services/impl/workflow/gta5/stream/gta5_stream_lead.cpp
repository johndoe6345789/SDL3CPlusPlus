#include "services/interfaces/workflow/gta5/stream/gta5_stream_lead.hpp"

#include <btBulletDynamicsCommon.h>

#include <algorithm>

namespace sdl3cpp::services::impl {

glm::vec3 Gta5StreamLead(const Gta5StreamState& state,
                         const glm::vec3& playerVelocity) {
    if (!state.streaming.prefetchEnabled) return glm::vec3(0.f);
    glm::vec3 velocity = playerVelocity;
    if (state.seated >= 0 &&
        state.seated < static_cast<int>(state.vehicles.size())) {
        if (const btRigidBody* chassis = state.vehicles[state.seated].chassis) {
            const btVector3& v = chassis->getLinearVelocity();
            velocity = glm::vec3(v.x(), v.y(), v.z());
        }
    }
    const glm::vec3 lead = glm::vec3(velocity.x, 0.f, velocity.z) *
                           state.streaming.velocityLeadSeconds;
    const float tileSize =
        state.world.tileSize > 0.f ? state.world.tileSize : 512.f;
    const float reach =
        std::max(0.f, static_cast<float>(state.streaming.evictRadiusTiles -
                                         state.streaming.loadRadiusTiles) -
                          0.5f) *
        tileSize;
    const float length = glm::length(lead);
    return length > reach ? lead * (reach / length) : lead;
}

glm::vec3 Gta5StreamOrigin(const Gta5StreamState& state,
                           const glm::vec3& playerOrigin) {
    if (state.seated < 0 ||
        state.seated >= static_cast<int>(state.vehicles.size())) {
        return playerOrigin;
    }
    const btRigidBody* chassis = state.vehicles[state.seated].chassis;
    if (!chassis) return playerOrigin;
    const btVector3& at = chassis->getWorldTransform().getOrigin();
    return glm::vec3(at.x(), at.y(), at.z());
}

}  // namespace sdl3cpp::services::impl
