#include "services/interfaces/workflow/workflow_generic_steps/switch_value_matcher.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

std::string SwitchValueToString(const std::any* value) {
    if (const auto* strVal = std::any_cast<std::string>(value)) {
        return *strVal;
    }
    if (const auto* boolVal = std::any_cast<bool>(value)) {
        return *boolVal ? "true" : "false";
    }
    if (const auto* numVal = std::any_cast<double>(value)) {
        return std::to_string(static_cast<long long>(*numVal));
    }
    if (const auto* intVal = std::any_cast<int>(value)) {
        return std::to_string(*intVal);
    }
    throw std::runtime_error(
        "control.condition.switch: value type must be string, bool, "
        "double, or int");
}

std::string FindSwitchCaseStepId(
    const std::unordered_map<std::string, std::string>& inputs,
    const std::string& valueStr) {
    std::string caseStepId;
    std::string defaultStepId;

    for (const auto& [key, value] : inputs) {
        if (key == "value") continue;

        if (key == "default") {
            defaultStepId = value;
        } else if (key.substr(0, 5) == "case_") {
            if (key.substr(5) == valueStr) {
                caseStepId = value;
                break;
            }
        }
    }

    return caseStepId.empty() ? defaultStepId : caseStepId;
}

}  // namespace sdl3cpp::services::impl
