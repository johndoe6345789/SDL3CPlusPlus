#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_sync_transforms_step.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <cstring>

namespace sdl3cpp::services::impl {

WorkflowPhysicsSyncTransformsStep::WorkflowPhysicsSyncTransformsStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPhysicsSyncTransformsStep::GetPluginId() const {
    return "physics.sync_transforms";
}

void WorkflowPhysicsSyncTransformsStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    auto bodies = context.Get<nlohmann::json>("physics_bodies", nlohmann::json::array());
    float time = static_cast<float>(context.GetDouble("frame.elapsed", 0.0));

    for (const auto& nameVal : bodies) {
        std::string name = nameVal.get<std::string>();

        auto* body = context.Get<btRigidBody*>("physics_body_" + name, nullptr);
        if (!body) continue;

        auto visual = context.Get<nlohmann::json>("physics_visual_" + name, nlohmann::json::object());

        // Read Bullet transform
        btTransform xform;
        body->getMotionState()->getWorldTransform(xform);
        btVector3 pos = xform.getOrigin();

        // Build rotation matrix
        btMatrix3x3 basis = xform.getBasis();
        glm::mat4 rotation(1.0f);
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                rotation[c][r] = basis[r][c];

        // Apply spinning animation if flagged
        if (visual.value("spinning", false)) {
            float spinX = visual.value("spin_speed_x", 1.0f);
            float spinY = visual.value("spin_speed_y", 0.7f);
            rotation = glm::rotate(rotation, time * spinX, glm::vec3(1, 0, 0));
            rotation = glm::rotate(rotation, time * spinY, glm::vec3(0, 1, 0));
        }

        // Get AABB for size
        btVector3 aabb_min, aabb_max;
        body->getCollisionShape()->getAabb(btTransform::getIdentity(), aabb_min, aabb_max);

        // Store computed transform in context
        std::vector<float> rot_data(16);
        std::memcpy(rot_data.data(), glm::value_ptr(rotation), sizeof(float) * 16);

        nlohmann::json sync;
        sync["pos"] = {pos.x(), pos.y(), pos.z()};
        sync["rotation"] = rot_data;
        sync["size"] = {
            aabb_max.x() - aabb_min.x(),
            aabb_max.y() - aabb_min.y(),
            aabb_max.z() - aabb_min.z()
        };
        context.Set("body_sync_" + name, sync);
    }
}

}  // namespace sdl3cpp::services::impl
