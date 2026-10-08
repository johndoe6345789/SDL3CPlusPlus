#include "services/interfaces/workflow/switchback/menu/switchback_menu_step.hpp"

#include "services/interfaces/workflow/switchback/menu/switchback_menu_logic.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowSwitchbackMenuStep::WorkflowSwitchbackMenuStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<SwitchbackSession> session)
    : logger_(std::move(logger)), session_(std::move(session)) {}

std::string WorkflowSwitchbackMenuStep::GetPluginId() const {
    return "switchback.menu";
}

void WorkflowSwitchbackMenuStep::Execute(const WorkflowStepDefinition&,
                                         WorkflowContext& context) {
    if (!session_) return;
    SwitchbackMenuKeys keys;
    keys.up = context.GetBool("input_key_up_pressed", false);
    keys.down = context.GetBool("input_key_down_pressed", false);
    keys.enter = context.GetBool("input_key_enter_pressed", false);
    keys.escape = context.GetBool("input_key_escape_pressed", false);
    const SwitchbackScreen before = session_->screen;
    StepSwitchbackMenu(*session_, keys,
                       context.GetBool("switchback.race.finished", false));
    if (before == session_->screen || !logger_) return;
    logger_->Trace(
        "WorkflowSwitchbackMenuStep", "Execute",
        "screen=" + std::to_string(static_cast<int>(before)) + " -> " +
            std::to_string(static_cast<int>(session_->screen)),
        "Switchback screen changed");
}

}  // namespace sdl3cpp::services::impl
