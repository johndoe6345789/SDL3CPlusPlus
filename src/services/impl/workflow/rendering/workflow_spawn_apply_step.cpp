#include "services/interfaces/workflow/rendering/workflow_spawn_apply_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_q3_coordinates.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>

#include <string>

namespace sdl3cpp::services::impl {

WorkflowSpawnApplyStep::WorkflowSpawnApplyStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowSpawnApplyStep::GetPluginId() const {
    return "spawn.apply";
}

namespace {

/// Reads spawn_x/spawn_y/spawn_z/spawn_angle as raw Quake3 map units
/// (the same units as a .map/BSP entity's "origin", e.g. straight out of
/// a level editor or an entity dump) so overriding the spawn doesn't
/// require doing the Z-up -> Y-up conversion by hand. Returns false if
/// x/y/z aren't all present, leaving the BSP-provided spawn untouched.
bool ReadSpawnOverride(const WorkflowStepDefinition& step, float scale,
                       float* x, float* y, float* z, float* angle) {
    WorkflowStepParameterResolver params;
    auto getFloat = [&](const char* key, float* out) {
        const auto* p = params.FindParameter(step, key);
        if (!p || p->type != WorkflowParameterValue::Type::String ||
            p->stringValue.empty()) {
            return false;
        }
        try {
            *out = std::stof(p->stringValue);
            return true;
        } catch (...) {
            return false;
        }
    };

    float qx = 0, qy = 0, qz = 0;
    if (!getFloat("spawn_x", &qx) || !getFloat("spawn_y", &qy) ||
        !getFloat("spawn_z", &qz)) {
        return false;
    }
    const auto p = ConvertQ3Point(qx, qy, qz, scale);
    *x           = p[0];
    *y           = p[1] + 1.0f;
    *z           = p[2];
    getFloat("spawn_angle", angle);
    return true;
}

}  // namespace

void WorkflowSpawnApplyStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    const auto* spawn = context.TryGet<nlohmann::json>("bsp.spawn");
    if (!spawn) return;

    auto playerName = context.GetString("physics_player_body", "");
    if (playerName.empty()) return;

    auto* body = context.Get<btRigidBody*>("physics_body_" + playerName, nullptr);
    if (!body) return;

    float x = spawn->value("x", 0.0f);
    float y = spawn->value("y", 5.0f);
    float z = spawn->value("z", 0.0f);
    float angle = spawn->value("angle", 0.0f);

    const auto bspConfig =
        context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    const float scale = bspConfig.value("scale", 1.0f / 32.0f);
    const bool overridden = ReadSpawnOverride(step, scale, &x, &y, &z, &angle);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(x, y, z));
    body->setWorldTransform(transform);
    body->getMotionState()->setWorldTransform(transform);
    body->setLinearVelocity(btVector3(0, 0, 0));
    body->setAngularVelocity(btVector3(0, 0, 0));

    // Convert Q3 angle (degrees, 0=east) to engine yaw (radians)
    float yaw = angle * 3.14159265f / 180.0f;
    context.Set<float>("camera_yaw", yaw);

    if (logger_) {
        logger_->Info(std::string("spawn.apply: Player at (") +
                     (overridden ? "override " : "") + std::to_string(x) +
                     ", " + std::to_string(y) + ", " + std::to_string(z) +
                     ") yaw=" + std::to_string(yaw));
    }
}

}
