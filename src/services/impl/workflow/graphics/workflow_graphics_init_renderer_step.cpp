#include "services/interfaces/workflow/graphics/workflow_graphics_init_renderer_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowGraphicsInitRendererStep::WorkflowGraphicsInitRendererStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGraphicsInitRendererStep::GetPluginId() const {
    return "graphics.gpu.init_renderer";
}

void WorkflowGraphicsInitRendererStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string rendererKey = resolver.GetRequiredInputKey(step, "renderer_type");
    const std::string outputRendererKey = resolver.GetRequiredOutputKey(step, "selected_renderer");

    const auto* renderer_str = context.TryGet<std::string>(rendererKey);
    if (!renderer_str) {
        throw std::runtime_error("graphics.gpu.init_renderer requires renderer_type input");
    }

    std::string renderer = *renderer_str;

    // Validate renderer type - SDL3 GPU supports metal, vulkan, d3d12 (no OpenGL)
    if (renderer != "metal" && renderer != "vulkan" &&
        renderer != "d3d12" && renderer != "auto") {
        throw std::runtime_error("graphics.gpu.init_renderer: unsupported renderer type '" + renderer +
                                 "' (valid: metal, vulkan, d3d12, auto)");
    }

    if (logger_) {
        logger_->Trace("WorkflowGraphicsInitRendererStep", "Execute",
                       "renderer=" + renderer,
                       "Renderer type selected");
    }

    context.Set(outputRendererKey, renderer);
}

}  // namespace sdl3cpp::services::impl
