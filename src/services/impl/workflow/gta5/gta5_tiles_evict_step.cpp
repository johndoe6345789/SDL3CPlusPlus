#include "services/interfaces/workflow/gta5/gta5_tiles_evict_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_evict_plan.hpp"
#include "services/interfaces/workflow/gta5/gta5_geometry_cache.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <btBulletDynamicsCommon.h>

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5TilesEvictStep::WorkflowGta5TilesEvictStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5TilesEvictStep::GetPluginId() const {
    return "gta5.tiles.evict";
}

void WorkflowGta5TilesEvictStep::Execute(
    const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    if (!state_ || state_->resident.empty()) return;

    const Gta5EvictResult result = ApplyGta5EvictPlan(
        *state_,
        context.Get<btDiscreteDynamicsWorld*>("physics_world",
                                              nullptr));
    if (result.instancesReleased == 0) return;

    // Only sweep once instances have given up their references, so an
    // archetype shared with a still-resident tile is not freed underneath
    // it.
    SweepGta5GeometryCache(
        *state_, context.Get<SDL_GPUDevice*>("gpu_device", nullptr), logger_);

    if (logger_) {
        logger_->Trace("WorkflowGta5TilesEvictStep", "Execute",
                       "dropped=" + std::to_string(result.dropped) +
                           " rebuilt=" + std::to_string(result.rebuilt),
                       "released " +
                           std::to_string(result.instancesReleased) +
                           " instances");
    }
}

}  // namespace sdl3cpp::services::impl
