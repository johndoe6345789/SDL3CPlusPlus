#include "services/interfaces/workflow/quake3/workflow_q3_nav_build_step.hpp"
#include "services/interfaces/workflow/quake3/q3_nav_build.hpp"
#include "services/interfaces/workflow/quake3/q3_nav_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowQ3NavBuildStep::WorkflowQ3NavBuildStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3NavBuildStep::GetPluginId() const {
    return "q3.nav.build";
}

void WorkflowQ3NavBuildStep::Execute(
    const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    if (context.GetBool("q3.nav_built", false)) return;

    auto* world = context.Get<btDiscreteDynamicsWorld*>(
        "physics_world", nullptr);
    if (!world) {
        if (logger_) {
            logger_->Warn(
                "q3.nav.build: no physics_world — skipping nav graph");
        }
        context.Set("q3.nav_built", true);
        return;
    }

    const auto* spawnPts = context.TryGet<nlohmann::json>(
        "bsp.spawn_points");
    if (!spawnPts) spawnPts = context.TryGet<nlohmann::json>("bsp.entities");

    const NavBuildAabb aabb = ComputeNavBuildAabb(spawnPts);
    auto graph = SampleNavGraph(world, aabb);
    ConnectNavNeighbors(world, *graph);

    context.Set("q3.nav_graph", graph);
    context.Set("q3.nav_built", true);

    if (logger_) {
        logger_->Info("q3.nav.build: built " +
            std::to_string(graph->nodes.size()) + " nav nodes");
    }
}

}  // namespace sdl3cpp::services::impl
