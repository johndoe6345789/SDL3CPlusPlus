#include "services/interfaces/workflow/rendering/workflow_bsp_build_collision_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_brush_collision.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowBspBuildCollisionStep::WorkflowBspBuildCollisionStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspBuildCollisionStep::GetPluginId() const {
    return "bsp.build_collision";
}

void WorkflowBspBuildCollisionStep::Execute(const WorkflowStepDefinition&,
                                            WorkflowContext& context) {
    auto bspDataPtr = context.Get<std::shared_ptr<std::vector<uint8_t>>>(
        "bsp_raw_data", nullptr);
    if (!bspDataPtr) {
        throw std::runtime_error(
            "bsp.build_collision: bsp_raw_data not in context");
    }

    auto bspConfig =
        context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    float scale = bspConfig.value("scale", 1.0f / 32.0f);
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!world) {
        if (logger_) logger_->Info("bsp.build_collision: No physics world");
        return;
    }

    // Remove previous BSP collision body to prevent ghost geometry on map
    // reload. NOTE: only the solid body is cleaned up here, matching the
    // original monolith — a stale bsp_playerclip_body is not removed on
    // reload; preserved as-is rather than silently fixed.
    auto* prevBody = context.Get<btRigidBody*>("bsp_collision_body",
                                                nullptr);
    RemoveBspCollisionBody(world, prevBody);
    context.Set<btRigidBody*>("bsp_collision_body", nullptr);

    const BspBrushCollisionShapes shapes =
        BuildBspBrushCollisionShapes(*bspDataPtr, scale);

    if (shapes.solidBrushes > 0) {
        auto* body = AddStaticCollisionBody(world, shapes.solid, 1.0f);
        context.Set<btRigidBody*>("bsp_collision_body", body);
    } else {
        delete shapes.solid;
    }

    if (shapes.clip) {
        auto* body = AddFilteredStaticCollisionBody(
            world, shapes.clip, btBroadphaseProxy::CharacterFilter,
            btBroadphaseProxy::AllFilter);
        context.Set<btRigidBody*>("bsp_playerclip_body", body);
    }

    if (logger_) {
        logger_->Info("bsp.build_collision: " +
                      std::to_string(shapes.solidBrushes) +
                      " solid brushes, " +
                      std::to_string(shapes.clipBrushes) +
                      " player-clip, " +
                      std::to_string(shapes.skippedBrushes) + " skipped");
    }
}

}  // namespace sdl3cpp::services::impl
