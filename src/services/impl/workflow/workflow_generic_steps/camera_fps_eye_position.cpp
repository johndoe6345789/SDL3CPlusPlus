#include "services/interfaces/workflow/workflow_generic_steps/camera_fps_update_helpers.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

glm::vec3 ComputeCameraFpsEyePosition(const WorkflowContext& context,
                                      float eyeHeight) {
    const float actualEyeHeight =
        context.Get<float>("camera_eye_height", eyeHeight);

    auto playerName = context.GetString("physics_player_body", "");
    glm::vec3 eyePos(0.0f, actualEyeHeight, 0.0f);

    if (!playerName.empty()) {
        auto* body =
            context.Get<btRigidBody*>("physics_body_" + playerName, nullptr);
        if (body) {
            btTransform transform;
            body->getMotionState()->getWorldTransform(transform);
            btVector3 pos = transform.getOrigin();
            eyePos = glm::vec3(pos.x(), pos.y() + actualEyeHeight, pos.z());
        }
    }
    return eyePos;
}

}  // namespace sdl3cpp::services::impl
