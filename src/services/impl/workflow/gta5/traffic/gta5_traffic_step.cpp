#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_step.hpp"

#include "services/interfaces/workflow/gta5/effects/gta5_effects_spawn.hpp"
#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_path.hpp"
#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_input.hpp"

#include <algorithm>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5TrafficStep::WorkflowGta5TrafficStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5TrafficStep::GetPluginId() const {
    return "gta5.traffic";
}

void WorkflowGta5TrafficStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    if (!state_) return;
    if (!tried_ && !Load(step, context)) return;
    state_->traffic.clear();
    const float dt =
        std::clamp(context.Get<float>("physics_dt", 1.f / 60.f), 0.f, 0.1f);
    btRigidBody* player = Gta5PlayerBody(context);
    if (!player) return;
    const btVector3& body = player->getWorldTransform().getOrigin();
    const glm::vec3 at(body.x(), body.y(), body.z());
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    StepGta5Lights(traffic_, roads_, at, dt);
    KeepGta5Traffic(traffic_, roads_, *state_, at, device, world,
                    logger_);
    DriveGta5Traffic(traffic_, roads_, dt);
    // Drawn from the physics, wheels and all, the same way the player's
    // own car is: its transform is where Bullet has actually put it.
    for (Gta5TrafficCar& car : traffic_.cars) {
        UpdateGta5Vehicle(car.car);
        if (car.car.instance.geometry) {
            state_->traffic.push_back(car.car.instance);
        }
        for (const Gta5Instance& wheel : car.car.wheels) {
            if (car.car.hasWheels && wheel.geometry) {
                state_->traffic.push_back(wheel);
            }
        }
    }
    FindGta5Lamps(*state_, traffic_, roads_, dt);
    ShowGta5Lights(*Gta5EffectsOf(context), traffic_, roads_);
}

}  // namespace sdl3cpp::services::impl
