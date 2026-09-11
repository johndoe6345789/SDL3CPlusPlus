#include "services/interfaces/workflow/gta5/gta5_vehicle_spawn_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <btBulletDynamicsCommon.h>

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5VehicleSpawnStep::WorkflowGta5VehicleSpawnStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5VehicleSpawnStep::GetPluginId() const {
    return "gta5.vehicle.spawn";
}

void WorkflowGta5VehicleSpawnStep::Execute(const WorkflowStepDefinition& step,
                                           WorkflowContext& context) {
    if (!state_ || spawned_) return;

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!device || !world) return;

    // Wait for the tile under the spawn point, or the car drops through
    // a world that has not streamed in yet.
    if (state_->resident.empty()) return;

    const std::string model = Gta5ResolvePath(step, context, "model", "");
    if (model.empty()) return;

    const glm::vec3 position(
        static_cast<float>(Gta5ParameterOrInt(step, "x", 0)),
        static_cast<float>(Gta5ParameterOrInt(step, "y", 0)),
        static_cast<float>(Gta5ParameterOrInt(step, "z", 0)));
    const auto mass =
        static_cast<float>(Gta5ParameterOrInt(step, "mass", 1600));

    spawned_ = SpawnGta5Vehicle(*state_, model, position, mass, device, world,
                                logger_);
}

}  // namespace sdl3cpp::services::impl
