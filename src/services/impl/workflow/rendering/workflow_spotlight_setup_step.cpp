#include "services/interfaces/workflow/rendering/workflow_spotlight_setup_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

WorkflowSpotlightSetupStep::WorkflowSpotlightSetupStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowSpotlightSetupStep::GetPluginId() const {
    return "spotlight.setup";
}

void WorkflowSpotlightSetupStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    // Pass all parameters straight through to context as JSON
    // No hardcoded defaults — everything comes from the workflow definition
    nlohmann::json spotlight;

    for (const auto& [key, param] : step.parameters) {
        switch (param.type) {
            case WorkflowParameterValue::Type::Number:
                spotlight[key] = param.numberValue;
                break;
            case WorkflowParameterValue::Type::String:
                spotlight[key] = param.stringValue;
                break;
            case WorkflowParameterValue::Type::Bool:
                spotlight[key] = param.boolValue;
                break;
            default:
                break;
        }
    }

    // Build color array from individual components if present
    if (spotlight.contains("color_r") || spotlight.contains("color_g") || spotlight.contains("color_b")) {
        spotlight["color"] = {
            spotlight.value("color_r", 0.0),
            spotlight.value("color_g", 0.0),
            spotlight.value("color_b", 0.0)
        };
    }

    // Build offset array from individual components if present
    if (spotlight.contains("offset_x") || spotlight.contains("offset_y") || spotlight.contains("offset_z")) {
        spotlight["offset"] = {
            spotlight.value("offset_x", 0.0),
            spotlight.value("offset_y", 0.0),
            spotlight.value("offset_z", 0.0)
        };
    }

    // Build position/direction arrays for static spotlights
    if (spotlight.contains("pos_x") || spotlight.contains("pos_y") || spotlight.contains("pos_z")) {
        spotlight["position"] = {
            spotlight.value("pos_x", 0.0),
            spotlight.value("pos_y", 0.0),
            spotlight.value("pos_z", 0.0)
        };
    }
    if (spotlight.contains("dir_x") || spotlight.contains("dir_y") || spotlight.contains("dir_z")) {
        spotlight["direction"] = {
            spotlight.value("dir_x", 0.0),
            spotlight.value("dir_y", 0.0),
            spotlight.value("dir_z", 0.0)
        };
    }

    context.Set("spotlight.state", spotlight);
}

}  // namespace sdl3cpp::services::impl
