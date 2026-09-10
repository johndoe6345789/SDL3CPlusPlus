#include "services/interfaces/workflow/rendering/workflow_spotlight_update_step.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow/rendering/spotlight_update_pose.hpp"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <cmath>

namespace sdl3cpp::services::impl {

WorkflowSpotlightUpdateStep::WorkflowSpotlightUpdateStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowSpotlightUpdateStep::GetPluginId() const {
    return "spotlight.update";
}

void WorkflowSpotlightUpdateStep::Execute(const WorkflowStepDefinition& step,
                                          WorkflowContext& context) {
    const auto* spot = context.TryGet<nlohmann::json>("spotlight.state");
    if (!spot) return;

    auto fu = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});

    // All values from JSON — no hardcoded defaults in C++
    const auto offsetVals = spot->value("offset", std::vector<float>{});
    const glm::vec3 offset(offsetVals.size() > 0 ? offsetVals[0] : 0.0f,
                           offsetVals.size() > 1 ? offsetVals[1] : 0.0f,
                           offsetVals.size() > 2 ? offsetVals[2] : 0.0f);

    const SpotlightPose pose =
        ComputeSpotlightPose(*spot, offset, context, logger_);

    const float innerCone = spot->value("inner_cone", 0.0f);
    const float outerCone = spot->value("outer_cone", 0.0f);
    const auto col        = spot->value("color", std::vector<float>{});
    const float intensity = spot->value("intensity", 0.0f);
    const float range     = spot->value("range", 0.0f);

    fu.flash_pos[0]   = pose.position.x;
    fu.flash_pos[1]   = pose.position.y;
    fu.flash_pos[2]   = pose.position.z;
    fu.flash_pos[3]   = std::cos(glm::radians(innerCone));
    fu.flash_dir[0]   = pose.direction.x;
    fu.flash_dir[1]   = pose.direction.y;
    fu.flash_dir[2]   = pose.direction.z;
    fu.flash_dir[3]   = std::cos(glm::radians(outerCone));
    fu.flash_color[0] = (col.size() > 0 ? col[0] : 0.0f) * intensity;
    fu.flash_color[1] = (col.size() > 1 ? col[1] : 0.0f) * intensity;
    fu.flash_color[2] = (col.size() > 2 ? col[2] : 0.0f) * intensity;
    fu.flash_color[3] = range;

    context.Set<rendering::FragmentUniformData>("render.frag_uniforms", fu);
}

}  // namespace sdl3cpp::services::impl
