#include "services/interfaces/workflow/stunts/player/stunts_car_drive_step.hpp"

#include "services/interfaces/workflow/stunts/player/stunts_road_query.hpp"
#include "services/interfaces/workflow/stunts/stunts_step_params.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kMetresPerSecondToMph = 2.23694f;

StuntsDriveInput ReadInput(const WorkflowContext& context) {
    StuntsDriveInput input;
    input.throttle =
        std::clamp(context.Get<float>("input.move_forward", 0.f), -1.f, 1.f);
    input.steer =
        std::clamp(context.Get<float>("input.move_right", 0.f), -1.f, 1.f);
    return input;
}

void PlaceCamera(WorkflowContext& context, const StuntsCarState& car,
                 float back, float height) {
    const glm::vec3 along(std::cos(car.heading), 0.f, std::sin(car.heading));
    context.Set("render.camera_pos",
                car.position - along * back + glm::vec3(0.f, height, 0.f));
    context.Set("render.camera_target", car.position + along * 6.f);
}

}  // namespace

WorkflowStuntsCarDriveStep::WorkflowStuntsCarDriveStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<StuntsWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowStuntsCarDriveStep::GetPluginId() const {
    return "stunts.car.drive";
}

void WorkflowStuntsCarDriveStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    if (!state_->loaded) return;
    if (!placed_) {
        car_.position = state_->start.position;
        car_.heading = state_->start.heading;
        placed_ = true;
    }

    const float dt =
        std::clamp(context.Get<float>("physics_dt", 1.f / 60.f), 0.f, 0.1f);
    const StuntsDriveTuning tune = StuntsTuningFor(state_->car.engine);
    car_.onRoad = StuntsOnRoad(*state_, car_.position);
    StepStuntsCar(car_, state_->car.engine, tune, ReadInput(context), dt);
    car_.position = StuntsClampToGrid(*state_, car_.position);
    car_.position.y = state_->params.roadHeight;

    context.Set("stunts.car_pos", car_.position);
    context.Set("stunts.car_heading", car_.heading);
    context.Set("stunts.speed_mph",
                std::fabs(car_.speed) * kMetresPerSecondToMph);
    context.Set("stunts.rpm", car_.rpm);
    context.Set("stunts.gear", car_.gear);
    context.Set("stunts.on_road", car_.onRoad);
    PlaceCamera(context, car_, StuntsNumberOr(step, "camera_back", 14.f),
                StuntsNumberOr(step, "camera_height", 6.f));
}

}  // namespace sdl3cpp::services::impl
