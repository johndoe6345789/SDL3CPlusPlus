#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_poll_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/input_event_drain.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/input_keyboard_snapshot.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowInputPollStep::WorkflowInputPollStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowInputPollStep::GetPluginId() const {
    return "input.poll";
}

void WorkflowInputPollStep::Execute(const WorkflowStepDefinition&,
                                    WorkflowContext& context) {
    const PolledInputEvents events = DrainInputEvents(context);
    WriteInputEventFlags(context, events);
    WriteKeyboardSnapshot(context);
}

}  // namespace sdl3cpp::services::impl
