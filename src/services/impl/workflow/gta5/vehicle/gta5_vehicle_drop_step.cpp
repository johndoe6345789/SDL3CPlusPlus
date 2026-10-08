#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_drop_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/resource/gta5_assets_index_step.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_hold.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_spec.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <btBulletDynamicsCommon.h>

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5VehicleDropStep::WorkflowGta5VehicleDropStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5VehicleDropStep::GetPluginId() const {
    return "gta5.vehicle.drop";
}

void WorkflowGta5VehicleDropStep::Execute(const WorkflowStepDefinition& step,
                                          WorkflowContext& context) {
    if (!state_ || attempted_) return;
    if (!Gta5AssetsReady(*state_, logger_)) return;
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!device || !world) return;
    attempted_ = true;

    glm::vec3 position(Gta5NumberOr(step, "x", 0.f),
                       Gta5NumberOr(step, "y", 0.f),
                       Gta5NumberOr(step, "z", 0.f));
    float ground = 0.f;
    if (!Gta5GroundBelow(world, position, ground)) {
        if (logger_) {
            logger_->Warn("gta5.vehicle.drop: no ground under (" +
                          std::to_string(position.x) + ", " +
                          std::to_string(position.z) + ")");
        }
        return;
    }
    position.y = ground + Gta5NumberOr(step, "drop_height", 1.5f);

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
    const float mass = Gta5NumberOr(step, "mass", 1600.f);

    if (logger_) {
        logger_->Trace("WorkflowGta5VehicleDropStep", "Execute",
                       "model=" + spec.model + ", ground=" +
                           std::to_string(ground),
                       "Dropping vehicle");
    }
    SpawnGta5Vehicle(*state_, spec, position, mass, device, world, logger_);
}

}  // namespace sdl3cpp::services::impl
