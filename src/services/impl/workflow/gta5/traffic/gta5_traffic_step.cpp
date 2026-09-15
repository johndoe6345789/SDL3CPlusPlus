#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_step.hpp"

#include "services/interfaces/workflow/gta5/effects/gta5_effects_spawn.hpp"
#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_path.hpp"
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
    const float dt =
        std::clamp(context.Get<float>("physics_dt", 1.f / 60.f), 0.f, 0.1f);
    btRigidBody* player = Gta5PlayerBody(context);
    if (!player) return;
    const btVector3& body = player->getWorldTransform().getOrigin();
    const glm::vec3 at(body.x(), body.y(), body.z());
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    Gta5Traffic& traffic = state_->traffic;
    const Gta5EffectsPtr effects = Gta5EffectsOf(context);
    StepGta5Lights(traffic, roads_, at, dt);
    KeepGta5Traffic(traffic, roads_, *state_, at, device, world,
                    effects.get(), logger_);
    DriveGta5Traffic(traffic, roads_, *state_, dt);
    // Drawn from the physics, wheels and all, the same way the player's
    // own car is: the batch reads the cars straight out of the state.
    for (Gta5TrafficCar& car : traffic.cars) UpdateGta5Vehicle(car.car);
    FindGta5Lamps(*state_, traffic, roads_, dt);
    if (effects) ShowGta5Lights(*effects, traffic, roads_);
}

}  // namespace sdl3cpp::services::impl
