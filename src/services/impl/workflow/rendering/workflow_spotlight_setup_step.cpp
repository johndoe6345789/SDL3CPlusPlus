#include "services/interfaces/workflow/rendering/workflow_spotlight_setup_step.hpp"
#include "services/interfaces/workflow/rendering/spotlight_setup_helpers.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

WorkflowSpotlightSetupStep::WorkflowSpotlightSetupStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowSpotlightSetupStep::GetPluginId() const {
    return "spotlight.setup";
}

void WorkflowSpotlightSetupStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    nlohmann::json spotlight = CopySpotlightParameters(step);

    CombineSpotlightVec3(spotlight, "color_r", "color_g", "color_b", "color");
    CombineSpotlightVec3(spotlight, "offset_x", "offset_y", "offset_z",
                        "offset");
    // Rotation is for spotlights attached to a viewmodel, so it can be
    // handed to the same transform the model is drawn with.
    CombineSpotlightVec3(spotlight, "rot_x", "rot_y", "rot_z", "rotation");
    // Position/direction are for static spotlights.
    CombineSpotlightVec3(spotlight, "pos_x", "pos_y", "pos_z", "position");
    CombineSpotlightVec3(spotlight, "dir_x", "dir_y", "dir_z", "direction");

    context.Set("spotlight.state", spotlight);
}

}  // namespace sdl3cpp::services::impl
