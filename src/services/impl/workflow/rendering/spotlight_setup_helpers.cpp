#include "services/interfaces/workflow/rendering/spotlight_setup_helpers.hpp"

namespace sdl3cpp::services::impl {

nlohmann::json CopySpotlightParameters(const WorkflowStepDefinition& step) {
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
    return spotlight;
}

void CombineSpotlightVec3(nlohmann::json& spotlight, const char* xKey,
                          const char* yKey, const char* zKey,
                          const char* arrayKey) {
    if (spotlight.contains(xKey) || spotlight.contains(yKey) ||
        spotlight.contains(zKey)) {
        spotlight[arrayKey] = {spotlight.value(xKey, 0.0),
                               spotlight.value(yKey, 0.0),
                               spotlight.value(zKey, 0.0)};
    }
}

}  // namespace sdl3cpp::services::impl
