#include "services/interfaces/workflow/switchback/checkpoint/switchback_checkpoint_arrow_step.hpp"

#include "services/interfaces/workflow/gta5/player/gta5_shown_transform.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>

#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowSwitchbackCheckpointArrowStep::WorkflowSwitchbackCheckpointArrowStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Gta5StreamState> vehicles)
    : logger_(std::move(logger)), vehicles_(std::move(vehicles)) {}

std::string WorkflowSwitchbackCheckpointArrowStep::GetPluginId() const {
    return "switchback.checkpoint.arrow";
}

void WorkflowSwitchbackCheckpointArrowStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;
    if (!loaded_) LoadOnce(step, context);
    if (!loaded_) return;
    if (context.GetBool("switchback.race.restarted", false)) route_.Reset();
    DrawMarquees(step, context);
    PublishProgress(context);

    if (!vehicles_ || vehicles_->seated < 0 ||
        vehicles_->seated >= static_cast<int>(vehicles_->vehicles.size())) {
        return;
    }
    const btRigidBody* chassis =
        vehicles_->vehicles[vehicles_->seated].chassis;
    if (!chassis || route_.Points().empty() || route_.Finished()) {
        return;
    }

    const btTransform t = Gta5ShownTransform(chassis);
    const glm::vec3 car(t.getOrigin().x(), t.getOrigin().y(),
                        t.getOrigin().z());
    const std::size_t target = route_.TargetIndex();
    if (route_.Update(car, radius_) && logger_) {
        logger_->Trace("WorkflowSwitchbackCheckpointArrowStep", "Execute",
                       "index=" + std::to_string(target),
                       route_.Finished() ? "Race finished"
                                         : "Checkpoint reached");
    }
    PublishProgress(context);
    if (route_.Finished()) return;

    DrawArrow(step, context, car);
}

void WorkflowSwitchbackCheckpointArrowStep::PublishProgress(
    WorkflowContext& context) const {
    context.Set<int>("switchback.checkpoint.passed",
                     static_cast<int>(route_.Passed()));
    context.Set<int>("switchback.checkpoint.total",
                     static_cast<int>(route_.GateCount()));
    context.Set<bool>("switchback.race.finished", route_.Finished());
}

}  // namespace sdl3cpp::services::impl
