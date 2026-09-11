#include "services/interfaces/workflow/gta5/gta5_map_look_step.hpp"

#include "services/interfaces/workflow_context.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5MapLookStep::WorkflowGta5MapLookStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGta5MapLookStep::GetPluginId() const {
    return "gta5.map.look";
}

void WorkflowGta5MapLookStep::Execute(const WorkflowStepDefinition&,
                                      WorkflowContext& context) {
    const bool open = context.GetBool("gta5.map.open", false);
    context.Set<bool>("gta5.mouse_look",
                      context.GetBool("movement_active", true) && !open);
    if (!open) return;
    context.Set<float>("input_mouse_rel_x", 0.f);
    context.Set<float>("input_mouse_rel_y", 0.f);
}

}  // namespace sdl3cpp::services::impl
