#include "services/interfaces/workflow/gta5/player/gta5_shown_transform.hpp"

namespace sdl3cpp::services::impl {

btTransform Gta5ShownTransform(const btRigidBody* body) {
    if (!body) return btTransform::getIdentity();
    if (const btMotionState* motion = body->getMotionState()) {
        btTransform shown;
        motion->getWorldTransform(shown);
        return shown;
    }
    return body->getWorldTransform();
}

}  // namespace sdl3cpp::services::impl
