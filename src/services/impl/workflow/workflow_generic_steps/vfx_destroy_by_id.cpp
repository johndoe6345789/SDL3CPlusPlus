#include "services/interfaces/workflow/workflow_generic_steps/vfx_destroy_by_id.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

bool DestroyById(const WorkflowStepDefinition& step,
                 const WorkflowStepParameterResolver& resolver,
                 std::vector<std::string>& effects) {
    const auto* param = resolver.FindParameter(step, "vfx_id");
    if (!param || param->type != WorkflowParameterValue::Type::String ||
        param->stringValue.empty()) {
        return false;
    }
    auto it = std::find(effects.begin(), effects.end(), param->stringValue);
    if (it == effects.end()) return false;
    effects.erase(it);
    return true;
}

bool DestroyByCommaSeparatedIds(const WorkflowStepDefinition& step,
                                const WorkflowStepParameterResolver& resolver,
                                std::vector<std::string>& effects) {
    const auto* param = resolver.FindParameter(step, "vfx_ids");
    if (!param || param->type != WorkflowParameterValue::Type::String ||
        param->stringValue.empty()) {
        return false;
    }

    bool destroyed           = false;
    const std::string& idStr = param->stringValue;
    size_t pos               = 0;
    while (pos < idStr.length()) {
        size_t commaPos = idStr.find(',', pos);
        std::string id =
            idStr.substr(pos, commaPos == std::string::npos ? std::string::npos
                                                            : commaPos - pos);

        // Trim leading whitespace only
        size_t start = id.find_first_not_of(" \t");
        if (start != std::string::npos) id = id.substr(start);

        if (!id.empty()) {
            auto it = std::find(effects.begin(), effects.end(), id);
            if (it != effects.end()) {
                effects.erase(it);
                destroyed = true;
            }
        }

        if (commaPos == std::string::npos) break;
        pos = commaPos + 1;
    }
    return destroyed;
}

}  // namespace sdl3cpp::services::impl
