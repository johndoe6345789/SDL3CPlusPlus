#include "workflow_frame_begin_step.hpp"
#include "../workflow_step_io_resolver.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowFrameBeginStep::WorkflowFrameBeginStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowFrameBeginStep::GetPluginId() const {
    return "frame.begin";
}

void WorkflowFrameBeginStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string deltaKey = resolver.GetRequiredInputKey(step, "delta");
    const std::string elapsedKey = resolver.GetRequiredInputKey(step, "elapsed");

    const auto* delta = context.TryGet<double>(deltaKey);
    const auto* elapsed = context.TryGet<double>(elapsedKey);
    if (!delta || !elapsed) {
        throw std::runtime_error("frame.begin requires delta and elapsed inputs");
    }

    if (logger_) {
        logger_->Trace("WorkflowFrameBeginStep", "Execute",
                       "delta=" + std::to_string(*delta) +
                       ", elapsed=" + std::to_string(*elapsed),
                       "Starting frame workflow");
    }
}

}  // namespace sdl3cpp::services::impl
