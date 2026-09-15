#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicles_free_step.hpp"

#include "services/interfaces/workflow/gta5/effects/gta5_effects.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_seat.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5VehiclesFreeStep::WorkflowGta5VehiclesFreeStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5VehiclesFreeStep::GetPluginId() const {
    return "gta5.vehicles.free";
}

void WorkflowGta5VehiclesFreeStep::Execute(
    const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    if (!state_) return;
    const std::size_t mine = state_->vehicles.size();
    const std::size_t driven = state_->traffic.cars.size();
    // Without the world a body cannot be taken out of it, so the shapes
    // would be freed under a world still holding them. Say so and leave
    // them: a leak on the way out beats a crash on the way out.
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!world) {
        if (logger_ && (mine || driven)) {
            logger_->Info("gta5.vehicles.free: no physics world, "
                          "leaving " + std::to_string(mine + driven) +
                          " cars to the process");
        }
        return;
    }
    const Gta5EffectsPtr effects = Gta5EffectsOf(context);
    DestroyGta5Vehicles(*state_, world, effects.get());
    if (logger_) {
        logger_->Info("gta5.vehicles.free: gave back " +
                      std::to_string(mine) + " of the player's cars and " +
                      std::to_string(driven) + " of the traffic's");
    }
}

}  // namespace sdl3cpp::services::impl
