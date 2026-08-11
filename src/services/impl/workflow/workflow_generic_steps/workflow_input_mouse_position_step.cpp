#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_mouse_position_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowInputMousePositionStep::WorkflowInputMousePositionStep(std::shared_ptr<IInputService> inputService,
                                                               std::shared_ptr<ILogger> logger)
    : inputService_(std::move(inputService)),
      logger_(std::move(logger)) {}

std::string WorkflowInputMousePositionStep::GetPluginId() const {
    return "input.mouse.position";
}

void WorkflowInputMousePositionStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!inputService_) {
        throw std::runtime_error("input.mouse.position requires an IInputService");
    }

    WorkflowStepIoResolver resolver;
    const std::string xOutputKey = resolver.GetRequiredOutputKey(step, "x");
    const std::string yOutputKey = resolver.GetRequiredOutputKey(step, "y");

    const auto [x, y] = inputService_->GetMousePosition();
    context.Set(xOutputKey, static_cast<double>(x));
    context.Set(yOutputKey, static_cast<double>(y));

    if (logger_) {
        logger_->Trace("WorkflowInputMousePositionStep", "Execute",
                       "x=" + std::to_string(x) +
                           ", y=" + std::to_string(y) +
                           ", x_output=" + xOutputKey +
                           ", y_output=" + yOutputKey,
                       "Retrieved mouse position");
    }
}

}  // namespace sdl3cpp::services::impl
