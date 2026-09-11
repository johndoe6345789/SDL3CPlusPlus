#include "services/interfaces/workflow/rendering/frame_render_scale.hpp"

#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

float ReadFrameRenderScale(const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;
    const auto* p = params.FindParameter(step, "render_scale");
    if (!p || p->type != WorkflowParameterValue::Type::Number) return 1.0f;
    return std::clamp(static_cast<float>(p->numberValue), 0.25f, 4.0f);
}

}  // namespace sdl3cpp::services::impl
