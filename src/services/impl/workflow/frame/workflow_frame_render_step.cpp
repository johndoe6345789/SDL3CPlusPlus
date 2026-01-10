#include "workflow_frame_render_step.hpp"
#include "../workflow_step_io_resolver.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowFrameRenderStep::WorkflowFrameRenderStep(std::shared_ptr<IRenderCoordinatorService> renderService,
                                                 std::shared_ptr<ILogger> logger)
    : renderService_(std::move(renderService)),
      logger_(std::move(logger)) {}

std::string WorkflowFrameRenderStep::GetPluginId() const {
    return "frame.render";
}

void WorkflowFrameRenderStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!renderService_) {
        throw std::runtime_error("frame.render requires an IRenderCoordinatorService");
    }
    WorkflowStepIoResolver resolver;
    const std::string elapsedKey = resolver.GetRequiredInputKey(step, "elapsed");
    const auto* elapsed = context.TryGet<double>(elapsedKey);
    if (!elapsed) {
        throw std::runtime_error("frame.render missing elapsed input");
    }
    const ViewState* viewState = nullptr;
    auto it = step.inputs.find("view_state");
    if (it != step.inputs.end()) {
        viewState = context.TryGet<ViewState>(it->second);
        if (!viewState) {
            throw std::runtime_error("frame.render missing view_state input");
        }
    }
    const std::vector<GuiCommand>* guiCommands = nullptr;
    auto guiIt = step.inputs.find("gui_commands");
    if (guiIt != step.inputs.end()) {
        guiCommands = context.TryGet<std::vector<GuiCommand>>(guiIt->second);
        if (!guiCommands) {
            throw std::runtime_error("frame.render missing gui_commands input");
        }
    }
    if (viewState || guiCommands) {
        renderService_->RenderFrameWithOverrides(static_cast<float>(*elapsed), viewState, guiCommands);
    } else {
        renderService_->RenderFrame(static_cast<float>(*elapsed));
    }
    if (logger_) {
        logger_->Trace("WorkflowFrameRenderStep", "Execute",
                       "elapsed=" + std::to_string(*elapsed),
                       "Render frame complete");
    }
}

}  // namespace sdl3cpp::services::impl
