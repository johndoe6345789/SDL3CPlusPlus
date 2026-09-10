#include "services/interfaces/workflow/quake3/workflow_q3_missiles_impact_step.hpp"
#include "services/interfaces/workflow/quake3/q3_missiles_impact_helpers.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>

namespace sdl3cpp::services::impl {

WorkflowQ3MissilesImpactStep::WorkflowQ3MissilesImpactStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3MissilesImpactStep::GetPluginId() const {
    return "q3.missiles.impact";
}

void WorkflowQ3MissilesImpactStep::Execute(
    const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    auto missiles =
        context.Get<sdl3cpp::q3::MissileList>("q3.missiles", nullptr);
    if (!missiles || missiles->empty()) {
        // Clean up stale prev-position entries for finished missiles.
        prevPositions_.clear();
        return;
    }

    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    const glm::vec3 playerPos =
        context.Get<glm::vec3>("q3.player_pos", glm::vec3(0.f));
    auto bots = context.Get<nlohmann::json>("q3.bots", nlohmann::json::array());
    auto botDamage =
        context.Get<nlohmann::json>("q3.bot_damage", nlohmann::json::object());
    int pendingDamage = context.Get<int>("q3.pending_damage", 0);

    // Phase 1: detect impacts via raycast from prev -> current position.
    DetectQ3MissileImpacts(world, *missiles, prevPositions_);

    // Phase 2: splash damage for every exploded missile.
    ApplyQ3MissileSplashDamage(*missiles, playerPos, bots, botDamage,
                              pendingDamage, prevPositions_);

    // Phase 3: remove all exploded missiles.
    missiles->erase(
        std::remove_if(missiles->begin(), missiles->end(),
                       [](const sdl3cpp::q3::Q3Missile& m) {
                           return m.exploded;
                       }),
        missiles->end());

    context.Set("q3.missiles", missiles);
    context.Set<int>("q3.pending_damage", pendingDamage);
    context.Set("q3.bot_damage", botDamage);
}

}  // namespace sdl3cpp::services::impl
