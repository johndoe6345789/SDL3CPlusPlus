#include "services/interfaces/workflow/graphics/workflow_graphics_init_viewport_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <cmath>

namespace sdl3cpp::services::impl {

WorkflowGraphicsInitViewportStep::WorkflowGraphicsInitViewportStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGraphicsInitViewportStep::GetPluginId() const {
    return "graphics.gpu.init_viewport";
}

void WorkflowGraphicsInitViewportStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string widthKey = resolver.GetRequiredInputKey(step, "width");
    const std::string heightKey = resolver.GetRequiredInputKey(step, "height");
    const std::string outputViewportKey = resolver.GetRequiredOutputKey(step, "viewport_config");

    const auto* width = context.TryGet<double>(widthKey);
    const auto* height = context.TryGet<double>(heightKey);

    // Optional present_mode: check step parameters first, then context fallback.
    std::string presentMode;
    WorkflowStepParameterResolver paramResolver;
    if (const auto* p = paramResolver.FindParameter(step, "present_mode")) {
        if (p->type == WorkflowParameterValue::Type::String) presentMode = p->stringValue;
    }
    if (presentMode.empty()) {
        if (const auto* p = context.TryGet<std::string>("present_mode")) presentMode = *p;
    }

    if (!width || !height) {
        throw std::runtime_error("graphics.gpu.init_viewport requires width and height inputs");
    }

    uint32_t w = static_cast<uint32_t>(*width);
    uint32_t h = static_cast<uint32_t>(*height);

    if (w == 0 || h == 0) {
        throw std::runtime_error("graphics.gpu.init_viewport: width and height must be > 0");
    }

    if (logger_) {
        logger_->Trace("WorkflowGraphicsInitViewportStep", "Execute",
                       "width=" + std::to_string(w) + ", height=" + std::to_string(h),
                       "Viewport dimensions set");
    }

    // Store viewport dimensions in context for next step
    // Format: JSON object with width/height for GPU init step
    nlohmann::json viewport_config = {
        {"width", w},
        {"height", h},
        {"aspect_ratio", static_cast<double>(w) / static_cast<double>(h)}
    };
    if (!presentMode.empty()) {
        viewport_config["present_mode"] = presentMode;
    }

    context.Set(outputViewportKey, viewport_config);
}

}  // namespace sdl3cpp::services::impl
