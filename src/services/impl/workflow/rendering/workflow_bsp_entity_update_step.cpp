#include "services/interfaces/workflow/rendering/workflow_bsp_entity_update_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_entity_update_helpers.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>

#include <string>

namespace sdl3cpp::services::impl {

WorkflowBspEntityUpdateStep::WorkflowBspEntityUpdateStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspEntityUpdateStep::GetPluginId() const {
    return "bsp.entities.update";
}

void WorkflowBspEntityUpdateStep::Execute(const WorkflowStepDefinition&,
                                          WorkflowContext& context) {
    const auto* entities = context.TryGet<nlohmann::json>("bsp.entities");
    if (!entities || !entities->is_array()) return;

    const std::string playerName =
        context.GetString("physics_player_body", "");
    if (playerName.empty()) return;

    auto* body =
        context.Get<btRigidBody*>("physics_body_" + playerName, nullptr);
    if (!body) return;

    btTransform xform;
    body->getMotionState()->getWorldTransform(xform);
    const btVector3 playerPos = xform.getOrigin();
    btVector3 playerAabbMin;
    btVector3 playerAabbMax;
    body->getCollisionShape()->getAabb(xform, playerAabbMin, playerAabbMax);
    const auto frame =
        static_cast<uint32_t>(context.GetDouble("loop.iteration", 0.0));

    auto collected =
        context.Get<nlohmann::json>("q3.collected", nlohmann::json::object());
    auto inventory =
        context.Get<nlohmann::json>("q3.inventory", nlohmann::json::object());
    auto cooldowns = context.Get<nlohmann::json>(
        "q3.trigger_cooldowns", nlohmann::json::object());

    for (const auto& ent : *entities) {
        const std::string classname = ent.value("classname", std::string{});
        const std::string id        = ent.value("id", std::string{});

        if (TryCollectPickup(ent, classname, id, playerPos, collected,
                             inventory, context, logger_)) {
            continue;
        }
        TryActivateTrigger(ent, classname, id, body, playerPos,
                           playerAabbMin, playerAabbMax, frame, cooldowns,
                           logger_);
    }

    context.Set("q3.collected", collected);
    context.Set("q3.inventory", inventory);
    context.Set("q3.trigger_cooldowns", cooldowns);
}

}  // namespace sdl3cpp::services::impl
