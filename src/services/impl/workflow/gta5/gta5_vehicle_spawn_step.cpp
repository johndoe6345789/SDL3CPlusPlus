#include "services/interfaces/workflow/gta5/gta5_vehicle_spawn_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_ground_probe.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_hold.hpp"
#include "services/interfaces/workflow/gta5/gta5_load_progress.hpp"

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
    // Needs the map index; gta5.tiles.load publishes it once it is built.
    if (!state_ || spawned_ || !state_->assets) return;
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!device || !world) return;

    glm::vec3 position(Gta5NumberOr(step, "x", 0.f),
                             Gta5NumberOr(step, "y", 0.f),
                             Gta5NumberOr(step, "z", 0.f));
    // Wait for the ground under the car, or it drops through a world that
    // has not streamed in yet.
    if (!MeasureGta5LoadProgress(*state_, position).done) return;
    // Dropped from drop_height onto whatever is below, rather than put at
    // a fixed y: it lands on the road and settles on its springs, instead
    // of starting inside a kerb or floating above one.
    float ground = 0.f;
    const bool found = Gta5GroundBelow(world, position, ground);
    if (found) position.y = ground + Gta5NumberOr(step, "drop_height", 1.5f);
    if (logger_) {
        logger_->Info("gta5.vehicle.spawn: " +
                      (found ? "ground at y=" + std::to_string(ground)
                             : "no ground below the spawn; " +
                                   DescribeGta5Column(world, position.x,
                                                      position.z)));
    }

    Gta5VehicleSpec spec;
    spec.model = Gta5ParameterOr(step, "model", "");
    spec.wheel = Gta5ParameterOr(step, "wheel_model", "");
    spec.paint = glm::vec3(Gta5NumberOr(step, "paint_r", 1.f),
                           Gta5NumberOr(step, "paint_g", 1.f),
                           Gta5NumberOr(step, "paint_b", 1.f));
    spec.wheelRadius = Gta5NumberOr(step, "wheel_radius", spec.wheelRadius);
    spec.wheelWidth = Gta5NumberOr(step, "wheel_width", spec.wheelWidth);
    spec.heading = Gta5NumberOr(step, "heading", 0.f);
    spec.rideHeight = Gta5NumberOr(step, "ride_height", spec.rideHeight);

    // One attempt: reading from the map either works or it never will.
    spawned_ = true;
    SpawnGta5Vehicle(*state_, spec, position,
                     Gta5NumberOr(step, "mass", 1600.f), device, world,
                     logger_);
}

}  // namespace sdl3cpp::services::impl
