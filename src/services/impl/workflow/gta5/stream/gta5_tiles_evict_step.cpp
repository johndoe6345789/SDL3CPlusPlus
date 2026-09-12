#include "services/interfaces/workflow/gta5/stream/gta5_tiles_evict_step.hpp"

#include <SDL3/SDL_timer.h>

#include "services/interfaces/workflow/gta5/stream/gta5_evict_plan.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_geometry_cache.hpp"
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

    const std::uint64_t start = SDL_GetTicksNS();
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
    const auto ms = (SDL_GetTicksNS() - start) / 1000000u;
    state_->cost.evict += static_cast<double>(SDL_GetTicksNS() - start) / 1e6;
    if (logger_ && ms >= 3) {
        logger_->Info("gta5.tiles.evict: " + std::to_string(ms) +
                      " ms to release " +
                      std::to_string(result.instancesReleased) +
                      " instances");
    }

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
